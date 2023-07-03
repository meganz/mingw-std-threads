/// \file mingw.latch.h
/// \brief std::latch implementation for MinGW.
///
/// (c) 2023 by Mega Limited, Auckland, New Zealand
/// \author Edoardo Sanguineti
///
/// \copyright Simplified (2-clause) BSD License.
///
/// \note This file may become part of the mingw-w64 runtime package. If/when
/// this happens, the appropriate license will be added, i.e. this code will
/// become dual-licensed, and the current BSD 2-clause license will stay.

//  Notes on the namespaces:
//  - The implementation can be accessed directly in the namespace
//    mingw_stdthread.
//  - Objects will be brought into namespace std by a using directive. This
//    will cause objects declared in std (such as MinGW's implementation) to
//    hide this implementation's definitions.
//  The end result is that if MinGW supplies an object, it is automatically
//  used. If MinGW does not supply an object, this implementation's version will
//  instead be used.

#ifndef MINGW_LATCH_H_
#define MINGW_LATCH_H_

#if !defined(__cplusplus) || (__cplusplus < 202002L)
#error The contents of <latch> require a C++20 compiler!
#endif

#include <sdkddkver.h>    //  Detect Windows version
#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0501)
#error To use the MinGW-std-threads library, you will need to define the macro _WIN32_WINNT to be 0x0501 (Windows XP) or higher.
#endif

#include <atomic>
#include <cassert>        // for descriptive errors
#include <cstddef>        // for std::ptrdiff_t
#include <limits>         // for std::numeric_limits

namespace mingw_stdthread
{
class latch
{
public:
    [[nodiscard]] static constexpr std::ptrdiff_t max() noexcept
    {
        return std::numeric_limits<std::ptrdiff_t>::max();
    }

    constexpr explicit latch(std::ptrdiff_t expected) : mCounter{expected}
    {
    }

    ~latch()=default;
    latch(const latch&)=delete;
    latch& operator=(const latch&)=delete;

    void count_down(std::ptrdiff_t update = 1 )
    {
        assert(update >= 0);

        const auto current = mCounter.fetch_sub(update, std::memory_order_release) - update;
        if (current == 0)
        {
            mCounter.notify_all();
        }
    }

    [[nodiscard]] bool try_wait() const noexcept
    {
        return 0 == mCounter.load(std::memory_order_acquire);
    }

    void wait() const
    {
        const auto current = mCounter.load(std::memory_order_acquire);
        if (current == 0)
        {
            return;
        }

        mCounter.wait(current, std::memory_order_relaxed);
    }

    void arrive_and_wait(const std::ptrdiff_t update = 1) noexcept
    {
        assert(update >= 0);

        const auto current = mCounter.fetch_sub(update, std::memory_order_acq_rel) - update;
        if (current == 0)
        {
            mCounter.notify_all();
        }
        else
        {
            assert(current > 0);

            mCounter.wait(current, std::memory_order_relaxed);
            wait();
        }
    }

private:
    std::atomic<std::ptrdiff_t> mCounter;
};
} //  Namespace mingw_stdthread

namespace std
{
using mingw_stdthread::latch;
} //  Namespace std

#endif // MINGW_LATCH_H_
