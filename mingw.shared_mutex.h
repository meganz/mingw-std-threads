/// \file mingw.shared_mutex.h
/// \brief Standard-compliant shared mutex for MinGW
///
/// (c) 2017 by Nathaniel McClatchey, Athens OH, United States
/// \author Nathaniel J. McClatchey
///
/// \copyright Simplified (2-clause) BSD License.
///
/// \note This file may become part of the mingw-w64 runtime package. If/when this happens,
/// the appropriate license will be added, i.e. this code will become dual-licensed,
/// and the current BSD 2-clause license will stay.

#ifndef MINGW_SHARED_MUTEX_H_
#define MINGW_SHARED_MUTEX_H_

#if !defined(__cplusplus) || (__cplusplus < 201103L)
#error A C++11 compiler is required!
#endif

//    Even if MinGW has supplied the appropriate C++17 shared_mutex
//  implementation, users may wish to use the native Windows shared mutex, or to
//  use the lightweight atomics-based shared mutex.
#if (__cplusplus >= 201703L) && (!defined(__MINGW32__) || defined(_GLIBCXX_HAS_GTHREADS))
#include <shared_mutex>
#endif

#include <system_error>

//    Implementing a shared_mutex without OS support will require atomic read-
//  modify-write capacity.
#include <atomic>
//  For this_thread::yield
#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
#pragma message "Using native WIN32 threads for shared_mutex (" __FILE__ ")."
#include "mingw.thread.h"
#else
#include <thread>
#endif
//  For defer_lock_t, adopt_lock_t, and try_to_lock_t
#include <mutex>
//  For timing (in shared_lock and shared_timed_mutex)
#include <chrono>

//  Might be able to use native Slim Reader-Writer (SRW) locks.
#ifdef _WIN32
#include <windows.h>
#endif


#include <assert.h>

namespace mingw_stdthread
{
//  Define a portable atomics-based shared mutex
namespace portable
{
class shared_mutex
{
  typedef uint_fast16_t atomic_type;
  std::atomic<atomic_type> atomic_;
  static constexpr atomic_type kWriteBit = 1 << (sizeof(atomic_type) * CHAR_BIT - 1);
 public:
  typedef shared_mutex * native_handle_type;

  shared_mutex ()
    : atomic_(0)
  {
  }

//  No form of copying or moving should be allowed.
  shared_mutex (const shared_mutex&) = delete;
  shared_mutex & operator= (const shared_mutex&) = delete;

  ~shared_mutex ()
  {
    assert(atomic_.load(std::memory_order_relaxed) == 0);
  }

  void lock_shared (void)
  {
    atomic_type expected = atomic_.load(std::memory_order_relaxed);
//  If writing, add a delay. Otherwise, spin until the cmpxchg goes through.
    do {
      //if ((expected & kWriteBit) || ((expected + 1) == kWriteBit))
      if (expected >= kWriteBit - 1)
      {
        using namespace std::this_thread;
        yield();
        expected = atomic_.load(std::memory_order_relaxed);
        continue;
      }
      //expected &= ~kWriteBit;
      if (atomic_.compare_exchange_weak(expected, expected + 1,
                                        std::memory_order_acquire,
                                        std::memory_order_relaxed))
        break;
    } while (true);
  }

  bool try_lock_shared (void)
  {
    atomic_type expected = atomic_.load(std::memory_order_relaxed) & (~kWriteBit);
    if (expected + 1 == kWriteBit)
      return false;
    else
      return atomic_.compare_exchange_strong( expected, expected + 1,
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed);
  }

  void unlock_shared (void)
  {
    using namespace std;
#ifndef NDEBUG
    if (!(atomic_.fetch_sub(1, memory_order_release) & (~kWriteBit)))
      throw system_error(make_error_code(errc::operation_not_permitted));
#else
    atomic_.fetch_sub(1, memory_order_release);
#endif
  }

//  Behavior is undefined if a lock was previously acquired.
  void lock (void)
  {
    using namespace std::this_thread;
//  Might be able to use relaxed memory order...
//  Wait for the write-lock to be unlocked.
    while (atomic_.fetch_or(kWriteBit, std::memory_order_relaxed) & kWriteBit)
      yield();
//  Wait for readers to finish up.
    while (atomic_.load(std::memory_order_acquire) & (~kWriteBit))
      yield();
  }

  bool try_lock (void)
  {
    atomic_type expected = 0;
    return atomic_.compare_exchange_strong(expected, kWriteBit,
                                         std::memory_order_acquire,
                                         std::memory_order_relaxed);
  }

  void unlock (void)
  {
    using namespace std;
#ifndef NDEBUG
    if (atomic_.load(memory_order_relaxed) != kWriteBit)
      throw system_error(make_error_code(errc::operation_not_permitted));
#endif
    atomic_.store(0, memory_order_release);
  }

  native_handle_type native_handle (void)
  {
    return this;
  }
};

} //  Namespace portable

#ifdef _WIN32
namespace win32
{
//    The native shared_mutex implementation primarily uses features of Windows
//  Vista, but the features used for try_lock and try_lock_shared were not
//  introduced until Windows 7. To allow limited use while compiling for Vista,
//  I define the class without try_* functions in that case.
//    Only fully-featured implementations will be placed into namespace std.
#if (WINVER >= _WIN32_WINNT_VISTA)
namespace windows7
{
class shared_mutex
{
  SRWLOCK handle_;
 public:
  typedef PSRWLOCK native_handle_type;

  shared_mutex ()
    : handle_()
  {
    InitializeSRWLock(&handle_);
  }

  ~shared_mutex () = default;

//  No form of copying or moving should be allowed.
  shared_mutex (const shared_mutex&) = delete;
  shared_mutex & operator= (const shared_mutex&) = delete;

//  Behavior is undefined if a lock was previously acquired by this thread.
  void lock (void)
  {
    AcquireSRWLockExclusive(&handle_);
  }

  void lock_shared (void)
  {
    AcquireSRWLockShared(&handle_);
  }

  void unlock_shared (void)
  {
    ReleaseSRWLockShared(&handle_);
  }

  void unlock (void)
  {
    ReleaseSRWLockExclusive(&handle_);
  }


#if (WINVER >= _WIN32_WINNT_WIN7)
  bool try_lock_shared (void)
  {
    return TryAcquireSRWLockShared(&handle_) != 0;
  }

  bool try_lock (void)
  {
    return TryAcquireSRWLockExclusive(&handle_) != 0;
  }
#endif

  native_handle_type native_handle (void)
  {
    return &handle_;
  }
};

} //  Namespace windows7
#endif  //  Compiling for Vista
} //  Namespace win32
#endif  //  Compiling for Win32
} //  Namespace mingw_stdthread

namespace std
{
//    Though adding anything to the std namespace is generally frowned upon, the
//  added features are only those intended for inclusion in C++17
#if (__cplusplus < 201703L) || (defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS))
#if (defined(_WIN32) && (WINVER >= _WIN32_WINNT_WIN7))
  using ::mingw_stdthread::win32::windows7::shared_mutex;
#else
  using ::mingw_stdthread::portable::shared_mutex;
#endif
#endif

//    If not supplied by shared_mutex (eg. because C++17 is not supported), I
//  supply the various helper classes that the header should have defined.
#if (__cplusplus < 201402L) || (defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS))
class shared_timed_mutex : protected shared_mutex
{
  typedef shared_mutex Base;
 public:
  using Base::lock;
  using Base::try_lock;
  using Base::unlock;
  using Base::lock_shared;
  using Base::try_lock_shared;
  using Base::unlock_shared;

  template< class Clock, class Duration >
  bool try_lock_until ( const std::chrono::time_point<Clock,Duration>& cutoff )
  {
    do {
      if (try_lock())
        return true;
    } while (std::chrono::steady_clock::now() < cutoff);
    return false;
  }

  template< class Rep, class Period >
  bool try_lock_for (const std::chrono::duration<Rep,Period>& rel_time)
  {
    return try_lock_until(std::chrono::steady_clock::now() + rel_time);
  }

  template< class Clock, class Duration >
  bool try_lock_shared_until ( const std::chrono::time_point<Clock,Duration>& cutoff )
  {
    do {
      if (try_lock_shared())
        return true;
    } while (std::chrono::steady_clock::now() < cutoff);
    return false;
  }

  template< class Rep, class Period >
  bool try_lock_shared_for (const std::chrono::duration<Rep,Period>& rel_time)
  {
    return try_lock_shared_until(std::chrono::steady_clock::now() + rel_time);
  }
};

template<class Mutex>
class shared_lock
{
  Mutex * mutex_;
  bool owns_;
//  Reduce code redundancy
  void verify_lockable (void)
  {
    if (mutex_ == nullptr)
      throw system_error(make_error_code(errc::operation_not_permitted));
    if (owns_)
      throw system_error(make_error_code(errc::resource_deadlock_would_occur));
  }
 public:
  typedef Mutex mutex_type;

  shared_lock (void) noexcept
    : mutex_(nullptr), owns_(false)
  {
  }

  shared_lock (shared_lock<Mutex> && other) noexcept
    : mutex_(other.mutex_), owns_(other.owns_)
  {
    other.mutex_ = nullptr;
    other.owns_ = false;
  }

  explicit shared_lock (mutex_type & m)
    : mutex_(&m), owns_(true)
  {
    mutex_->lock_shared();
  }

  shared_lock (mutex_type & m, defer_lock_t) noexcept
    : mutex_(&m), owns_(false)
  {
  }

  shared_lock (mutex_type & m, adopt_lock_t)
    : mutex_(&m), owns_(true)
  {
  }

  shared_lock (mutex_type & m, try_to_lock_t)
    : mutex_(&m), owns_(m.try_lock_shared())
  {
  }

  template< class Rep, class Period >
  shared_lock( mutex_type& m, const chrono::duration<Rep,Period>& timeout_duration )
    : mutex_(&m), owns_(m.try_lock_shared_for(timeout_duration))
  {
  }

  template< class Clock, class Duration >
  shared_lock( mutex_type& m, const chrono::time_point<Clock,Duration>& timeout_time )
    : mutex_(&m), owns_(m.try_lock_shared_until(timeout_time))
  {
  }

  shared_lock& operator= (shared_lock<Mutex> && other) noexcept
  {
    if (&other != this)
    {
      if (owns_)
        mutex_->unlock_shared();
      mutex_ = other.mutex_;
      owns_ = other.owns_;
      other.mutex_ = nullptr;
      other.owns_ = false;
    }
    return *this;
  }


  ~shared_lock (void)
  {
    if (owns_)
      mutex_->unlock_shared();
  }

  shared_lock (const shared_lock<Mutex> &) = delete;
  shared_lock& operator= (const shared_lock<Mutex> &) = delete;

//  Shared locking
  void lock (void)
  {
    verify_lockable();
    mutex_->lock_shared();
  }

  bool try_lock (void)
  {
    verify_lockable();
    return mutex_->try_lock_shared();
  }

  template< class Clock, class Duration >
  bool try_lock_until( const chrono::time_point<Clock,Duration>& cutoff )
  {
    verify_lockable();
    do {
      if (mutex_->try_lock_shared())
        return true;
    } while (chrono::steady_clock::now() < cutoff);
    return false;
  }

  template< class Rep, class Period >
  bool try_lock_for (const chrono::duration<Rep,Period>& rel_time)
  {
    return try_lock_until(chrono::steady_clock::now() + rel_time);
  }

  void unlock (void)
  {
    if (!owns_)
      throw system_error(make_error_code(errc::operation_not_permitted));
    mutex_->unlock_shared();
    owns_ = false;
  }

//  Modifiers
  void swap (shared_lock<Mutex> & other) noexcept
  {
    swap(mutex_, other.mutex_);
    swap(owns_, other.owns_);
  }

  mutex_type * release (void) noexcept
  {
    mutex_type * ptr = mutex_;
    mutex_ = nullptr;
    owns_ = false;
    return ptr;
  }
//  Observers
  mutex_type * mutex (void) const noexcept
  {
    return mutex_;
  }

  bool owns_lock (void) const noexcept
  {
    return owns_;
  }

  explicit operator bool () const noexcept
  {
    return owns_lock();
  }
};

template< class Mutex >
void swap( shared_lock<Mutex>& lhs, shared_lock<Mutex>& rhs ) noexcept
{
  lhs.swap(rhs);
}
#endif
} //  Namespace std
#endif // MINGW_SHARED_MUTEX_H_
