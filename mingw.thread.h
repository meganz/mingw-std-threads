/**
* @file mingw.thread.h
* @brief std::thread implementation for MinGW
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

#ifndef WIN32STDTHREAD_H
#define WIN32STDTHREAD_H

#include <windows.h>
#include <functional>
#include <memory>
#include <chrono>
#include <system_error>
#include <cerrno>
#include <process.h>
#include <utility>
#include <ostream>

#if !defined(__MINGW32__) || defined(_GLIBCXX_HAS_GTHREADS)
#include <thread>
#endif

//instead of INVALID_HANDLE_VALUE _beginthreadex returns 0
#define _STD_THREAD_INVALID_HANDLE nullptr
namespace mingw_stdthread
{
namespace xp
{
class thread
{
public:
    class id
    {
        DWORD mId;
        void clear() {mId = 0;}
        friend class thread;
        friend class std::hash<id>;
    public:
        id() noexcept : mId(0) {}
        explicit id(DWORD aId) noexcept : mId(aId) {}
        bool operator==(const id& other) const noexcept {return mId == other.mId;}
        bool operator<(const id& other) const noexcept {return mId < other.mId;}
        bool operator!=(const id& other) const noexcept {return mId != other.mId;}
        bool operator>(const id& other) const noexcept {return mId > other.mId;}
        bool operator<=(const id& other) const noexcept {return mId <= other.mId;}
        bool operator>=(const id& other) const noexcept {return mId >= other.mId;}
    };
protected:
    HANDLE mHandle;
    id mThreadId;
public:
    typedef HANDLE native_handle_type;
    id get_id() const noexcept {return mThreadId;}
    native_handle_type native_handle() const {return mHandle;}
    thread(): mHandle(_STD_THREAD_INVALID_HANDLE), mThreadId() {}

    thread(thread&& other)
    :mHandle(other.mHandle), mThreadId(other.mThreadId)
    {
        other.mHandle = _STD_THREAD_INVALID_HANDLE;
        other.mThreadId.clear();
    }

    thread(const thread &other)=delete;

    template<class Function, class... Args>
    explicit thread(Function&& f, Args&&... args)
      : mHandle(_STD_THREAD_INVALID_HANDLE), mThreadId()
    {
        typedef decltype(std::bind(f, args...)) Call;
        Call* call = new Call(std::bind(f, args...));
        mHandle = (HANDLE)_beginthreadex(nullptr, 0, threadfunc<Call>,
            (LPVOID)call, 0, (unsigned*)&(mThreadId.mId));
        if (mHandle == _STD_THREAD_INVALID_HANDLE)
        {
            int errnum = errno;
            delete call;
            throw std::system_error(errnum, std::generic_category());
        }
    }
    template <class Call>
    static unsigned int __stdcall threadfunc(void* arg)
    {
        std::unique_ptr<Call> upCall(static_cast<Call*>(arg));
        (*upCall)();
        return (unsigned long)0;
    }
    bool joinable() const {return mHandle != _STD_THREAD_INVALID_HANDLE;}
    void join()
    {
        if (get_id() == id(GetCurrentThreadId()))
            throw std::system_error(EDEADLK, std::generic_category());
        if (mHandle == _STD_THREAD_INVALID_HANDLE)
            throw std::system_error(ESRCH, std::generic_category());
        if (!joinable())
            throw std::system_error(EINVAL, std::generic_category());
        WaitForSingleObject(mHandle, INFINITE);
        CloseHandle(mHandle);
        mHandle = _STD_THREAD_INVALID_HANDLE;
        mThreadId.clear();
    }

    ~thread()
    {
        if (joinable())
            std::terminate();
    }
    thread& operator=(const thread&) = delete;
    thread& operator=(thread&& other) noexcept
    {
        if (joinable())
          std::terminate();
        swap(other);
        return *this;
    }
    void swap(thread& other) noexcept
    {
        std::swap(mHandle, other.mHandle);
        std::swap(mThreadId.mId, other.mThreadId.mId);
    }
    static unsigned int hardware_concurrency() noexcept
    {
        static int ncpus = -1;
        if (ncpus == -1)
        {
            SYSTEM_INFO sysinfo;
            GetSystemInfo(&sysinfo);
            ncpus = sysinfo.dwNumberOfProcessors;
        }
        return ncpus;
    }
    void detach()
    {
        if (!joinable())
            throw std::system_error(EINVAL, std::generic_category());
        if (mHandle != _STD_THREAD_INVALID_HANDLE)
        {
            CloseHandle(mHandle);
            mHandle = _STD_THREAD_INVALID_HANDLE;
        }
        mThreadId.clear();
    }
};
} //  Namespace xp
using xp::thread;
#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
} //  Namespace mingw_stdthread

namespace std
{
using ::mingw_stdthread::thread;
#else
#pragma message("This version of MinGW seems to include a win32 port of pthreads\
, and probably already has C++11 std threading classes implemented, based on\
 pthreads. If you would still like to use this implementation (as it is more\
 lightweight), you can use the classes in namespace mingw_stdthread, rather than\
 those in std.")
#endif

namespace this_thread
{
  inline thread::id get_id()
  {
//    If, for some reason (such as a programmer editing this file), std::thread
//  and ::mingw_stdthread::xp::thread are different, this will emit a
//  compiler error, rather than allowing incorrect behavior.
    return ::mingw_stdthread::xp::thread::id(GetCurrentThreadId());
  }
  inline void yield() {
    Sleep(0);
  }
  template< class Rep, class Period >
  void sleep_for( const std::chrono::duration<Rep,Period>& sleep_duration)
  {
    Sleep(std::chrono::duration_cast<std::chrono::milliseconds>(sleep_duration).count());
  }
  template <class Clock, class Duration>
  void sleep_until(const std::chrono::time_point<Clock,Duration>& sleep_time)
  {
    sleep_for(sleep_time-Clock::now());
  }
}
}

namespace std
{
//  Specialization of templates is allowed in namespace std.
template<>
struct hash<typename ::mingw_stdthread::xp::thread::id>
{
  typedef typename ::mingw_stdthread::xp::thread::id argument_type;
  typedef size_t result_type;
  size_t operator() (const argument_type & i) const noexcept
  {
    return i.mId;
  }
};
} //  Namespace std

template< class CharT, class Traits >
std::basic_ostream<CharT,Traits>&
    operator<<( std::basic_ostream<CharT,Traits>& ost,
               typename mingw_stdthread::xp::thread::id id )
{
  std::hash<typename mingw_stdthread::xp::thread::id> hasher;
  ost << hasher(id);
  return ost;
}
#endif // WIN32STDTHREAD_H
