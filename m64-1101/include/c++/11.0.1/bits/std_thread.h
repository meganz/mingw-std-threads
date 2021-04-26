// std::thread declarations -*- C++ -*-

// Copyright (C) 2008-2021 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file bits/std_thread.h
 *  This is an internal header file, included by other library headers.
 *  Do not attempt to use it directly. @headername{thread}
 */

#ifndef _GLIBCXX_THREAD_H
#define _GLIBCXX_THREAD_H 1

#pragma GCC system_header

#if __cplusplus >= 201103L
#include <bits/c++config.h>

#include <exception>		// std::terminate
#include <iosfwd>		// std::basic_ostream
#include <tuple>		// std::tuple

#include <bits/mingw.invoke.h>

#ifdef MINGWSTD
#include <cstddef>      //  For std::size_t
#include <cerrno>       //  Detect error type.
#include <exception>    //  For std::terminate
#include <system_error> //  For std::system_error
#include <functional>   //  For std::hash
#include <tuple>        //  For std::tuple
#include <chrono>       //  For sleep timing.
#include <memory>       //  For std::unique_ptr
#include <iosfwd>       //  Stream output for thread ids.
#include <utility>      //  For std::swap, std::forward

#if (defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR))
#pragma message "The Windows API that MinGW-w32 provides is not fully compatible\
 with Microsoft's API. We'll try to work around this, but we can make no\
 guarantees. This problem does not exist in MinGW-w64."
#include <windows.h>    //  No further granularity can be expected.
#else
#include <synchapi.h>   //  For WaitForSingleObject
#include <handleapi.h>  //  For CloseHandle, etc.
#include <sysinfoapi.h> //  For GetNativeSystemInfo
#include <processthreadsapi.h>  //  For GetCurrentThreadId
#endif
#include <process.h>  //  For _beginthreadex
#endif

#include <bits/functional_hash.h> // std::hash
#include <bits/invoke.h>	// std::__invoke
#include <bits/refwrap.h>       // not required, but helpful to users
#include <bits/unique_ptr.h>	// std::unique_ptr

#ifdef _GLIBCXX_HAS_GTHREADS
# include <bits/gthr.h>
#else
# include <errno.h>
# include <bits/functexcept.h>
#endif

#ifdef MINGWSTD
namespace detail
{
    template<std::size_t...>
    struct IntSeq {};

    template<std::size_t N, std::size_t... S>
    struct GenIntSeq : GenIntSeq<N-1, N-1, S...> { };

    template<std::size_t... S>
    struct GenIntSeq<0, S...> { typedef IntSeq<S...> type; };

//    Use a template specialization to avoid relying on compiler optimization
//  when determining the parameter integer sequence.
    template<class Func, class T, typename... Args>
    class ThreadFuncCall;
// We can't define the Call struct in the function - the standard forbids template methods in that case
    template<class Func, std::size_t... S, typename... Args>
    class ThreadFuncCall<Func, detail::IntSeq<S...>, Args...>
    {
        static_assert(sizeof...(S) == sizeof...(Args), "Args must match.");
        using Tuple = std::tuple<typename std::decay<Args>::type...>;
        typename std::decay<Func>::type mFunc;
        Tuple mArgs;

    public:
        ThreadFuncCall(Func&& aFunc, Args&&... aArgs)
          : mFunc(std::forward<Func>(aFunc)),
            mArgs(std::forward<Args>(aArgs)...)
        {
        }

        void callFunc()
        {
            std::__invoke(std::move(mFunc), std::move(std::get<S>(mArgs)) ...);
        }
    };

//  Allow construction of threads without exposing implementation.
    class ThreadIdTool;
} //  Namespace "detail"
#endif

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

  /** @addtogroup threads
   *  @{
   */

  /// thread
  class thread
  {
  public:
#ifdef _GLIBCXX_HAS_GTHREADS
    // Abstract base class for types that wrap arbitrary functors to be
    // invoked in the new thread of execution.
    struct _State
    {
      virtual ~_State();
      virtual void _M_run() = 0;
    };
    using _State_ptr = unique_ptr<_State>;

    using native_handle_type = __gthread_t;
#else
#ifdef MINGWSTD
    typedef HANDLE native_handle_type;
#else
    using native_handle_type = int;
#endif
#endif

    /// thread::id
    class id
    {
#ifdef MINGWSTD
      DWORD _M_thread = 0;
#else
      native_handle_type	_M_thread;
#endif

#ifdef MINGWSTD
    public:
      id (void) noexcept = default;

      explicit
      id(DWORD aId) noexcept : _M_thread(aId){}
#else
    public:
      id() noexcept : _M_thread() { }

      explicit
      id(native_handle_type __id) : _M_thread(__id) { }
#endif

    private:
      friend class thread;
      friend struct hash<id>;
#ifdef MINGWSTD
      friend class detail::ThreadIdTool;

      friend bool
      operator==(id __x, id __y) noexcept {return __x._M_thread == __y._M_thread; }
      friend bool
      operator!=(id __x, id __y) noexcept {return __x._M_thread != __y._M_thread; }
      friend bool
      operator< (id __x, id __y) noexcept {return __x._M_thread <  __y._M_thread; }
      friend bool
      operator<=(id __x, id __y) noexcept {return __x._M_thread <= __y._M_thread; }
      friend bool
      operator> (id __x, id __y) noexcept {return __x._M_thread >  __y._M_thread; }
      friend bool
      operator>=(id __x, id __y) noexcept {return __x._M_thread >= __y._M_thread; }
#else
      friend bool
      operator==(id __x, id __y) noexcept;

#if __cpp_lib_three_way_comparison
      friend strong_ordering
      operator<=>(id __x, id __y) noexcept;
#else
      friend bool
      operator<(id __x, id __y) noexcept;
#endif
#endif

      template<class _CharT, class _Traits>
      friend basic_ostream<_CharT, _Traits>&
      operator<<(basic_ostream<_CharT, _Traits>& __out, id __id)
#ifdef MINGWSTD
      {
          if (__id._M_thread == 0)
          {
              return __out << "(invalid std::thread::id)";
          }
          else
          {
              return __out << __id._M_thread;
          }
      }
#else
      ;
#endif
    };

  private:
#ifdef MINGWSTD
    static constexpr HANDLE kInvalidHandle = nullptr;
    static constexpr DWORD kInfinite = 0xffffffffl;
    HANDLE mHandle;
    id _M_id;

    template <class Call>
    static unsigned __stdcall threadfunc(void* arg)
    {
        std::unique_ptr<Call> call(static_cast<Call*>(arg));
        call->callFunc();
        return 0;
    }

    static unsigned int _hardware_concurrency_helper() noexcept
    {
        SYSTEM_INFO sysinfo;
//    This is one of the few functions used by the library which has a nearly-
//  equivalent function defined in earlier versions of Windows. Include the
//  workaround, just as a reminder that it does exist.
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0501)
        ::GetNativeSystemInfo(&sysinfo);
#else
        ::GetSystemInfo(&sysinfo);
#endif
        return sysinfo.dwNumberOfProcessors;
    }
#else
    id				_M_id;
#endif

    // _GLIBCXX_RESOLVE_LIB_DEFECTS
    // 2097.  packaged_task constructors should be constrained
    // 3039. Unnecessary decay in thread and packaged_task
    template<typename _Tp>
      using __not_same = __not_<is_same<__remove_cvref_t<_Tp>, thread>>;

  public:
#ifdef MINGWSTD
    thread(): mHandle(kInvalidHandle), _M_id(){}
#else
    thread() noexcept = default;
#endif

#ifdef _GLIBCXX_HAS_GTHREADS
    template<typename _Callable, typename... _Args,
	     typename = _Require<__not_same<_Callable>>>
      explicit
      thread(_Callable&& __f, _Args&&... __args)
      {
	static_assert( __is_invocable<typename decay<_Callable>::type,
				      typename decay<_Args>::type...>::value,
	  "std::thread arguments must be invocable after conversion to rvalues"
	  );

#ifdef GTHR_ACTIVE_PROXY
	// Create a reference to pthread_create, not just the gthr weak symbol.
	auto __depend = reinterpret_cast<void(*)()>(&pthread_create);
#else
	auto __depend = nullptr;
#endif
	using _Wrapper = _Call_wrapper<_Callable, _Args...>;
	// Create a call wrapper with DECAY_COPY(__f) as its target object
	// and DECAY_COPY(__args)... as its bound argument entities.
	_M_start_thread(_State_ptr(new _State_impl<_Wrapper>(
	      std::forward<_Callable>(__f), std::forward<_Args>(__args)...)),
	    __depend);
      }
#endif // _GLIBCXX_HAS_GTHREADS

    ~thread()
    {
      if (joinable())
#ifndef NDEBUG
            std::printf("Error: Must join() or detach() a thread before \
destroying it.\n");
#endif
	std::terminate();
    }

#ifdef MINGWSTD
    thread(thread&& other)
    :mHandle(other.mHandle), _M_id(other._M_id)
    {
        other.mHandle = kInvalidHandle;
        other._M_id = id{};
    }

    thread(const thread &other)=delete;
#else
    thread(const thread&) = delete;

    thread(thread&& __t) noexcept
    { swap(__t); }
#endif

#ifdef MINGWSTD
    template<class Func, typename... Args>
    explicit thread(Func&& func, Args&&... args) : mHandle(), _M_id()
    {
        using ArgSequence = typename detail::GenIntSeq<sizeof...(Args)>::type;
        using Call = detail::ThreadFuncCall<Func, ArgSequence, Args...>;
        auto call = new Call(
            std::forward<Func>(func), std::forward<Args>(args)...);
        unsigned id_receiver;
        auto int_handle = _beginthreadex(NULL, 0, threadfunc<Call>,
            static_cast<LPVOID>(call), 0, &id_receiver);
        if (int_handle == 0)
        {
            mHandle = kInvalidHandle;
            int errnum = errno;
            delete call;
//  Note: Should only throw EINVAL, EAGAIN, EACCES
            throw std::system_error(errnum, std::generic_category());
        } else {
            _M_id._M_thread = id_receiver;
            mHandle = reinterpret_cast<HANDLE>(int_handle);
        }
    }
#endif

    thread& operator=(const thread&) = delete;

#ifdef MINGWSTD
    thread& operator=(thread&& other) noexcept
#else
    thread& operator=(thread&& __t) noexcept
#endif
    {
      if (joinable())
#ifndef NDEBUG
            std::printf("Error: Must join() or detach() a thread before \
moving another thread to it.\n");
#endif
	  std::terminate();
#ifdef MINGWSTD
      swap(std::forward<thread>(other));
#else
      swap(__t);
#endif
      return *this;
    }

    void
#ifdef MINGWSTD
    swap(thread&& other) noexcept
    {
        std::swap(mHandle, other.mHandle);
        std::swap(_M_id._M_thread, other._M_id._M_thread);
    }
#else
    swap(thread& __t) noexcept
    { std::swap(_M_id, __t._M_id); }
#endif

#ifdef MINGWSTD
    bool
    joinable() const
    {return mHandle != kInvalidHandle;}
#else
    bool
    joinable() const noexcept
    { return !(_M_id == id()); }
#endif


//    Note: Due to lack of synchronization, this function has a race condition
//  if called concurrently, which leads to undefined behavior. The same applies
//  to all other member functions of this class, but this one is mentioned
//  explicitly.

    void
    join()
#ifdef MINGWSTD
    {
        using namespace std;
        if (get_id() == id(GetCurrentThreadId()))
            throw system_error(make_error_code(errc::resource_deadlock_would_occur));
        if (mHandle == kInvalidHandle)
            throw system_error(make_error_code(errc::no_such_process));
        if (!joinable())
            throw system_error(make_error_code(errc::invalid_argument));
        WaitForSingleObject(mHandle, kInfinite);
        CloseHandle(mHandle);
        mHandle = kInvalidHandle;
        _M_id = id{};
    }
#else
    ;
#endif


    void
    detach()
#ifdef MINGWSTD
    {
        if (!joinable())
        {
            using namespace std;
            throw system_error(make_error_code(errc::invalid_argument));
        }
        if (mHandle != kInvalidHandle)
        {
            CloseHandle(mHandle);
            mHandle = kInvalidHandle;
        }
        _M_id = id{};
    }
#else
    ;
#endif

    id
    get_id() const noexcept
    { return _M_id; }

    /** @pre thread is joinable
     */
    native_handle_type
    native_handle()
#ifdef MINGWSTD
    const { return mHandle; }
#else
    { return _M_id._M_thread; }
#endif

    // Returns a value that hints at the number of hardware thread contexts.
    static unsigned int
    hardware_concurrency() noexcept
#ifdef MINGWSTD
    {
        static unsigned int cached = _hardware_concurrency_helper();
        return cached;
    }
#else
    ;
#endif

#ifdef _GLIBCXX_HAS_GTHREADS
  private:
    template<typename _Callable>
      struct _State_impl : public _State
      {
	_Callable		_M_func;

	template<typename... _Args>
	  _State_impl(_Args&&... __args)
	  : _M_func{{std::forward<_Args>(__args)...}}
	  { }

	void
	_M_run() { _M_func(); }
      };

    void
    _M_start_thread(_State_ptr, void (*)());

#if _GLIBCXX_THREAD_ABI_COMPAT
  public:
    struct _Impl_base;
    typedef shared_ptr<_Impl_base>	__shared_base_type;
    struct _Impl_base
    {
      __shared_base_type	_M_this_ptr;
      virtual ~_Impl_base() = default;
      virtual void _M_run() = 0;
    };

  private:
    void
    _M_start_thread(__shared_base_type, void (*)());

    void
    _M_start_thread(__shared_base_type);
#endif

  private:
    // A call wrapper that does INVOKE(forwarded tuple elements...)
    template<typename _Tuple>
      struct _Invoker
      {
	_Tuple _M_t;

	template<typename>
	  struct __result;
	template<typename _Fn, typename... _Args>
	  struct __result<tuple<_Fn, _Args...>>
	  : __invoke_result<_Fn, _Args...>
	  { };

	template<size_t... _Ind>
	  typename __result<_Tuple>::type
	  _M_invoke(_Index_tuple<_Ind...>)
	  { return std::__invoke(std::get<_Ind>(std::move(_M_t))...); }

	typename __result<_Tuple>::type
	operator()()
	{
	  using _Indices
	    = typename _Build_index_tuple<tuple_size<_Tuple>::value>::__type;
	  return _M_invoke(_Indices());
	}
      };

  public:
    template<typename... _Tp>
      using _Call_wrapper = _Invoker<tuple<typename decay<_Tp>::type...>>;
#endif // _GLIBCXX_HAS_GTHREADS
  };

#ifndef MINGWSTD
#ifndef _GLIBCXX_HAS_GTHREADS
  inline void thread::join() { std::__throw_system_error(EINVAL); }
  inline void thread::detach() { std::__throw_system_error(EINVAL); }
  inline unsigned int thread::hardware_concurrency() { return 0; }
#endif

  inline void
  swap(thread& __x, thread& __y) noexcept
  { __x.swap(__y); }

  inline bool
  operator==(thread::id __x, thread::id __y) noexcept
  {
    // pthread_equal is undefined if either thread ID is not valid, so we
    // can't safely use __gthread_equal on default-constructed values (nor
    // the non-zero value returned by this_thread::get_id() for
    // single-threaded programs using GNU libc). Assume EqualityComparable.
    return __x._M_thread == __y._M_thread;
  }
#endif

  // N.B. other comparison operators are defined in <thread>

  // DR 889.
  /// std::hash specialization for thread::id.
  template<>
    struct hash<thread::id>
    : public __hash_base<size_t, thread::id>
    {
/*#ifdef MINGWSTD
      typedef thread::id argument_type;
      typedef size_t result_type;
      size_t
      operator()(const argument_type& __id) const noexcept
      { return __id._M_thread; }
#else*/
      size_t
      operator()(const thread::id& __id) const noexcept
      { return std::_Hash_impl::hash(__id._M_thread); }
//#endif
    };

namespace detail
{
    class ThreadIdTool
    {
    public:
        static thread::id make_id (DWORD base_id) noexcept
        {
            return thread::id(base_id);
        }
    };
} //  Namespace "detail"

  namespace this_thread
  {
    /// this_thread::get_id
    inline thread::id
    get_id() noexcept
    {
#ifndef _GLIBCXX_HAS_GTHREADS
      return thread::id(1);
#elif defined _GLIBCXX_NATIVE_THREAD_ID
      return thread::id(_GLIBCXX_NATIVE_THREAD_ID);
#elif defined MINGWSTD
      return detail::ThreadIdTool::make_id(GetCurrentThreadId());
#else
      return thread::id(__gthread_self());
#endif
    }

    /// this_thread::yield
    inline void
    yield() noexcept
    {
#if defined _GLIBCXX_HAS_GTHREADS && defined _GLIBCXX_USE_SCHED_YIELD
      __gthread_yield();
#endif
#ifdef MINGWSTD
      Sleep(0);
#endif
    }
#ifdef MINGWSTD
    template< class Rep, class Period >
    void sleep_for( const std::chrono::duration<Rep,Period>& sleep_duration)
    {
        static constexpr DWORD kInfinite = 0xffffffffl;
        using namespace std::chrono;
        using rep = milliseconds::rep;
        rep ms = duration_cast<milliseconds>(sleep_duration).count();
        while (ms > 0)
        {
            constexpr rep kMaxRep = static_cast<rep>(kInfinite - 1);
            auto sleepTime = (ms < kMaxRep) ? ms : kMaxRep;
            Sleep(static_cast<DWORD>(sleepTime));
            ms -= sleepTime;
        }
    }
    template <class Clock, class Duration>
    void sleep_until(const std::chrono::time_point<Clock,Duration>& sleep_time)
    {
        sleep_for(sleep_time-Clock::now());
    }
#endif
  } // namespace this_thread

  /// @}

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace
#endif // C++11

#endif // _GLIBCXX_THREAD_H
