#ifndef __THREADPROC_H__
#define __THREADPROC_H__
#include <atlbase.h>
#include <atlsync.h>
#include <atlcoll.h>

/////////////////////////////////////////////////////////////////////////
// CFakeDrvIoFunc
// 假IO，啥都没干
class CFakeDrvIoFunc
{
public:
	CFakeDrvIoFunc(){};
};

//////////////////////////////////////////////////////////////////////////
// CThreadProcT
template < class T, class CCritSec = CComMultiThreadModel::AutoCriticalSection, class CIOModel = CFakeDrvIoFunc> 
class CThreadProcT
{
protected:
	typedef CComCritSecLock<CCritSec> CAutoCritSecLock;
	CCritSec m_csThreadStart; // 线程创建和结束用。防止一线程在调用StartThread，另外一线程在调用EndThread导致野线程的产生

public:
	CHandle m_ThreadHandle;
	unsigned int m_nThreadId;
	bool m_bExitThread;

protected:
	static unsigned int __stdcall TryThreadStartRoutine(void* pParam)
	{
		__try
		{
			ThreadStartRoutine(pParam);
		}
		__except( UnhandledExceptionFilter( GetExceptionInformation() ) )
		{
			__noop;
		}
		return 0;
	}

	static unsigned int __stdcall ThreadStartRoutine(void* pParam)
	{
		CIOModel DrvThisFuncIo;
		T* pThis = (T*)pParam;
		return (unsigned int)pThis->ThreadProc();
	}

public:
	CThreadProcT(void)
		: m_bExitThread(false)
		, m_nThreadId(0)
	{
	}
	~CThreadProcT(void)
	{
		if(m_ThreadHandle.m_h != NULL)
		{
			m_bExitThread = true;
			WaitForSingleObject(
				m_ThreadHandle,
				INFINITE);
			m_ThreadHandle.Close();
		}
	}

	HANDLE StartThread(
		T* pThis,
		unsigned nStackSize = 0,
		LPSECURITY_ATTRIBUTES lpThreadAttributes = NULL, 
		DWORD dwCreationFlags = 0)
	{
		CAutoCritSecLock Lock(m_csThreadStart);
		ATLASSERT(m_ThreadHandle.m_h == NULL);
		if(m_ThreadHandle.m_h != NULL)
		{
			return m_ThreadHandle.m_h;
		}
		m_bExitThread = false;
		unsigned nThreadId = 0;
		HANDLE hThread = (HANDLE)_beginthreadex(
			lpThreadAttributes,
			nStackSize,
			&T::TryThreadStartRoutine,
			pThis,
			dwCreationFlags,
			&nThreadId);
		if(hThread != NULL)
		{
			m_ThreadHandle.Attach(hThread);
			m_nThreadId = nThreadId;
		}
		return hThread;
	}

	bool EndThread(DWORD dwMilliseconds = INFINITE)
	{
		CAutoCritSecLock Lock(m_csThreadStart);
		if(m_ThreadHandle.m_h != NULL)
		{
			m_bExitThread = true;
			DWORD dwResult = WaitForSingleObject(
				m_ThreadHandle,
				dwMilliseconds);
			if(dwResult != WAIT_TIMEOUT)
			{
				m_ThreadHandle.Close();
			}
		}
		return( m_ThreadHandle == NULL );
	}

	bool Wait(DWORD dwMilliseconds)
	{
		CAutoCritSecLock Lock(m_csThreadStart);
		if(m_ThreadHandle.m_h != NULL)
		{
			DWORD dwResult = WaitForSingleObject(
				m_ThreadHandle,
				dwMilliseconds);
			if(dwResult != WAIT_TIMEOUT)
			{
				m_ThreadHandle.Close();
			}
		}
		return( m_ThreadHandle == NULL );
	}

public:
	HANDLE GetThreadHandle(void) const
	{
		return m_ThreadHandle;
	}
	operator HANDLE() const
	{
		return m_ThreadHandle;
	}
	DWORD GetThreadId(void) const
	{
		return m_nThreadId;
	}
	bool IsThreadExiting(void) const
	{
		return m_bExitThread;
	}
	void SetThreadExiting(void)
	{
		m_bExitThread = true;
	}
	void ThreadLock(void)
	{
		m_csThreadStart.Lock();
	}
	void ThreadUnlock(void)
	{
		m_csThreadStart.Unlock();
	}
};

//////////////////////////////////////////////////////////////////////////
// CContainedThreadT
template < class T, class CCritSec = CComMultiThreadModel::AutoCriticalSection, class CIOModel = CFakeDrvIoFunc > 
class CContainedThreadT : public CThreadProcT<T, CCritSec, CIOModel>
{
public:
	CContainedThreadT():m_pContainer(NULL),
		Container_ThreadProc(NULL)
	{

	}
protected:
	static unsigned int __stdcall TryThreadStartRoutine(void* pParam)
	{
		typedef CContainedThreadT<T, CCritSec, CIOModel> CContainedThreadEx;
		__try
		{
			ThreadStartRoutine(pParam);
		}
		__except( UnhandledExceptionFilter( GetExceptionInformation() ) )
		{
			__noop;
		}
		return 0;
	}

	static unsigned int __stdcall ThreadStartRoutine(void* pParam)
	{
		typedef CContainedThreadT<T, CCritSec, CIOModel> CContainedThread;

		CIOModel DrvThisFuncIo;
		CContainedThread* pThis = (CContainedThread*)pParam;
		return (unsigned int)pThis->ThreadProc();
	}
	DWORD ThreadProc(void)
	{
		return (m_pContainer->*Container_ThreadProc)();
	}

protected:
	T* m_pContainer;
	DWORD (T::*Container_ThreadProc)(void);

public:
	HANDLE StartThread(
		T* pContainer,
		DWORD (T::*pfnThreadProc)(void),
		unsigned nStackSize = 0,
		LPSECURITY_ATTRIBUTES lpThreadAttributes = NULL, 
		DWORD dwCreationFlags = 0)
	{
		CAutoCritSecLock Lock(m_csThreadStart);
		ATLASSERT(m_ThreadHandle.m_h == NULL);
		if(m_ThreadHandle.m_h != NULL)
		{
			return m_ThreadHandle.m_h;
		}
		m_pContainer = pContainer;
		Container_ThreadProc = pfnThreadProc;
		m_bExitThread = false;
		unsigned nThreadId = 0;
		HANDLE hThread = (HANDLE)_beginthreadex(
			lpThreadAttributes,
			nStackSize,
			&TryThreadStartRoutine,
			this,
			dwCreationFlags,
			&nThreadId);
		if(hThread != NULL)
		{
			m_ThreadHandle.Attach(hThread);
			m_nThreadId = nThreadId;
		}
		return hThread;
	}
};

//////////////////////////////////////////////////////////////////////////
// CSingletonTaskThreadT
template < class T, class CCritSec = CComMultiThreadModel::AutoCriticalSection, class CIOModel = CFakeDrvIoFunc > 
class CSingletonTaskThreadT : public CThreadProcT< CSingletonTaskThreadT<T,CCritSec,CIOModel>, CCritSec, CIOModel>
{
	friend class CThreadProcT< CSingletonTaskThreadT<T,CCritSec,CIOModel>, CCritSec, CIOModel>;
protected:
	enum { Task_None, Task_NewTask, Task_Accept };

	T* m_pContainer;
	void (T::*Container_TaskProc)(void);
	volatile long m_lTaskState;

public:
	CSingletonTaskThreadT(
		T* pContainer,
		void (T::*pfnThreadProc)(void) )
		: m_pContainer(pContainer)
		, Container_TaskProc(pfnThreadProc)
		, m_lTaskState(Task_None)
	{
	}

	HRESULT DoTask(void)
	{
		long lOldState = InterlockedExchange(
			&m_lTaskState, 
			Task_NewTask);
		if(Task_None == lOldState)
		{
			EndThread();
			HANDLE hThread = StartThread(this);
			if(hThread == NULL)
			{
				HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
				if(SUCCEEDED(hr))
					hr = E_FAIL;
				return hr;
			}
		}
		return S_OK;
	}

protected:
	DWORD ThreadProc(void)
	{
		for(;;)
		{
			InterlockedExchange(&m_lTaskState, Task_Accept);
			(m_pContainer->*Container_TaskProc)();
			long lState = InterlockedCompareExchange(
				&m_lTaskState,
				Task_None,
				Task_Accept);
			if(lState == Task_Accept)
				break;
		}
		return 0;
	}
};

//////////////////////////////////////////////////////////////////////////
// CMultiTaskThreadPoolT
template <
	class T,
	class CTaskParamType, 
	class CThreadStorage,
	unsigned c_nMaxThreadCount, 
	class CThreadStateCritSec = CComMultiThreadModel::AutoCriticalSection, 
	class CIOModel = CFakeDrvIoFunc > 
class CMultiTaskThreadPoolT
{
protected:
	typedef CComCritSecLock<CComCriticalSection> CAutoCritSecLock;

	enum { Task_None, Task_NewTask, Task_Accept };

protected:
	CComMultiThreadModel::AutoCriticalSection m_critsec;
	CThreadStateCritSec m_csThreadStart;
	
	T* m_pContainer;
	void (T::*Container_TaskProc)(CThreadStorage&, CTaskParamType&);

	CAtlList<CTaskParamType> m_TaskList;

	CHandle m_ThreadHandle[c_nMaxThreadCount];
	volatile long m_lTaskStates[c_nMaxThreadCount];
	unsigned m_lThreadIndex[c_nMaxThreadCount];

	bool m_bExitThread;

public:
	CMultiTaskThreadPoolT(
		T* pContainer,
		void (T::*pfnThreadProc)(CThreadStorage&, CTaskParamType&) )
		: m_pContainer(pContainer)
		, Container_TaskProc(pfnThreadProc)
		, m_bExitThread(false)
	{
		for(unsigned i = 0; i < c_nMaxThreadCount; ++i)
		{
			m_lTaskStates[i] = Task_None;
			m_lThreadIndex[i] = i;
		}
	}

	~CMultiTaskThreadPoolT(void)
	{
		EndTasks();
	}

	bool IsThreadExiting(void) const
	{
		return m_bExitThread;
	}
	void SetThreadExiting(void)
	{
		m_bExitThread = true;
	}

	void EndTasks(void)
	{
		m_bExitThread = true;

		DWORD nCount = 0;
		CHandle ThreadHandles[c_nMaxThreadCount];

		m_critsec.Lock();
		m_TaskList.RemoveAll();
		m_critsec.Unlock();

		m_csThreadStart.Lock();
		m_bExitThread = true;
		for(unsigned i = 0; i < c_nMaxThreadCount; ++i)
		{
			if(m_ThreadHandle[i] != NULL)
			{
				ThreadHandles[nCount].Attach(m_ThreadHandle[i].Detach());
				++nCount;
			}
			m_lTaskStates[i] = Task_None;
		}
		m_csThreadStart.Unlock();

		if(nCount > 0)
		{
			m_bExitThread = true;
			WaitForMultipleObjects(
				nCount,
				(HANDLE*)ThreadHandles,
				TRUE,
				INFINITE);
		}
	}

	HRESULT AddTask(CTaskParamType& TaskParam)
	{
		HRESULT hr = AddToTaskList(TaskParam);
		if(FAILED(hr))
		{
			return hr;
		}
		return NewTaskNotify();
	}

protected:
	static unsigned int __stdcall TryThreadStartRoutine(void* pParam)
	{
		__try
		{
			ThreadStartRoutine(pParam);
		}
		__except( UnhandledExceptionFilter( GetExceptionInformation() ) )
		{
			__noop;
		}
		return 0;
	}

	static unsigned int __stdcall ThreadStartRoutine(void* pParam)
	{
		CIOModel DrvThisFuncIo;
		unsigned nThreadIndex = *(unsigned*)pParam;
		CMultiTaskThreadPoolT* pThis = CONTAINING_RECORD(
			pParam,
			CMultiTaskThreadPoolT,
			m_lThreadIndex[nThreadIndex]);
		return (unsigned int)pThis->_ThreadProc(nThreadIndex);
	}

	unsigned int __stdcall _ThreadProc(const unsigned c_nThreadIndex)
	{
		ATLASSERT(c_nThreadIndex < c_nMaxThreadCount);
		volatile long* plThreadState = m_lTaskStates + c_nThreadIndex;
		CThreadStorage ThreadStorage;
		bool bThreadStateInit = false;
		for(;;)
		{
			InterlockedExchange(plThreadState, Task_Accept);

			while(m_TaskList.GetCount() > 0 && !m_bExitThread)
			{
				CAutoCritSecLock AutoLock(m_critsec);
				if(m_TaskList.IsEmpty())
					break;
				CTaskParamType TaskParam = m_TaskList.RemoveHead();
				AutoLock.Unlock();
				(m_pContainer->*Container_TaskProc)(ThreadStorage, TaskParam);
			}

			long lState = InterlockedCompareExchange(
				plThreadState,
				Task_None,
				Task_Accept);
			if(lState == Task_Accept)
				break;
		}
		return 0;
	}

protected:
	HRESULT AddToTaskList(CTaskParamType& TaskParam)
	{
		CAutoCritSecLock AutoLock(m_critsec);
		try
		{
			m_TaskList.AddTail(TaskParam);
		}
		catch(...)
		{
			return E_OUTOFMEMORY;
		}
		return S_OK;
	}

	HRESULT NewTaskNotify(void)
	{
		HRESULT hr = S_OK;
		CHandle ThreadHandle;
		m_csThreadStart.Lock();
		m_bExitThread = false;
		for(unsigned i = 0; i < c_nMaxThreadCount; ++i)
		{
			long lOldState = InterlockedExchange(
				&m_lTaskStates[i],
				Task_NewTask);
			if(Task_None == lOldState)
			{
				ThreadHandle.Attach(m_ThreadHandle[i].Detach());

				unsigned nThreadId = 0;
				HANDLE hThread = (HANDLE)_beginthreadex(
					NULL,
					0,
					&TryThreadStartRoutine,
					m_lThreadIndex + i,
					0,
					&nThreadId);
				if(hThread != NULL)
				{
					m_ThreadHandle[i].Attach(hThread);
				}
				else
				{
					if(SUCCEEDED(hr = HRESULT_FROM_WIN32(GetLastError())))
						hr = E_FAIL;
				}
				break;
			}
		}
		m_csThreadStart.Unlock();

		if(ThreadHandle != NULL)
		{
			WaitForSingleObject(
				ThreadHandle,
				INFINITE);
			ThreadHandle.Close();
		}
		return hr;
	}
};


struct nilstruct {};
//////////////////////////////////////////////////////////////////////////
// CPresistMultiTaskThreadPoolT
template <
	class T,
	class CTaskParamType, 
	class CThreadStorage,
	unsigned c_nMaxThreadCount, 
	class CThreadStateCritSec = CComMultiThreadModel::AutoCriticalSection, 
	class CIOModel = CFakeDrvIoFunc > 
class CPresistMultiTaskThreadPoolT
{
protected:
	typedef CComCritSecLock<CComCriticalSection> CAutoCritSecLock;

protected:
	CComMultiThreadModel::AutoCriticalSection m_critsec;
	CThreadStateCritSec m_csThreadStart;

	T* m_pContainer;
	void (T::*Container_TaskProc)(CThreadStorage&, CTaskParamType&);

	CAtlList<CTaskParamType> m_TaskList;

	CHandle m_ThreadHandle[c_nMaxThreadCount];
	unsigned m_lThreadIndex[c_nMaxThreadCount];
	CEvent m_TaskEvent[c_nMaxThreadCount];

	bool m_bExitThread;

public:
	CPresistMultiTaskThreadPoolT(
		T* pContainer,
		void (T::*pfnThreadProc)(CThreadStorage&, CTaskParamType&) )
		: m_pContainer(pContainer)
		, Container_TaskProc(pfnThreadProc)
		, m_bExitThread(false)
	{
		for(unsigned i = 0; i < c_nMaxThreadCount; ++i)
		{
			m_lThreadIndex[i] = i;
		}
	}

	~CPresistMultiTaskThreadPoolT(void)
	{
		EndThreadPool();
	}

	HRESULT Init(void)
	{
		HRESULT hr = S_OK;
		m_csThreadStart.Lock();
		for(unsigned i = 0; i < c_nMaxThreadCount; ++i)
		{
			CEvent Event;
			BOOL bResult = Event.Create(
				NULL, 
				FALSE,
				FALSE,
				NULL);
			if(!bResult)
			{
				if(SUCCEEDED(hr = HRESULT_FROM_WIN32(GetLastError())))
					hr = E_FAIL;
				break;
			}

			m_TaskEvent[i].Attach(Event.Detach());

			unsigned nThreadId = 0;
			HANDLE hThread = (HANDLE)_beginthreadex(
				NULL,
				0,
				&TryThreadStartRoutine,
				m_lThreadIndex + i,
				0,
				&nThreadId);
			if(NULL == hThread)
			{
				if(SUCCEEDED(hr = HRESULT_FROM_WIN32(GetLastError())))
					hr = E_FAIL;
				break;
			}

			m_ThreadHandle[i].Attach(hThread);
		}
		m_csThreadStart.Unlock();
		return hr;
	}

	bool IsThreadExiting(void) const
	{
		return m_bExitThread;
	}
	void SetThreadExiting(void)
	{
		m_bExitThread = true;
	}

	void EndThreadPool(void)
	{
		m_bExitThread = true;

		DWORD nCount = 0;
		CHandle ThreadHandles[c_nMaxThreadCount];

		m_critsec.Lock();
		{
			POSITION pos = m_TaskList.GetHeadPosition();
			while(pos)
			{
				CTaskParamType& TaskParam = m_TaskList.GetNext(pos);
				OnTaskCleanup(TaskParam);
			}
			m_TaskList.RemoveAll();
		}
		m_critsec.Unlock();

		m_csThreadStart.Lock();
		m_bExitThread = true;
		for(unsigned i = 0; i < c_nMaxThreadCount; ++i)
		{
			if(m_ThreadHandle[i] != NULL)
			{
				ThreadHandles[nCount].Attach(m_ThreadHandle[i].Detach());
				++nCount;
			}
		}
		m_csThreadStart.Unlock();

		if(nCount > 0)
		{
			m_bExitThread = true;
			NewTaskNotify();
			WaitForMultipleObjects(
				nCount,
				(HANDLE*)ThreadHandles,
				TRUE,
				INFINITE);
		}
	}

	HRESULT AddTask(CTaskParamType& TaskParam)
	{
		HRESULT hr = AddToTaskList(TaskParam);
		if(FAILED(hr))
		{
			return hr;
		}
		return NewTaskNotify();
	}

protected:
	static unsigned int __stdcall TryThreadStartRoutine(void* pParam)
	{
		__try
		{
			ThreadStartRoutine(pParam);
		}
		__except( UnhandledExceptionFilter( GetExceptionInformation() ) )
		{
			__noop;
		}
		return 0;
	}

	static unsigned int __stdcall ThreadStartRoutine(void* pParam)
	{
		CIOModel DrvThisFuncIo;
		unsigned nThreadIndex = *(unsigned*)pParam;
		CPresistMultiTaskThreadPoolT* pThis = CONTAINING_RECORD(
			pParam,
			CPresistMultiTaskThreadPoolT,
			m_lThreadIndex[nThreadIndex]);
		return (unsigned int)pThis->_ThreadProc(nThreadIndex);
	}

	unsigned int __stdcall _ThreadProc(const unsigned c_nThreadIndex)
	{
		ATLASSERT(c_nThreadIndex < c_nMaxThreadCount);
		CEvent& TaskEvent = m_TaskEvent[c_nThreadIndex];
		CThreadStorage ThreadStorage;
		for(;;)
		{
			WaitForSingleObject(TaskEvent, INFINITE);

			while(m_TaskList.GetCount() > 0 && !m_bExitThread)
			{
				CAutoCritSecLock AutoLock(m_critsec);
				if(m_TaskList.IsEmpty())
					break;
				CTaskParamType TaskParam = m_TaskList.RemoveHead();
				AutoLock.Unlock();
				(m_pContainer->*Container_TaskProc)(ThreadStorage, TaskParam);
			}

			if(m_bExitThread)
				break;
		}
		return 0;
	}

	virtual void OnTaskCleanup(CTaskParamType& TaskParam)
	{

	}

protected:
	HRESULT AddToTaskList(CTaskParamType& TaskParam)
	{
		CAutoCritSecLock AutoLock(m_critsec);
		try
		{
			m_TaskList.AddTail(TaskParam);
		}
		catch(...)
		{
			return E_OUTOFMEMORY;
		}
		return S_OK;
	}

	HRESULT NewTaskNotify(void)
	{
		HRESULT hr = E_FAIL;

		for(unsigned i = 0; i < c_nMaxThreadCount; ++i)
		{
			if(NULL == m_TaskEvent[i].m_h)
				continue;
			BOOL bResult = m_TaskEvent[i].Set();
			if(bResult)
				hr = S_OK;
		}
		return hr;
	}
};

//////////////////////////////////////////////////////////////////////////
// CMultiContainedThreadT
template < class T, unsigned c_nMaxThreadCount, class CIOModel = CFakeDrvIoFunc > 
class CMultiContainedThreadT
{
	typedef CContainedThreadT<T, CComMultiThreadModel::AutoCriticalSection, CIOModel> CContainedThread;

private:
	T* m_pContainer;
	DWORD (T::*Container_TaskProc)(void);

	CContainedThread m_ContainedThread[c_nMaxThreadCount];
	bool m_bExitThread;

public:
	CMultiContainedThreadT(
		T* pContainer,
		DWORD (T::*pfnThreadProc)(void) )
		: m_pContainer(pContainer)
		, Container_TaskProc(pfnThreadProc)
		, m_bExitThread(true)
	{
	}

	HRESULT StartThread(void)
	{
		HRESULT hr = S_OK;
		m_bExitThread = false;
		for (int n = 0; n < c_nMaxThreadCount; n++)
		{
			HANDLE h = m_ContainedThread[n].StartThread(
				m_pContainer,
				Container_TaskProc);
			if(NULL == h)
			{
				if(SUCCEEDED(hr = HRESULT_FROM_WIN32(GetLastError())))
					hr = E_FAIL;
				break;
			}
		}
		return hr;
	}

	void EndThread(DWORD dwMilliseconds = INFINITE)
	{
		for (int n = 0; n < c_nMaxThreadCount; n++)
		{
			m_ContainedThread[n].EndThread(dwMilliseconds);
		}
	}

	void SetThreadExiting(void)
	{
		m_bExitThread = true;

		for (int n = 0; n < c_nMaxThreadCount; n++)
		{
			m_ContainedThread[n].SetThreadExiting();
		}
	}

	bool IsThreadExiting(void)
	{
		return m_bExitThread;
	}
};

#endif //__THREADPROC_H__
