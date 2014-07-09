mingw-std-threads
=================

Standard C++11 threading classes implementation, which are currently still missing
on MinGW GCC.

Usage
=====

This is a header-only library. To use, just include the corresponding mingw.xxx.h file, where
xxx would be the name of the standard header that you would normally include.
For additional mutex helper classes, such as std::scoped_guard or std::unique_lock, you need to
include &lt;mutex&gt; before including mingw.mutex.h

Compatibility
=============

This code has been developed with MinGW64 4.8.2, but should work with any other MinGW version
that has C++11 support for lambda functions, variadic templates, and has
working mutex helper classes in &lt;mutex&gt;

Why MinGW has no threading classes 
==================================
It seems that for cross-platform threading implementation, the GCC standard library relies on
the gthreads library. If this library is not available, as is the case with MinGW, the
std::thread, std::mutex, std::condition_variable are not defined. However, higher-level mutex
helper classes are still defined in &lt;mutex&gt; and are usable. Hence, this implementation
does not re-define them, and to use these helpers, you should include &lt;mutex&gt; as well, as explained
in the usage section.
