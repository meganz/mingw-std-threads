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
#include <ostream>
#include <process.h>
#include <ostream>

//instead of INVALID_HANDLE_VALUE _beginthreadex returns 0
#define _STD_THREAD_INVALID_HANDLE 0
namespace mingw_stdthread
{
namespace detail
{
    template<int...>
    struct IntSeq {};

    template<int N, int... S>
    struct GenIntSeq : GenIntSeq<N-1, N-1, S...> { };

    template<int... S>
    struct GenIntSeq<0, S...> { typedef IntSeq<S...> type; };

    // We can't define the Call struct in the function - the standard forbids template methods in that case
    template<class Func, typename... Args>
    struct ThreadFuncCall
    {
      typedef std::tuple<Args...> Tuple;
      Tuple mArgs;
      Func mFunc;
      ThreadFuncCall(Func&& aFunc, Args&&... aArgs)
      :mFunc(std::forward<Func>(aFunc)), mArgs(std::forward<Args>(aArgs)...){}
      template <int... S>
      void callFunc(detail::IntSeq<S...>)
      {
          mFunc(std::get<S>(std::forward<Tuple>(mArgs)) ...);
      }
    };

}

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
        explicit id(DWORD aId=0) noexcept : mId(aId){}
        friend bool operator==(id x, id y) noexcept {return x.mId == y.mId; }
        friend bool operator!=(id x, id y) noexcept {return x.mId != y.mId; }
        friend bool operator< (id x, id y) noexcept {return x.mId <  y.mId; }
        friend bool operator<=(id x, id y) noexcept {return x.mId <= y.mId; }
        friend bool operator> (id x, id y) noexcept {return x.mId >  y.mId; }
        friend bool operator>=(id x, id y) noexcept {return x.mId >= y.mId; }

        template<class _CharT, class _Traits>
        friend std::basic_ostream<_CharT, _Traits>&
        operator<<(std::basic_ostream<_CharT, _Traits>& __out, id __id)
        {
            if (__id.mId == 0)
            {
                return __out << "(invalid std::thread::id)";
            }
            else
            {
                return __out << __id.mId;
            }
        }
    };
protected:
    HANDLE mHandle;
    id mThreadId;
public:
    typedef HANDLE native_handle_type;
    id get_id() const noexcept {return mThreadId;}
    native_handle_type native_handle() const {return mHandle;}
    thread(): mHandle(_STD_THREAD_INVALID_HANDLE){}

    thread(thread&& other)
    :mHandle(other.mHandle), mThreadId(other.mThreadId)
    {
        other.mHandle = _STD_THREAD_INVALID_HANDLE;
        other.mThreadId.clear();
    }

    thread(const thread &other)=delete;

    template<class Func, typename... Args>
    explicit thread(Func&& func, Args&&... args)
    {
        typedef detail::ThreadFuncCall<Func, Args...> Call;
        auto call = new Call(
            std::forward<Func>(func), std::forward<Args>(args)...);
        mHandle = (HANDLE)_beginthreadex(NULL, 0, threadfunc<Call, Args...>,
            (LPVOID)call, 0, (unsigned*)&(mThreadId.mId));
        if (mHandle == _STD_THREAD_INVALID_HANDLE)
        {
            int errnum = errno;
            delete call;
            throw std::system_error(errnum, std::generic_category());
        }
    }
    template <class Call, typename... Args>
    static unsigned __stdcall threadfunc(void* arg)
    {
        std::unique_ptr<Call> call(static_cast<Call*>(arg));
        call->callFunc(typename detail::GenIntSeq<sizeof...(Args)>::type());
        return 0;
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
        swap(std::forward<thread>(other));
        return *this;
    }
    void swap(thread&& other) noexcept
    {
        std::swap(mHandle, other.mHandle);
        std::swap(mThreadId.mId, other.mThreadId.mId);
    }

    static unsigned int _hardware_concurrency_helper() noexcept
    {
        SYSTEM_INFO sysinfo;
        ::GetSystemInfo(&sysinfo);
        return sysinfo.dwNumberOfProcessors;
    }

    static unsigned int hardware_concurrency() noexcept
    {
        static unsigned int cached = _hardware_concurrency_helper();
        return cached;
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

//  Contains only those objects that ought to be placed in std.
namespace visible
{
using mingw_stdthread::thread;
namespace this_thread
{
    inline thread::id get_id() noexcept {return thread::id(GetCurrentThreadId());}
    inline void yield() noexcept {Sleep(0);}
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
} //  Namespace mingw_stdthread::visible
using namespace visible;
} //  Namespace mingw_stdthread

namespace std
{
//  Push objects into std, but only if they are not already there.
using namespace mingw_stdthread::visible;
//    Specialize hash for this implementation's thread::id, even if the
//  std::thread::id already has a hash.
template<>
struct hash<mingw_stdthread::thread::id>
{
    typedef mingw_stdthread::thread::id argument_type;
    typedef size_t result_type;
    size_t operator() (const argument_type & i) const noexcept
    {
        return i.mId;
    }
};
}
#endif // WIN32STDTHREAD_H
