#undef _GLIBCXX_HAS_GTHREADS
#include "../mingw.thread.h"
#include <mutex>
#include "../mingw.mutex.h"
#include "../mingw.condition_variable.h"
#include <atomic>
#include <assert.h>

using namespace std;

bool cond = false;
std::mutex m;
std::condition_variable cv;
#define LOG(fmtString,...) printf(fmtString "\n", ##__VA_ARGS__); fflush(stdout)
int main()
{
    std::thread t([](bool a, const char* b, int c)mutable
    {
        try
        {
            LOG("Worker thread started, sleeping for a while...");
            assert(a && !strcmp(b, "test message") && (c == -20));
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
    true, "test message", -20);
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

    return 0;
}

