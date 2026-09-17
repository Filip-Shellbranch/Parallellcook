#include <cmath>
#include <iostream>
#include <pthread.h>
#include <format>
#include <bits/chrono.h>

#define THREAD_COUNT 1
#define MAX 1000

void *thread_func(void *args)
{
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    pthread_t threads[THREAD_COUNT];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        pthread_create(&threads[i], &attr, thread_func, NULL);
    }
    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(threads[i], NULL);
    }
    auto end = std::chrono::steady_clock::now();

    // Calculate duration in milliseconds
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Elapsed time: " << duration.count() << " ms\n";
}