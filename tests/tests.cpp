#undef _GLIBCXX_HAS_GTHREADS
#include "../mingw.thread.h"
#include <mutex>
#include "../mingw.mutex.h"
#include "../mingw.condition_variable.h"
#include <atomic>
#include <assert.h>
#include <string>

using namespace std;

bool cond = false;
std::mutex m;
std::condition_variable cv;
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
    TestMove(const TestMove& other)
    {
        assert(false && "TestMove: Object COPIED instead of moved");
    }
};

int main()
{
    std::thread t([](TestMove&& a, const char* b, int c) mutable
    {
        try
        {
            LOG("Worker thread started, sleeping for a while...");
            assert(a.mStr == "move test moved" && !strcmp(b, "test message")
               && (c == -20));
            auto move2nd = std::move(a); //test move to final destination
            this_thread::sleep_for(std::chrono::milliseconds(5000));
            {
                lock_guard<mutex> lock(m);
                cond = true;
                LOG("Notifying condvar");
                cv.notify_all();
            }

            LOG("Worker thread finishing");
        }
        catch(std::exception& e)
        {
            printf("EXCEPTION in worker thread: %s\n", e.what());
        }
    },
    TestMove("move test"), "test message", -20);
    try
    {
        LOG("Main thread: waiting on condvar...");
        {
            std::unique_lock<mutex> lk(m);
            cv.wait(lk, []{ return cond;} );
            LOG("condvar notified, cond = %d", cond);
        }
        LOG("Main thread: Waiting on worker join...");

        t.join();
        LOG("Main thread: Worker thread joined");
        fflush(stdout);
    }
    catch(std::exception& e)
    {
        LOG("EXCEPTION in main thread: %s", e.what());
    }
    once_flag of;
    call_once(of, test_call_once, 1, "test");
    call_once(of, test_call_once, 1, "ERROR! Should not be called second time");
    LOG("Test complete");
    return 0;
}

