#ifndef MINGW_CONDITIONAL_VARIABLE_H
#define MINGW_CONDITIONAL_VARIABLE_H
#include <atomic>
#include <assert.h>
namespace std
{
class condition_variable
{
protected:
    recursive_mutex mMutex;
    HANDLE mSemaphore;
    HANDLE mWakeEvent;
    atomic<int> mNumWaiters;
public:
    typedef HANDLE native_handle_type;
    native_handle_type native_handle() {return mSemaphore;}
    condition_variable(const condition_variable&) = delete;
    condition_variable& operator=(const condition_variable&) = delete;
    condition_variable()
        :mNumWaiters(0), mSemaphore(CreateSemaphore(NULL, 0, 0xFFFF, NULL)),
         mWakeEvent(CreateEvent(NULL, TRUE, FALSE, NULL))
    {}
    ~condition_variable() {  CloseHandle(mWakeEvent); CloseHandle(mSemaphore);  }
    void wait(std::unique_lock<std::mutex>& lock)
    {
        {
            lock_guard<recursive_mutex> guard(mMutex);
            mNumWaiters++;
        }
        lock.unlock();
        DWORD ret = WaitForSingleObject(mSemaphore, INFINITE);
        if (ret != WAIT_OBJECT_0)
        {
            lock.lock();
            throw system_error(EPROTO, generic_category());
        }
        mNumWaiters--;
        SetEvent(mWakeEvent);
        lock.lock();
    }
    template <class Predicate>
    void wait(std::unique_lock<std::mutex> &lock, Predicate pred)
    {
        while(!pred())
        {
            wait(lock);
        };
    }

    void notify_all() noexcept
    {
        lock_guard<recursive_mutex> lock(mMutex);
        if (!mNumWaiters)
            return;

        ReleaseSemaphore(mSemaphore, mNumWaiters, NULL);
        while(mNumWaiters > 0)
        {
            auto ret = WaitForSingleObject(mWakeEvent, 1000);
            if ((ret == WAIT_FAILED) || (ret == WAIT_ABANDONED))
                throw system_error(EPROTO, generic_category());
        }
#ifndef NDEBUG
        assert(mNumWaiters == 0);
        long semCount;
        ReleaseSemaphore(mSemaphore, 0, &semCount);
        assert(semCount == 0xFFFF);
#endif
    }
    void notify_one() noexcept
    {
        lock_guard<recursive_mutex> lock(mMutex);
        if (!mNumWaiters)
            return;
        int targetWaiters = mNumWaiters.load() - 1;
        ReleaseSemaphore(mSemaphore, 1, NULL);
        while(mNumWaiters > targetWaiters)
        {
            auto ret = WaitForSingleObject(mWakeEvent, 1000);
            if ((ret == WAIT_FAILED) || (ret == WAIT_ABANDONED))
                throw system_error(EPROTO, generic_category());
        }
        assert(mNumWaiters == targetWaiters);
    }
};
}
#endif // MINGW_CONDITIONAL_VARIABLE_H
