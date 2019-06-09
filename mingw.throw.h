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

// (Multi args version)
//#define mingw_throw_multi(_1,_2,NAME,...) NAME
//#define mingw_throw_system_error(...) mingw_throw_multi(__VA_ARGS__,  mingw_throw_system_error2, mingw_throw_system_error1)(__VA_ARGS__)

//    Disabling exceptions is very much non-standard behavior. Though C++20 may
//  add a feature-test macro, earlier versions do not provide such a mechanism.
//  Instead, appropriate compiler-specific macros must be checked.
#if (defined(__cpp_exceptions) && (__cpp_exceptions >= 199711L)) || \
  defined(__EXCEPTIONS) || (!defined(__clang__) && !defined(__GNUC__))

	#define mingw_make_error_code(err) 			     std::make_error_code(err)

	#define mingw_throw_system_error(err)   	     throw std::system_error(err)
	#define mingw_throw_system_error_arg(err, arg)   throw std::system_error(err, arg)

	#define mingw_throw_future_error(err)   	     throw std::future_error(err)
	#define mingw_throw_runtime_error(err)  	     throw std::runtime_error(err)

#else

	#define mingw_make_error_code(err) 			     int(err)

	#define mingw_throw_system_error(err)   	     std::__throw_system_error(err)
	#define mingw_throw_system_error_arg(err, arg)   std::__throw_system_error(err)

	#define mingw_throw_future_error(err)   	     std::__throw_future_error(err)
	#define mingw_throw_runtime_error(err)  	     std::__throw_runtime_error(err)

#endif


#endif // MINGW_THROW_H_
