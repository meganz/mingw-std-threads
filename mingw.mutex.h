#ifndef WIN32STDMUTEX_H
#define WIN32STDMUTEX_H
namespace std
{
class recursive_mutex
{
protected:
    CRITICAL_SECTION mHandle;
public:
    typedef LPCRITICAL_SECTION native_handle_type;
    native_handle_type native_handle() {return &mHandle;}
    recursive_mutex() noexcept
    {
        InitializeCriticalSection(&mHandle);
    }
    recursive_mutex (const recursive_mutex&) = delete;
    ~recursive_mutex() noexcept
    {
        DeleteCriticalSection(&mHandle);
    }
    void lock()
    {
        EnterCriticalSection(&mHandle);
    }
    void unlock()
    {
        LeaveCriticalSection(&mHandle);
    }
    bool try_lock()
    {
        return (TryEnterCriticalSection(&mHandle)!=0);
    }
};

class mutex: protected recursive_mutex
{
protected:
    typedef recursive_mutex base;
#ifndef NDEBUG
    DWORD mOwnerThread;
#endif
public:
    using base::native_handle_type;
    using base::native_handle;
    mutex() noexcept :base()
#ifndef NDEBUG
    , mOwnerThread(0)
#endif
    {}
    mutex (const mutex&) = delete;
    void lock()
    {
        base::lock();
#ifndef NDEBUG
        DWORD self = GetCurrentThreadId();
        if (mOwnerThread == self)
            throw system_error(EDEADLK, generic_category());
        mOwnerThread = self;
#endif
    }
    void unlock()
    {
#ifndef NDEBUG
        if (mOwnerThread != GetCurrentThreadId())
            throw system_error(EDEADLK, generic_category());
        mOwnerThread = 0;
#endif
        base::unlock();
    }
    bool try_lock()
    {
        bool ret = base::try_lock();
#ifndef NDEBUG
        if (ret)
        {
            DWORD self = GetCurrentThreadId();
            if (mOwnerThread == self)
                throw system_error(EDEADLK, generic_category());
            mOwnerThread = self;
        }
#endif
        return ret;
    }
};

class recursive_timed_mutex
{
protected:
    HANDLE mHandle;
public:
    typedef HANDLE native_handle_type;
    native_handle_type native_handle() const {return mHandle;}
    recursive_timed_mutex(const recursive_timed_mutex&) = delete;
    recursive_timed_mutex(): mHandle(CreateMutex(NULL, FALSE, NULL)){}
    ~recursive_timed_mutex()
    {
        CloseHandle(mHandle);
    }
    void lock()
    {
        DWORD ret = WaitForSingleObject(mHandle, INFINITE);
        if (ret != WAIT_OBJECT_0)
        {
            if (ret == WAIT_ABANDONED)
                throw system_error(EOWNERDEAD, generic_category());
            else
                throw system_error(EPROTO, generic_category());
        }
    }
    void unlock()
    {
        if (!ReleaseMutex(mHandle))
            throw system_error(EDEADLK, generic_category());
    }
    bool try_lock()
    {
        DWORD ret = WaitForSingleObject(mHandle, 0);
        if (ret == WAIT_TIMEOUT)
            return false;
        else if (ret == WAIT_OBJECT_0)
            return true;
        else if (ret == WAIT_ABANDONED)
            throw system_error(EOWNERDEAD, generic_category());
        else
            throw system_error(EPROTO, generic_category());
    }
    template <class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep,Period>& dur)
    {
        DWORD timeout = (DWORD)chrono::duration_cast<chrono::milliseconds>(dur).count();

        DWORD ret = WaitForSingleObject(mHandle, timeout);
        if (ret == WAIT_TIMEOUT)
            return false;
        else if (ret == WAIT_OBJECT_0)
            return true;
        else if (ret == WAIT_ABANDONED)
            throw system_error(EOWNERDEAD, generic_category());
        else
            throw system_error(EPROTO, generic_category());
    }
    template <class Clock, class Duration>
    bool try_lock_until(const std::chrono::time_point<Clock,Duration>& timeout_time)
    {
        return try_lock_for(timeout_time - Clock::now());
    }
};
}
#endif // WIN32STDMUTEX_H
