#include <cmath>
#include <iostream>
#include <pthread.h>
#include <format>
#include <bits/chrono.h>

#define DEF_THREAD_COUNT 1
#define DEF_TRAPEZE_COUNT 10

#define INTERVAL_MIN 0
#define INTERVAL_MAX 1

#define DEBUG true

typedef struct info
{
    int t_id;
    int t_count;
    int trap_count; // Hela intervallet
    double dx;
} info_t;

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
    info_t thread_info = *((info_t *)args);
    int t_id = thread_info.t_id;
    int t_count = thread_info.t_count;
    int total_traps = thread_info.trap_count;
    double dx = thread_info.dx;

    int traps_per_thread = total_traps / t_count;

    int start_trap = t_id * traps_per_thread;
    // Takes all the rest of the trapezes if this is the last thread,
    // to solve issues when number of trapezes are not divisible by thread count.
    int end_trap = (t_id == t_count - 1) ? total_traps : start_trap + traps_per_thread;

    double sum_local = 0;
    for (int i = start_trap; i < end_trap; i++)
    {
        double x1 = INTERVAL_MIN + i * dx;
        double x2 = x1 + dx;

        double area = calculateTrapeze(x1, x2);
        sum_local += area;
    }

    pthread_spin_lock(&sum_lock);
    sum += sum_local;
    pthread_spin_unlock(&sum_lock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int thread_count = DEF_THREAD_COUNT;
    int trapeze_count = DEF_TRAPEZE_COUNT;
    if (argc == 2)
    {
        std::string arg = argv[1];
        if (arg == "-h")
        {
            std::cout << "Usage: ./e1.out <thread_count> <trapeze_count>\n";
            return 0;
        }
    }
    if (argc > 1)
    {
        thread_count = std::stoi(argv[1]);
    }
    if (argc > 2)
    {
        trapeze_count = std::stoi(argv[2]);
    }

    pthread_spin_init(&sum_lock, 0);
    double step = (INTERVAL_MAX - INTERVAL_MIN) / (double)trapeze_count;

    pthread_t threads[thread_count];
    info_t thread_infos[thread_count];

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < thread_count; i++)
    {
        thread_infos[i].t_id = i;
        thread_infos[i].t_count = thread_count;
        thread_infos[i].trap_count = trapeze_count;
        thread_infos[i].dx = (INTERVAL_MAX - INTERVAL_MIN) / (double)trapeze_count;

        pthread_create(&threads[i], &attr, thread_func, &thread_infos[i]);
    }

    for (size_t i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], NULL);
    }
    auto end = std::chrono::steady_clock::now();

    pthread_spin_destroy(&sum_lock);

    // Calculate duration in milliseconds
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Thread count: " << thread_count << "\n";
    std::cout << "Trapeze count: " << trapeze_count << "\n";
    std::cout << "Elapsed time: " << duration.count() << " ms\n";
    std::cout << "Total area: " << sum << "\n";
}