#include "../mingw.thread.h"
#include <mutex>
#include "../mingw.mutex.h"
#include "../mingw.condition_variable.h"
#include <atomic>
using namespace std;

bool cond = false;
std::mutex m;
std::condition_variable cv;

int main()
{

    std::thread t([](int a, const char* b, int c)mutable
    {
      try
      {
  //     printf("Thread started: arg = %d, %s, %d\n", a, b, c);
  //     fflush(stdout);
     this_thread::sleep_for(std::chrono::milliseconds(5000));
       {
        lock_guard<mutex> lock(m);
        cond = true;
        cv.notify_all();
       }

       printf("thread finished\n");
       fflush(stdout);
      }
      catch(std::exception& e)
      {
        printf("EXCEPTION in thread: %s\n", e.what());
      }
    },
    1, "test message", 3);
try
{
//    printf("trylock: %d\n", m.try_lock());
//    fflush(stdout);
    printf("condvar waiting\n");
    fflush(stdout);
    {
        std::unique_lock<mutex> lk(m);
        cv.wait(lk, []{ return cond;} );
        printf("condvar notified, cond = %d\n", cond);
        fflush(stdout);
    }
    printf("waiting for thread to terminate\n");
    fflush(stdout);

    t.join();
    printf("join complete\n");
    fflush(stdout);
}
catch(std::exception& e)
{
    printf("EXCEPTION in main thread: %s\n", e.what());
}

    return 0;
}

