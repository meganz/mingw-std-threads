#include "../mingw.thread.h"
#include "../mingw.mutex.h"
using namespace std;

int main()
{
    std::mutex m;
    std::thread t([&m](int a, const char* b, int c)mutable
    {
       printf("Thread started: arg = %d, %s, %d\n", a, b, c);
       fflush(stdout);
       m.lock();
       this_thread::sleep_for(std::chrono::milliseconds(5000));
       m.unlock();
       this_thread::sleep_for(std::chrono::milliseconds(5000));
       printf("thread finished\n");
       fflush(stdout);

    },
    1, "test message", 3);
    this_thread::sleep_for(std::chrono::milliseconds(1));
    printf("trylock: %d\n", m.try_lock());
    fflush(stdout);
    printf("mutex waiting\n");
    fflush(stdout);
    m.lock();
    printf("join waiting\n");
    fflush(stdout);
    t.join();
    printf("join complete\n");
    fflush(stdout);
    return 0;
}

