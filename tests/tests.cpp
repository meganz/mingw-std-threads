#undef _GLIBCXX_HAS_GTHREADS
#include "../mingw.thread.h"
#include <mutex>
#include "../mingw.mutex.h"
#include "../mingw.condition_variable.h"
#include "../mingw.shared_mutex.h"
#include <atomic>
#include <assert.h>

using namespace std;

int cond = 0;
std::mutex m;
std::shared_mutex sm;
std::condition_variable_any cv;
#define LOG(fmtString,...) printf(fmtString "\n", ##__VA_ARGS__); fflush(stdout)
int main()
{
  if ((typeid(std::thread) != typeid(mingw_stdthread::thread)) ||
      (typeid(std::shared_mutex) != typeid(mingw_stdthread::shared_mutex)) ||
      (typeid(std::condition_variable) != typeid(mingw_stdthread::condition_variable)) ||
      (typeid(std::mutex) != typeid(mingw_stdthread::mutex)))
    LOG("Using built-in classes. Tests will not reflect status of MinGW STD threads.");
    std::thread t([](bool a, const char* b, int c)mutable
    {
        try
        {
            LOG("Worker thread started, sleeping for a while...");
            assert(a && !strcmp(b, "test message") && (c == -20));
            this_thread::sleep_for(std::chrono::milliseconds(5000));
            {
                lock_guard<decltype(m)> lock(m);
                cond = 1;
                LOG("Notifying condvar");
                cv.notify_all();
            }

            this_thread::sleep_for(std::chrono::milliseconds(500));
            {
                lock_guard<decltype(sm)> lock(sm);
                cond = 2;
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
      std::unique_lock<decltype(m)> lk(m);
      std::unique_lock<decltype(sm)> slk(sm);
      LOG("Main thread: waiting on condvar...");
      {
          //std::unique_lock<decltype(m)> lk(m);
          cv.wait(lk, []{ return cond == 1;} );
          LOG("condvar notified, cond = %d", cond);
      }
      LOG("Main thread: waiting on condvar...");
      {
          //std::unique_lock<decltype(m)> lk(m);
          cv.wait(slk, []{ return cond == 2;} );
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

