#include <cmath>
#include <iostream>
#include <pthread.h>
#include <format>
#include <bits/chrono.h>

#define THREAD_COUNT 1
#define TRAPEZE_COUNT 10000

#define INTERVAL_MIN 0
#define INTERVAL_MAX 1

#define DEBUG true

pthread_spinlock_t counter_lock;
int shared_counter = 0;

pthread_spinlock_t sum_lock;
double sum = 0;

float function(double x)
{
    return 4 / (1 + pow(x, 2));
}

double max(double a, double b)
{
    if (a > b)
    {
        return a;
    }
    else
    {
        return b;
    }
}

double min(double a, double b)
{
    if (a < b)
    {
        return a;
    }
    else
    {
        return b;
    }
}

double calculateTrapeze(double x1, double x2)
{
    double y1 = function(x1);
    double y2 = function(x2);
    double minY = min(y1, y2);
    double maxY = max(y1, y2);

    double dX = x2 - x1;
    double rect = minY * dX;
    double tri = dX * (maxY - minY) / 2;
    return rect + tri;
}

void *thread_func(void *args)
{
    while (true)
    {
        pthread_spin_lock(&counter_lock);
        int id = shared_counter;
        shared_counter++;
        pthread_spin_unlock(&counter_lock);

        if (id > TRAPEZE_COUNT)
        {
            break;
        }

        double step = *(double *)(args);

        double start = INTERVAL_MIN + id * step;
        double end = start + step;
        double area = calculateTrapeze(start, end);

        pthread_spin_lock(&sum_lock);
        sum += area;
        pthread_spin_unlock(&sum_lock);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    pthread_spin_init(&counter_lock, 0);
    pthread_spin_init(&sum_lock, 0);
    double step = (INTERVAL_MAX - INTERVAL_MIN) / (double)TRAPEZE_COUNT;

    pthread_t threads[THREAD_COUNT];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        pthread_create(&threads[i], &attr, thread_func, &step);
    }
    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(threads[i], NULL);
    }
    auto end = std::chrono::steady_clock::now();

    // Calculate duration in milliseconds
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Elapsed time: " << duration.count() << " ms\n";
    std::cout << "Total area: " << sum << "\n";
}