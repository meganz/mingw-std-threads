/**
* @file mingw.throw.h
* @brief throw helper to enable -fno-exceptions
* (c) 2013-2016 by Mega Limited, Auckland, New Zealand
* @author Maeiky
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

#ifndef MINGW_THROW_H_
#define MINGW_THROW_H_

#if !defined(__cplusplus) || (__cplusplus < 201103L)
#error A C++11 compiler is required!
#endif

#if (defined(__cpp_exceptions) && (__cpp_exceptions >= 199711L)) || \
  defined(__EXCEPTIONS) || (!defined(__clang__) && !defined(__GNUC__))
#define MINGW_STDTHREAD_NO_EXCEPTIONS 0
#else
#define MINGW_STDTHREAD_NO_EXCEPTIONS 1
#endif

//    Disabling exceptions is very much non-standard behavior. Though C++20 may
//  add a feature-test macro, earlier versions do not provide such a mechanism.
//  Instead, appropriate compiler-specific macros must be checked.
namespace mingw_stdthread
{
#if MINGW_STDTHREAD_NO_EXCEPTIONS
template<class T, class ... Args>
inline void throw_error (Args&&...)
{
  std::abort();
}
#else
template<class T, class ... Args>
inline void throw_error (Args&&... args)
{
  throw T(std::forward<Args>(args)...);
}
#endif
}


#endif // MINGW_THROW_H_
