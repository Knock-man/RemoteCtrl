#pragma once
#include "pch.h"
#include <atomic>
#include <list>
#include "EdoyunThread.h"

template<class T>
class CEdoyunQueue
{//线程安全的队列（利用IOCP实现）
public:
	enum {
		EQNone,
		EQPush,//入队
		EQPop,//出队
		EQSize,//队列大小
		EQClear//清空队列
	};
	
	//投递信息结构体
	typedef struct IocpParam {
		size_t nOperator;//操作
		T Data;//数据
		HANDLE hEvent;//pop操作需要的 等待线程填写数据完成唤醒
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam() {
			nOperator = EQNone;
		}
	}PPARAM;//Post Parameter 用于投递信息的结构体
public:
	CEdoyunQueue() {
		m_lock = false;//队列是否在析构
		//创建一个完全端口映射
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);

		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompeletionPort != NULL) {
			//开启一个线程 处理完全端口事件
			m_hThread = (HANDLE)_beginthread(
				&CEdoyunQueue<T>::threadEntry,
				0, this
			);
		}
	}
	virtual ~CEdoyunQueue() {
		if (m_lock)return;//队列已经析构了
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);//发送一个结束信号 线程收到自动结束
		WaitForSingleObject(m_hThread, INFINITE);//回收线程

		//回收完全端口 虽然线程中也会回收 防止完全端口创建了 线程没启动成功 
		if (m_hCompeletionPort != NULL) {
			HANDLE hTemp = m_hCompeletionPort;
			m_hCompeletionPort = NULL;
			CloseHandle(hTemp);
		}
	}

	//入队
	bool PushBack(const T& data) {
		IocpParam* pParam = new IocpParam(EQPush, data);
		if (m_lock) {//已经析构了
			delete pParam;
			return false;
		}
		//发送入队信号
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		//printf("push back done %d %08p\r\n", ret, (void*)pParam);
		return ret;
	}
	//出队
	virtual bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//创建事件，阻塞等待线程操作完之后唤醒
		IocpParam Param(EQPop, data, hEvent);
		if (m_lock) {//已经析构
			if (hEvent)CloseHandle(hEvent);
			return false;
		}
		//发送出队信号
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}
		//等待线程完成操作
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		//传回出队数据
		if (ret) {
			data = Param.Data;
		}
		return ret;
	}
	//查看队列大小
	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(EQSize, T(), hEvent);
		if (m_lock) {
			if (hEvent)CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			return Param.nOperator;
		}
		return -1;
	}
	//清空队列
	bool Clear() {
		if (m_lock)return false;
		IocpParam* pParam = new IocpParam(EQClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		//printf("Clear %08p\r\n", (void*)pParam);
		return ret;
	}
protected:
	//线程入口
	static void threadEntry(void* arg) {
		CEdoyunQueue<T>* thiz = (CEdoyunQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	//操作处理函数
	virtual void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator)
		{
		case EQPush://入队
			m_lstData.push_back(pParam->Data);
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		case EQPop://出队
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			//唤醒阻塞主线程
			if (pParam->hEvent != NULL)SetEvent(pParam->hEvent);
			break;
		case EQSize://大小
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;
		case EQClear://清空队列
			m_lstData.clear();
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
	//监听线程(循环内核上报事件)
	virtual void threadMain() {
		DWORD dwTransferred = 0;//内核填写了多少数据
		PPARAM* pParam = NULL;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;//重叠结构
		while (GetQueuedCompletionStatus(
			m_hCompeletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped, INFINITE))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {//接收到退出信号
				printf("thread is prepare to exit!\r\n");
				break;
			}
			//接收到事件 处理
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		//防止析构通知结束时，内核中还有残留数据
		while (GetQueuedCompletionStatus(
			m_hCompeletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped, 0))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare to exit!\r\n");
				continue;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		//关闭完全端口
		HANDLE hTemp = m_hCompeletionPort;
		m_hCompeletionPort = NULL;
		CloseHandle(hTemp);
	}
protected:
	std::list<T> m_lstData;//队列
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock;//队列正在析构
};



template<class T>
class EdoyunSendQueue :public CEdoyunQueue<T>, public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* EDYCALLBACK)(T& data);
	EdoyunSendQueue(ThreadFuncBase* obj, EDYCALLBACK callback)
		:CEdoyunQueue<T>(), m_base(obj), m_callback(callback)
	{
		m_thread.Start();//启动线程
		m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&EdoyunSendQueue<T>::threadTick));//注册线程函数
	}
	virtual ~EdoyunSendQueue() {
		m_base = NULL;
		m_callback = NULL;
	}
protected:
	virtual bool PopFront(T& data) {
		return false;
	};
	//出队
	bool PopFront() {
		typename CEdoyunQueue<T>::IocpParam* Param = new typename CEdoyunQueue<T>::IocpParam(CEdoyunQueue<T>::EQPop, T());
		if (CEdoyunQueue<T>::m_lock) {//已经析构了
			delete Param;
			return false;
		}
		//发送出队信号
		bool ret = PostQueuedCompletionStatus(CEdoyunQueue<T>::m_hCompeletionPort, sizeof(typename CEdoyunQueue<T>::PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			delete Param;
			return false;
		}
		return ret;
	}
	int threadTick() {
		if (CEdoyunQueue<T>::m_lstData.size() > 0) {
			PopFront();
		}
		Sleep(1);
		return 0;
	}
	//事件处理函数(重载父类)
	virtual void DealParam(typename CEdoyunQueue<T>::PPARAM* pParam) override {
		switch (pParam->nOperator)
		{
		case CEdoyunQueue<T>::EQPush:
			CEdoyunQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		case CEdoyunQueue<T>::EQPop:
			if (CEdoyunQueue<T>::m_lstData.size() > 0) {
				pParam->Data = CEdoyunQueue<T>::m_lstData.front();
				if ((m_base->*m_callback)(pParam->Data) == 0)
					CEdoyunQueue<T>::m_lstData.pop_front();
			}
			delete pParam;
			break;
		case CEdoyunQueue<T>::EQSize:
			pParam->nOperator = CEdoyunQueue<T>::m_lstData.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;
		case CEdoyunQueue<T>::EQClear:
			CEdoyunQueue<T>::m_lstData.clear();
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
private:
	ThreadFuncBase* m_base;//线程函数对象
	EDYCALLBACK m_callback;//回调 int(*)(T& data)
	EdoyunThread m_thread;//线程对象
};

typedef EdoyunSendQueue<std::vector<char>>::EDYCALLBACK  SENDCALLBACK;