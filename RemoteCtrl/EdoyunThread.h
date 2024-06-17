#pragma once
#include "pch.h"
#include <atomic>
#include <vector>
#include <mutex>
#include <Windows.h>

class ThreadFuncBase {};
typedef int (ThreadFuncBase::* FUNCTYPE)();

//线程工作对象
class ThreadWorker {
public:
	ThreadWorker() :thiz(NULL), func(NULL) {};

	ThreadWorker(void* obj, FUNCTYPE f) :thiz((ThreadFuncBase*)obj), func(f) {}

	ThreadWorker(const ThreadWorker& worker) {
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) {
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}

	int operator()() {
		if (IsValid()) {
			return (thiz->*func)();
		}
		return -1;
	}

	//是否有效
	bool IsValid() const {
		return (thiz != NULL) && (func != NULL);
	}

private:
	ThreadFuncBase* thiz;//ThreadFuncBase对象
	FUNCTYPE func;//ThreadFuncBase对象的成员函数
};

//线程对象
class EdoyunThread
{
public:
	EdoyunThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}

	~EdoyunThread() {
		Stop();
	}

	//开启线程 true 表示成功 false表示失败
	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&EdoyunThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	//线程是否有效
	bool IsValid() {//返回true表示有效 返回false表示线程异常或者已经终止
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE))return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	//停止线程
	bool Stop() {
		if (m_bStatus == false)return true;
		m_bStatus = false;
		bool ret = WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;
		UpdateWorker();
		return ret;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {
		if (m_worker.load() != NULL && m_worker.load() != &worker) {//回收原来线程工作对象
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		if (!worker.IsValid()) {
			m_worker.store(NULL);
			return;
		}
		//设置线程工作对象
		m_worker.store(new ::ThreadWorker(worker));
	}

	//true表示空闲 false表示已经分配了工作
	bool IsIdle() {
		if (m_worker == NULL)return true;
		return !m_worker.load()->IsValid();
	}
private:
	//线程函数
	void ThreadWorker() {
		while (m_bStatus) {
			if (m_worker == NULL) {//没有注册线程工作对象 等待
				Sleep(1);
				continue;
			}
			//注册了线程工作对象
			::ThreadWorker worker = *m_worker.load();
			if (worker.IsValid()) {
				int ret = worker();//执行工作对象的工作函数
				if (ret != 0) {
					CString str;
					str.Format(_T("thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}
				if (ret < 0) {//出错
					m_worker.store(NULL);
				}
			}
			else {
				Sleep(1);
			}
		}
	}
	//线程入口函数
	static void ThreadEntry(void* arg) {
		EdoyunThread* thiz = (EdoyunThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}
private:
	HANDLE m_hThread;
	bool m_bStatus;//false 表示线程将要关闭  true 表示线程正在运行
	std::atomic<::ThreadWorker*> m_worker;//线程工作对象
};

//线程池
class EdoyunThreadPool
{
public:
	EdoyunThreadPool(size_t size) {
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++)
			m_threads[i] = new EdoyunThread();//创建线程对象
	}
	EdoyunThreadPool() {}
	~EdoyunThreadPool() {
		Stop();
		for (size_t i = 0; i < m_threads.size(); i++)//销毁线程对象
		{
			EdoyunThread* pThread = m_threads[i];
			m_threads[i] = NULL;
			delete pThread;
		}

		m_threads.clear();
	}
	//启动线程吃
	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->Start() == false) {//启动线程
				ret = false;
				break;
			}
		}
		//启动失败，所有线程都应该停止
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); i++) {
				m_threads[i]->Stop();
			}
		}
		return ret;
	}
	//停止线程池
	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i]->Stop();
		}
	}

	//向线程池中分配工作对象
	//返回-1 表示分配失败，所有线程都在忙 大于等于0，表示第n个线程分配来做这个事情
	int DispatchWorker(const ThreadWorker& worker) {
		int index = -1;
		m_lock.lock();
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i] != NULL && m_threads[i]->IsIdle()) {//找到空闲线程
				m_threads[i]->UpdateWorker(worker);//设置该线程的工作对象
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	//检查index线程是否有效
	bool CheckThreadValid(size_t index) {
		if (index < m_threads.size()) {
			return m_threads[index]->IsValid();
		}
		return false;
	}
private:
	std::mutex m_lock;
	std::vector<EdoyunThread*> m_threads;//线程池
};