#include "../mingw.thread.h"
#include "../mingw.mutex.h"
#include "../mingw.condition_variable.h"
#include "../mingw.shared_mutex.h"
#include <atomic>
#include <assert.h>
#include <string>
#include <iostream>
#include <windows.h>

using namespace std;

int cond = 0;
std::mutex m;
std::shared_mutex sm;
std::condition_variable cv;
std::condition_variable_any cv_any;
#define LOG(fmtString,...) printf(fmtString "\n", ##__VA_ARGS__); fflush(stdout)
void test_call_once(int a, const char* str)
{
    LOG("test_call_once called with a=%d, str=%s", a, str);
    this_thread::sleep_for(std::chrono::milliseconds(5000));
}

struct TestMove
{
    std::string mStr;
    TestMove(const std::string& aStr): mStr(aStr){}
    TestMove(TestMove&& other): mStr(other.mStr+" moved")
    { printf("%s: Object moved\n", mStr.c_str()); }
    TestMove(const TestMove&) : mStr()
    {
        assert(false && "TestMove: Object COPIED instead of moved");
    }
};

int main()
{
    if (!is_standard_layout<mutex>::value)
      LOG("WARNING: Class std::%s does not satisfy concept StandardLayoutType.","mutex");
    if (!is_standard_layout<recursive_mutex>::value)
      LOG("WARNING: Class std::%s does not satisfy concept StandardLayoutType.","recursive_mutex");
    if (!is_standard_layout<timed_mutex>::value)
      LOG("WARNING: Class std::%s does not satisfy concept StandardLayoutType.","timed_mutex");
    if (!is_standard_layout<recursive_timed_mutex>::value)
      LOG("WARNING: Class std::%s does not satisfy concept StandardLayoutType.","recursive_timed_mutex");
    if (!is_standard_layout<shared_mutex>::value)
      LOG("WARNING: Class std::%s does not satisfy concept StandardLayoutType.","shared_mutex");
    if (!is_standard_layout<shared_timed_mutex>::value)
      LOG("WARNING: Class std::%s does not satisfy concept StandardLayoutType.","shared_timed_mutex");
//    With C++ feature level and target Windows version potentially affecting
//  behavior, make this information visible.
    {
        switch (__cplusplus)
        {
            case 201103L: std::cout << "Compiled in C++11"; break;
            case 201402L: std::cout << "Compiled in C++14"; break;
            case 201703L: std::cout << "Compiled in C++17"; break;
            default: std::cout << "Compiled in a non-conforming C++ compiler";
        }
        std::cout << ", targeting Windows ";
        static_assert(WINVER > 0x0500, "Windows NT and earlier are not supported.");
        switch (WINVER)
        {
            case 0x0501: std::cout << "XP"; break;
            case 0x0502: std::cout << "Server 2003"; break;
            case 0x0600: std::cout << "Vista"; break;
            case 0x0601: std::cout << "7"; break;
            case 0x0602: std::cout << "8"; break;
            case 0x0603: std::cout << "8.1"; break;
            case 0x0A00: std::cout << "10"; break;
            default: std::cout << "10+";
        }
        std::cout << "\n";
    }

    {
        LOG("%s","Testing serialization and hashing for thread::id...");
        std::cout << "Serialization:\t" << this_thread::get_id() << "\n";
        std::hash<thread::id> hasher;
        std::cout << "Hash:\t" << hasher(this_thread::get_id()) << "\n";
    }
    std::thread t([](TestMove&& a, const char* b, int c) mutable
    {
        try
        {
            LOG("%s","Worker thread started, sleeping for a while...");
//  Thread might move the string more than once.
            assert(a.mStr.substr(0, 15) == "move test moved");
            assert(!strcmp(b, "test message"));
            assert(c == -20);
            auto move2nd = std::move(a); //test move to final destination
            this_thread::sleep_for(std::chrono::milliseconds(5000));
            {
                lock_guard<mutex> lock(m);
                cond = 1;
                LOG("%s","Notifying condvar");
                cv.notify_all();
            }

            this_thread::sleep_for(std::chrono::milliseconds(500));
            {
                lock_guard<decltype(sm)> lock(sm);
                cond = 2;
                LOG("%s","Notifying condvar");
                cv_any.notify_all();
            }

            this_thread::sleep_for(std::chrono::milliseconds(500));
            {
                lock_guard<decltype(sm)> lock(sm);
                cond = 3;
                LOG("%s","Notifying condvar");
                cv_any.notify_all();
            }

            LOG("%s","Worker thread finishing");
        }
        catch(std::exception& e)
        {
            printf("EXCEPTION in worker thread: %s\n", e.what());
        }
    },
    TestMove("move test"), "test message", -20);
    try
    {
      LOG("%s","Main thread: Locking mutex, waiting on condvar...");
      {
          std::unique_lock<decltype(m)> lk(m);
          cv.wait(lk, []{ return cond >= 1;} );
          LOG("condvar notified, cond = %d", cond);
          assert(lk.owns_lock());
      }
      LOG("%s","Main thread: Locking shared_mutex, waiting on condvar...");
      {
          std::unique_lock<decltype(sm)> lk(sm);
          cv_any.wait(lk, []{ return cond >= 2;} );
          LOG("condvar notified, cond = %d", cond);
          assert(lk.owns_lock());
      }
      LOG("%s","Main thread: Locking shared_mutex in shared mode, waiting on condvar...");
      {
          std::shared_lock<decltype(sm)> lk(sm);
          cv_any.wait(lk, []{ return cond >= 3;} );
          LOG("condvar notified, cond = %d", cond);
          assert(lk.owns_lock());
      }
      LOG("%s","Main thread: Waiting on worker join...");

      t.join();
      LOG("%s","Main thread: Worker thread joined");
      fflush(stdout);
    }
    catch(std::exception& e)
    {
        LOG("EXCEPTION in main thread: %s", e.what());
    }
    once_flag of;
    call_once(of, test_call_once, 1, "test");
    call_once(of, test_call_once, 1, "ERROR! Should not be called second time");
    LOG("%s","Test complete");
    return 0;
}
