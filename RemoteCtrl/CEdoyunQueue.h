#pragma once
#include "pch.h"
#include <atomic>
#include <list>
#include "EdoyunThread.h"

template<class T>
class CEdoyunQueue
{//�̰߳�ȫ�Ķ��У�����IOCPʵ�֣�
public:
	enum {
		EQNone,
		EQPush,//���
		EQPop,//����
		EQSize,//���д�С
		EQClear//��ն���
	};
	
	//Ͷ����Ϣ�ṹ��
	typedef struct IocpParam {
		size_t nOperator;//����
		T Data;//����
		HANDLE hEvent;//pop������Ҫ�� �ȴ��߳���д������ɻ���
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam() {
			nOperator = EQNone;
		}
	}PPARAM;//Post Parameter ����Ͷ����Ϣ�Ľṹ��
public:
	CEdoyunQueue() {
		m_lock = false;//�����Ƿ�������
		//����һ����ȫ�˿�ӳ��
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);

		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompeletionPort != NULL) {
			//����һ���߳� ������ȫ�˿��¼�
			m_hThread = (HANDLE)_beginthread(
				&CEdoyunQueue<T>::threadEntry,
				0, this
			);
		}
	}
	virtual ~CEdoyunQueue() {
		if (m_lock)return;//�����Ѿ�������
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);//����һ�������ź� �߳��յ��Զ�����
		WaitForSingleObject(m_hThread, INFINITE);//�����߳�

		//������ȫ�˿� ��Ȼ�߳���Ҳ����� ��ֹ��ȫ�˿ڴ����� �߳�û�����ɹ� 
		if (m_hCompeletionPort != NULL) {
			HANDLE hTemp = m_hCompeletionPort;
			m_hCompeletionPort = NULL;
			CloseHandle(hTemp);
		}
	}

	//���
	bool PushBack(const T& data) {
		IocpParam* pParam = new IocpParam(EQPush, data);
		if (m_lock) {//�Ѿ�������
			delete pParam;
			return false;
		}
		//��������ź�
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		//printf("push back done %d %08p\r\n", ret, (void*)pParam);
		return ret;
	}
	//����
	virtual bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//�����¼��������ȴ��̲߳�����֮����
		IocpParam Param(EQPop, data, hEvent);
		if (m_lock) {//�Ѿ�����
			if (hEvent)CloseHandle(hEvent);
			return false;
		}
		//���ͳ����ź�
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}
		//�ȴ��߳���ɲ���
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		//���س�������
		if (ret) {
			data = Param.Data;
		}
		return ret;
	}
	//�鿴���д�С
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
	//��ն���
	bool Clear() {
		if (m_lock)return false;
		IocpParam* pParam = new IocpParam(EQClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		//printf("Clear %08p\r\n", (void*)pParam);
		return ret;
	}
protected:
	//�߳����
	static void threadEntry(void* arg) {
		CEdoyunQueue<T>* thiz = (CEdoyunQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	//����������
	virtual void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator)
		{
		case EQPush://���
			m_lstData.push_back(pParam->Data);
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		case EQPop://����
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			//�����������߳�
			if (pParam->hEvent != NULL)SetEvent(pParam->hEvent);
			break;
		case EQSize://��С
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL)
				SetEvent(pParam->hEvent);
			break;
		case EQClear://��ն���
			m_lstData.clear();
			delete pParam;
			//printf("delete %08p\r\n", (void*)pParam);
			break;
		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
	//�����߳�(ѭ���ں��ϱ��¼�)
	virtual void threadMain() {
		DWORD dwTransferred = 0;//�ں���д�˶�������
		PPARAM* pParam = NULL;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;//�ص��ṹ
		while (GetQueuedCompletionStatus(
			m_hCompeletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped, INFINITE))
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {//���յ��˳��ź�
				printf("thread is prepare to exit!\r\n");
				break;
			}
			//���յ��¼� ����
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		//��ֹ����֪ͨ����ʱ���ں��л��в�������
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
		//�ر���ȫ�˿�
		HANDLE hTemp = m_hCompeletionPort;
		m_hCompeletionPort = NULL;
		CloseHandle(hTemp);
	}
protected:
	std::list<T> m_lstData;//����
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock;//������������
};



template<class T>
class EdoyunSendQueue :public CEdoyunQueue<T>, public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* EDYCALLBACK)(T& data);
	EdoyunSendQueue(ThreadFuncBase* obj, EDYCALLBACK callback)
		:CEdoyunQueue<T>(), m_base(obj), m_callback(callback)
	{
		m_thread.Start();//�����߳�
		m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&EdoyunSendQueue<T>::threadTick));//ע���̺߳���
	}
	virtual ~EdoyunSendQueue() {
		m_base = NULL;
		m_callback = NULL;
	}
protected:
	virtual bool PopFront(T& data) {
		return false;
	};
	//����
	bool PopFront() {
		typename CEdoyunQueue<T>::IocpParam* Param = new typename CEdoyunQueue<T>::IocpParam(CEdoyunQueue<T>::EQPop, T());
		if (CEdoyunQueue<T>::m_lock) {//�Ѿ�������
			delete Param;
			return false;
		}
		//���ͳ����ź�
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
	//�¼�������(���ظ���)
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
	ThreadFuncBase* m_base;//�̺߳�������
	EDYCALLBACK m_callback;//�ص� int(*)(T& data)
	EdoyunThread m_thread;//�̶߳���
};

typedef EdoyunSendQueue<std::vector<char>>::EDYCALLBACK  SENDCALLBACK;