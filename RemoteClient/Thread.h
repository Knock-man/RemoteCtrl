#pragma once
#include<atomic>
#include<vector>
#include<mutex>
class ThreadFuncBase {};

typedef int (ThreadFuncBase::* FUNCTYPPE)();
class ThreadWorker {
public:
	ThreadWorker() :thiz(NULL), func(NULL){}
	ThreadWorker(ThreadFuncBase* obj, FUNCTYPPE f):thiz(obj),func(f){}
	ThreadWorker(const ThreadWorker& threadworker) {
		thiz = threadworker.thiz;
		func = threadworker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& threadworker) {
		if (this != &threadworker) {
			thiz = threadworker.thiz;
			func = threadworker.func;
		}
		return *this;
	}

	int operator()() {
		if (IsValid()) {
			return (thiz->*func)();
		}
		return -1;
	}
	bool IsValid() {
		return (thiz != NULL) && (func != NULL);
	}
	
private:
	ThreadFuncBase* thiz;
	FUNCTYPPE func;
};
class Thread
{
public:
	Thread()
	{
		m_hThread = NULL;
	}
	~Thread() {
		Stop();
		//if (isValid())CloseHandle(m_hThread);
		//m_bStatus = false;
		//delete &m_worker;
	}

	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(Thread::ThreadEntry, 0, this);
		if (!isValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	bool isValid() {
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE))return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	bool Stop() {
		if (m_bStatus == false) return true;
		m_bStatus = false;
		return WaitForSingleObject(m_hThread, INFINITY) == WAIT_OBJECT_0;
	}

	void UpdateWorker(const ThreadWorker& worker = ThreadWorker()) {
		m_worker.store(worker);//m_worker = worker;
	}

	//true��ʾ���� false��ʾ�Ѿ����乤��
	bool IsIdle() {
		return !m_worker.load().IsValid();
	}
private:
	static void ThreadEntry(void *arg) {
		Thread* thiz = (Thread*)arg;
		thiz->threadWorkey();
		_endthread();
	}
	//�����߳�
	void threadWorkey() 
	{
		while (m_bStatus) {
			ThreadWorker& worker = m_worker.load();
			if (worker.IsValid())
			{
				int ret = worker();
				if (ret != 0) {
					CStringA str;
					str.Format(TEXT("thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}
				else if (ret <= 0) {
					m_worker.store(ThreadWorker());//m_worker = ThreadWorker();
				}
			}
			else
			{
				Sleep(1);
			}
			
			
		}
	}

protected:
	
private:
	HANDLE m_hThread;
	bool m_bStatus;//false��ʾ�߳̽�Ҫ�ر� true��ʾ�߳���������
	std::atomic<ThreadWorker> m_worker;
 };

class ThreadPool
{
public:
	ThreadPool(size_t size) {
		m_threads.resize(size);
	}
	ThreadPool() {};
	~ThreadPool() {
		Stop();
		m_threads.clear();
	};
	bool Invake() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			if (m_threads[i].Start()==false) {
				ret = false;
				break;
			}
		}

		//�̳߳�����ʧ�� �Ѿ��������߳�ҲҪ�رյ�
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); i++)
			{
				m_threads[i].Stop();
			}
		}
		return true;
	}

	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			m_threads[i].Stop();
		}
	}

	//����-1 ��ʾ����ʧ�� �����̶߳���æ ���ڵ���0  ��ʾ��index���߳̽�������
	int DispatchWorker(const ThreadWorker& worker)
	{
		
		int index = -1;
		m_lock.lock();
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			if (m_threads[i].IsIdle()) {
				
				m_threads[i].UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	//�ж�index�߳��Ƿ���Ч
	bool checkThreadValid(size_t index) {
		if (index <= m_threads.size()) {
			return m_threads[index].isValid();
		}
		return false;
	}
private:
	std::vector<Thread> m_threads;
	std::mutex m_lock;
};