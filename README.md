mingw-std-threads
=================

Standard C++11 threading classes implementation, which are currently still missing
on MinGW GCC.

Windows compatibility
=====================
This implementation should work with Windows XP (regardless of service pack), or newer.
The library automatically detects the version of Windows that is being targeted, and selects an implementation that takes advantage of available Windows features. In MinGW GCC, the target Windows version may optionally be selected by the command-line option `-D _WIN32_WINNT=...`. Use `0x0600` for Windows Vista, or `0x0601` for Windows 7. See ["Modifying `WINVER` and `_WIN32_WINNT`](https://docs.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt).

Usage
=====

This is a header-only library. To use, just include the corresponding mingw.xxx.h file, where
xxx would be the name of the standard header that you would normally include.
For additional mutex helper classes, such as std::scoped_guard or std::unique_lock, you need to
include &lt;mutex&gt; before including mingw.mutex.h

Compatibility
=============

This code has been tested to work with MinGW-w64 5.3.0, but should work with any other MinGW version
that has the std threading classes missing, has C++11 support for lambda functions, variadic
templates, and has working mutex helper classes in &lt;mutex&gt;.  

Switching from the win32-pthread based implemetation
====================================================
It seems that recent versions of mingw-w64 include a win32 port of pthreads, and have
the std::thread, std::mutex etc classes implemented and working, based on that compatibility
layer. This is a somewhat heavier implementation, as it brings a not very thin abstraction layer.
So you may still want to use this implementation for efficiency purposes. Unfortunately you can't use it
standalone and independent of the system &lt;mutex&gt; header, as it relies on it for std::unique_lock and other
non-trivial utility classes. In that case you will need to edit the c++-config.h file of your MinGW setup
and comment out the definition of _GLIBCXX_HAS_GTHREADS. This will cause the system headers to not define the
actual thread, mutex, etc classes, but still define the necessary utility classes.

Why MinGW has no threading classes 
==================================
It seems that for cross-platform threading implementation, the GCC standard library relies on
the gthreads/pthreads library. If this library is not available, as is the case with MinGW, the
std::thread, std::mutex, std::condition_variable are not defined. However, higher-level mutex
helper classes are still defined in &lt;mutex&gt; and are usable. Hence, this implementation
does not re-define them, and to use these helpers, you should include &lt;mutex&gt; as well, as explained
in the usage section.
