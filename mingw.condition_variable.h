/**
* @file condition_variable.h
* @brief std::condition_variable implementation for MinGW
*
* (c) 2013-2016 by Mega Limited, Auckland, New Zealand
* @author Alexander Vassilev
*
* @copyright Simplified (2-clause) BSD License.
* You should have received a copy of the license along with this
* program.
*
* This code is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* @note
* This file may become part of the mingw-w64 runtime package. If/when this happens,
* the appropriate license will be added, i.e. this code will become dual-licensed,
* and the current BSD 2-clause license will stay.
*/

#ifndef MINGW_CONDITIONAL_VARIABLE_H
#define MINGW_CONDITIONAL_VARIABLE_H
#include <atomic>
#include <assert.h>
#include "mingw.mutex.h"
#include <chrono>
#include <system_error>
#include <windows.h>

namespace mingw_stdthread
{
enum class cv_status { no_timeout, timeout };
class condition_variable_any
{
protected:
    recursive_mutex mMutex;
    std::atomic<int> mNumWaiters;
    HANDLE mSemaphore;
    HANDLE mWakeEvent;
public:
    typedef HANDLE native_handle_type;
    native_handle_type native_handle() {return mSemaphore;}
    condition_variable_any(const condition_variable_any&) = delete;
    condition_variable_any& operator=(const condition_variable_any&) = delete;
    condition_variable_any()
        :mNumWaiters(0), mSemaphore(CreateSemaphore(NULL, 0, 0xFFFF, NULL)),
         mWakeEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
    {}
    ~condition_variable_any() {  CloseHandle(mWakeEvent); CloseHandle(mSemaphore);  }
protected:
    template <class M>
    bool wait_impl(M& lock, DWORD timeout)
    {
        {
            lock_guard<recursive_mutex> guard(mMutex);
            mNumWaiters++;
        }
        lock.unlock();
            DWORD ret = WaitForSingleObject(mSemaphore, timeout);

        mNumWaiters--;
        SetEvent(mWakeEvent);
        lock.lock();
        if (ret == WAIT_OBJECT_0)
            return true;
        else if (ret == WAIT_TIMEOUT)
            return false;
//2 possible cases:
//1)The point in notify_all() where we determine the count to
//increment the semaphore with has not been reached yet:
//we just need to decrement mNumWaiters, but setting the event does not hurt
//
//2)Semaphore has just been released with mNumWaiters just before
//we decremented it. This means that the semaphore count
//after all waiters finish won't be 0 - because not all waiters
//woke up by acquiring the semaphore - we woke up by a timeout.
//The notify_all() must handle this grafecully
//
        else
            throw std::system_error(EPROTO, std::generic_category());
    }
public:
    template <class M>
    void wait(M& lock)
    {
        wait_impl(lock, INFINITE);
    }
    template <class M, class Predicate>
    void wait(M& lock, Predicate pred)
    {
        while(!pred())
        {
            wait(lock);
        };
    }

    void notify_all() noexcept
    {
        lock_guard<recursive_mutex> lock(mMutex); //block any further wait requests until all current waiters are unblocked
        if (mNumWaiters.load() <= 0)
            return;

        ReleaseSemaphore(mSemaphore, mNumWaiters, NULL);
        while(mNumWaiters > 0)
        {
            auto ret = WaitForSingleObject(mWakeEvent, 1000);
            if (ret == WAIT_FAILED || ret == WAIT_ABANDONED)
                std::terminate();
        }
        assert(mNumWaiters == 0);
//in case some of the waiters timed out just after we released the
//semaphore by mNumWaiters, it won't be zero now, because not all waiters
//woke up by acquiring the semaphore. So we must zero the semaphore before
//we accept waiters for the next event
//See _wait_impl for details
        while(WaitForSingleObject(mSemaphore, 0) == WAIT_OBJECT_0);
    }
    void notify_one() noexcept
    {
        lock_guard<recursive_mutex> lock(mMutex);
        int targetWaiters = mNumWaiters.load() - 1;
        if (targetWaiters <= -1)
            return;
        ReleaseSemaphore(mSemaphore, 1, NULL);
        while(mNumWaiters > targetWaiters)
        {
            auto ret = WaitForSingleObject(mWakeEvent, 1000);
            if (ret == WAIT_FAILED || ret == WAIT_ABANDONED)
                std::terminate();
        }
        assert(mNumWaiters == targetWaiters);
    }
    template <class M, class Rep, class Period>
    cv_status wait_for(M& lock,
      const std::chrono::duration<Rep, Period>& rel_time)
    {
        long long timeout = std::chrono::duration_cast<std::chrono::milliseconds>(rel_time).count();
        if (timeout < 0)
            timeout = 0;
        bool ret = wait_impl(lock, (DWORD)timeout);
        return ret?cv_status::no_timeout:cv_status::timeout;
    }

    template <class M, class Rep, class Period, class Predicate>
    bool wait_for(M& lock,
       const std::chrono::duration<Rep, Period>& rel_time, Predicate pred)
    {
        return wait_until(lock, std::chrono::steady_clock::now()+rel_time, pred);
    }
    template <class M, class Clock, class Duration>
    cv_status wait_until (M& lock,
      const std::chrono::time_point<Clock,Duration>& abs_time)
    {
        return wait_for(lock, abs_time - Clock::now());
    }
    template <class M, class Clock, class Duration, class Predicate>
    bool wait_until (M& lock,
      const std::chrono::time_point<Clock, Duration>& abs_time,
      Predicate pred)
    {
        while (!pred())
        {
            if (wait_until(lock, abs_time) == cv_status::timeout)
            {
                return pred();
            }
        }
        return true;
    }
};
class condition_variable: protected condition_variable_any
{
protected:
    typedef condition_variable_any base;
public:
    using base::native_handle_type;
    using base::native_handle;
    using base::base;
    using base::notify_all;
    using base::notify_one;
    void wait(unique_lock<mutex> &lock)
    {       base::wait(lock);                               }
    template <class Predicate>
    void wait(unique_lock<mutex>& lock, Predicate pred)
    {       base::wait(lock, pred);                         }
    template <class Rep, class Period>
    cv_status wait_for(unique_lock<mutex>& lock, const std::chrono::duration<Rep, Period>& rel_time)
    {      return base::wait_for(lock, rel_time);           }
    template <class Rep, class Period, class Predicate>
    bool wait_for(unique_lock<mutex>& lock, const std::chrono::duration<Rep, Period>& rel_time, Predicate pred)
    {        return base::wait_for(lock, rel_time, pred);   }
    template <class Clock, class Duration>
    cv_status wait_until (unique_lock<mutex>& lock, const std::chrono::time_point<Clock,Duration>& abs_time)
    {        return base::wait_until(lock, abs_time);         }
    template <class Clock, class Duration, class Predicate>
    bool wait_until (unique_lock<mutex>& lock, const std::chrono::time_point<Clock, Duration>& abs_time, Predicate pred)
    {        return base::wait_until(lock, abs_time, pred); }
};

//  Contains only those objects that ought to be placed in std.
namespace visible
{
using mingw_stdthread::cv_status;
using mingw_stdthread::condition_variable;
using mingw_stdthread::condition_variable_any;
} //  Namespace mingw_stdthread::visible
} //  Namespace mingw_stdthread

//  Push objects into std, but only if they are not already there.
namespace std
{
using namespace mingw_stdthread::visible;
}
#endif // MINGW_CONDITIONAL_VARIABLE_H
