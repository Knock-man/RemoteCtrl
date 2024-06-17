#pragma once
#include "pch.h"
#include <atomic>
#include <list>


template<class T>
class CEdoyunQueue
{//�̰߳�ȫ�Ķ���(����IOCPʵ��)
public:
    //��������
    enum {
        EQNone,
        EQPush,//���
        EQPop,//����
        EQSize,//��С
        EQClear//���
    };

    //Ͷ����Ϣ���ݰ�
    typedef void(*PFUNC)(void*);
    typedef struct IocpParam {
        size_t nOperator;//����
        T Data;//����
        HANDLE hEvent;//pop������Ҫ��,�߳�ͨ��
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
    }PPARAM;//Post Parameter ����Ͷ����Ϣ�Ľṹ��

 

public:
    CEdoyunQueue()
    {
        m_lock = false;
        m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);//����һ����ɶ˿�
        m_hThread = INVALID_HANDLE_VALUE;
        if (m_hCompeletionPort != NULL)//��ȫ�˿ڴ����ɹ��Ŵ����߳�
        {    
            //�����߳�
            m_hThread = (HANDLE)_beginthread(&CEdoyunQueue<T>::threadEntry, 0,this);
        }
    }
    ~CEdoyunQueue()
    {
        if (m_lock)return;//�Ѿ�������
        m_lock = true;
        //���ͽ���״̬
        PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);
        WaitForSingleObject(m_hThread, INFINITE);//�ȴ��߳̽���
        
        //��Ȼ�߳���Ҳ�����ˣ����Ƿ�ֹ�̴߳���ʧ��,��ȫ�˿ڴ����ɹ���������ȫ�˿�û�л����ڴ�й©
        if (m_hCompeletionPort != NULL)
        {
            HANDLE hTemp = m_hCompeletionPort;//�ر���ɶ˿�
            m_hCompeletionPort = NULL;
            CloseHandle(hTemp);
        }
        

    }

    //�������
    bool PushBack(const T& data)
    {
        IocpParam* pParam = new IocpParam(EQPush, data);//���new ��Ҫ���߳����ͷ��ڴ�
        if (m_lock)
        {
            delete pParam;
            return false;
        }
        //Ͷ�ݵ���Ϣ������
        BOOL ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
        if (ret == false)delete pParam;
        return ret;
    }

    //���ݳ���
    bool PopFront(T& data)
    {
        HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        IocpParam pParam(EQPop, data, hEvent);//ջ�ڴ治��Ҫ�ֶ��ͷ�
        if (m_lock)//�Ѿ�������
        {
            if (hEvent)CloseHandle(hEvent);
            return false;
        }
        //Ͷ�ݵ���Ϣ������
        BOOL ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL);
        if (ret == false)
        {
            CloseHandle(hEvent);
            return false;
        }

        //�����ȴ��߳���ɲ���������
        ret =  WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
        if (ret)
        {
            data = pParam.Data;
        }
        return ret;
    }
    //��ȡ��Ϣ���д�С
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
        ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;//���� �ȴ��̻߳���(�߳̽���С���)
        if (ret)
        {
            return pParam.nOperator;
        }
        return -1;
    }

    //�����Ϣ����
    bool Clear() {
        if (m_lock) return false;//����ʱ��׼���
        IocpParam* pParam = new IocpParam(EQClear, T());
        BOOL ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
        if (ret == false)delete pParam;
        return ret;
    }
private:
    //�߳����
    static void threadEntry(void* arg) {
        CEdoyunQueue<T>* thiz = (CEdoyunQueue<T>*)arg;
        thiz->threadMain();
        _endthread;
    }
    //����������
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
            //����Event
            if (pParam->hEvent != NULL) SetEvent(pParam->hEvent);
            break;
        case EQSize:
            pParam->nOperator = m_lstData.size();
            //����Event
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
    //�̺߳���
    void threadMain()
    {
        PPARAM* pParam = NULL;
        DWORD dwTransferred = 0;
        ULONG_PTR CompletionKey = 0;
        OVERLAPPED* pOverlapped = NULL;
      
        //ѭ�����ն�����Ϣ �̰߳�ȫ
        while (GetQueuedCompletionStatus(
            m_hCompeletionPort,
            &dwTransferred,
            &CompletionKey,
            &pOverlapped, INFINITE))
        {
            if ((dwTransferred == 0) || (CompletionKey == NULL)) {//�˳���Ϣ(��������)
                printf("thread is prepare to exit!\r\n");
                break;
            }
            //������Ϣ
            pParam = (PPARAM*)CompletionKey;
            DealParam(pParam);
        }
        //��ֹ��������Post�˳��ź�֮����Ϣ���л��в�������
        while (GetQueuedCompletionStatus(
            m_hCompeletionPort,
            &dwTransferred,
            &CompletionKey,
            &pOverlapped, 0))//�ȴ�0����
        {
            if ((dwTransferred == 0) || (CompletionKey == NULL)) {
                printf("thread is prepare to exit!\r\n");
                continue;
            }
            pParam = (PPARAM*)CompletionKey;
            DealParam(pParam);
        }
        //�ر���ȫ�˿�ӳ��
        HANDLE hTemp = m_hCompeletionPort;//�ر���ɶ˿���Ϣ����
        m_hCompeletionPort = NULL;
        CloseHandle(hTemp);
    }

private:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;
    std::atomic<bool> m_lock;//������������
};