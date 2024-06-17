#pragma once
#include "pch.h"
#include <atomic>
#include <list>


template<class T>
class CEdoyunQueue
{//线程安全的队列(利用IOCP实现)
public:
    //操作类型
    enum {
        EQNone,
        EQPush,//入队
        EQPop,//出队
        EQSize,//大小
        EQClear//清空
    };

    //投递信息数据包
    typedef void(*PFUNC)(void*);
    typedef struct IocpParam {
        size_t nOperator;//操作
        T Data;//数据
        HANDLE hEvent;//pop操作需要的,线程通信
        IocpParam(int op, const T& data, HANDLE hEve=NULL)
        {
            nOperator = op;
            Data = data;
            hEvent = hEve;
        }
        IocpParam()
        {
            nOperator = EQNone;
        }
    }PPARAM;//Post Parameter 用于投递信息的结构体

 

public:
    CEdoyunQueue()
    {
        m_lock = false;
        m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);//创建一个完成端口
        m_hThread = INVALID_HANDLE_VALUE;
        if (m_hCompeletionPort != NULL)//完全端口创建成功才创建线程
        {    
            //创建线程
            m_hThread = (HANDLE)_beginthread(&CEdoyunQueue<T>::threadEntry, 0,this);
        }
    }
    ~CEdoyunQueue()
    {
        if (m_lock)return;//已经析构了
        m_lock = true;
        //发送结束状态
        PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);
        WaitForSingleObject(m_hThread, INFINITE);//等待线程结束
        
        //虽然线程内也回收了，但是防止线程创建失败,完全端口创建成功，导致完全端口没有回收内存泄漏
        if (m_hCompeletionPort != NULL)
        {
            HANDLE hTemp = m_hCompeletionPort;//关闭完成端口
            m_hCompeletionPort = NULL;
            CloseHandle(hTemp);
        }
        

    }

    //入队请求
    bool PushBack(const T& data)
    {
        IocpParam* pParam = new IocpParam(EQPush, data);//入队new 需要在线程中释放内存
        if (m_lock)
        {
            delete pParam;
            return false;
        }
        //投递到消息队列中
        BOOL ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
        if (ret == false)delete pParam;
        return ret;
    }

    //数据出队
    bool PopFront(T& data)
    {
        HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        IocpParam pParam(EQPop, data, hEvent);//栈内存不需要手动释放
        if (m_lock)//已经析构了
        {
            if (hEvent)CloseHandle(hEvent);
            return false;
        }
        //投递到消息队列中
        BOOL ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL);
        if (ret == false)
        {
            CloseHandle(hEvent);
            return false;
        }

        //阻塞等待线程完成操作，唤醒
        ret =  WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
        if (ret)
        {
            data = pParam.Data;
        }
        return ret;
    }
    //获取消息队列大小
    size_t Size()
    {
        HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        IocpParam pParam(EQSize, T(), hEvent);
        if (m_lock)
        {
            if (hEvent)CloseHandle(hEvent);
            return -1;
        }
        BOOL ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL);
        if (ret == false)
        {
            CloseHandle(hEvent);
            return -1;
        }
        ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;//阻塞 等待线程唤醒(线程将大小填好)
        if (ret)
        {
            return pParam.nOperator;
        }
        return -1;
    }

    //清空消息队列
    bool Clear() {
        if (m_lock) return false;//析构时不准入队
        IocpParam* pParam = new IocpParam(EQClear, T());
        BOOL ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
        if (ret == false)delete pParam;
        return ret;
    }
private:
    //线程入口
    static void threadEntry(void* arg) {
        CEdoyunQueue<T>* thiz = (CEdoyunQueue<T>*)arg;
        thiz->threadMain();
        _endthread;
    }
    //操作处理函数
    void DealParam(PPARAM* pParam)
    {
        switch (pParam->nOperator)
        {
        case EQPush:
            m_lstData.push_back(pParam->Data);
            delete pParam;
            break;
        case EQPop:
            if (m_lstData.size() > 0) {
                pParam->Data = m_lstData.front();
                m_lstData.pop_front();
            }
            //唤醒Event
            if (pParam->hEvent != NULL) SetEvent(pParam->hEvent);
            break;
        case EQSize:
            pParam->nOperator = m_lstData.size();
            //唤醒Event
            if (pParam->hEvent != NULL) SetEvent(pParam->hEvent);
            break;
        case EQClear:
            m_lstData.clear();
            delete pParam;
            break;
        default:
            OutputDebugStringA("unlnown operator!\r\n");
            break;
        }
    }
    //线程函数
    void threadMain()
    {
        PPARAM* pParam = NULL;
        DWORD dwTransferred = 0;
        ULONG_PTR CompletionKey = 0;
        OVERLAPPED* pOverlapped = NULL;
      
        //循环接收队列消息 线程安全
        while (GetQueuedCompletionStatus(
            m_hCompeletionPort,
            &dwTransferred,
            &CompletionKey,
            &pOverlapped, INFINITE))
        {
            if ((dwTransferred == 0) || (CompletionKey == NULL)) {//退出消息(析构发送)
                printf("thread is prepare to exit!\r\n");
                break;
            }
            //处理消息
            pParam = (PPARAM*)CompletionKey;
            DealParam(pParam);
        }
        //防止析构发送Post退出信号之后消息队列还有残留数据
        while (GetQueuedCompletionStatus(
            m_hCompeletionPort,
            &dwTransferred,
            &CompletionKey,
            &pOverlapped, 0))//等待0毫秒
        {
            if ((dwTransferred == 0) || (CompletionKey == NULL)) {
                printf("thread is prepare to exit!\r\n");
                continue;
            }
            pParam = (PPARAM*)CompletionKey;
            DealParam(pParam);
        }
        //关闭完全端口映射
        HANDLE hTemp = m_hCompeletionPort;//关闭完成端口消息队列
        m_hCompeletionPort = NULL;
        CloseHandle(hTemp);
    }

private:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;
    std::atomic<bool> m_lock;//队列正在析构
};