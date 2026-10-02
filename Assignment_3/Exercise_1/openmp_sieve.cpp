#include <cmath>
#include <iostream>
#include <bits/chrono.h>
#include <vector>
#include <omp.h>

#define THREAD_COUNT 10
#define MAX 10000000

// Written by David Olmedo and Filip Hellgren
// This OpenMP version of sieve.cpp is based on our POSIX version written together for Assignment 2
// Compile by running:
// "g++ -fopenmp Exercise_1/openmp_sieve.cpp -o a.out"
// Run by typing:
// "./a.out"

typedef std::vector<uint8_t> marks_t;

typedef struct thread_info
{
    int id;
    int start;
    int end;
    marks_t *marks;
    int seed_end;
} thread_info_t;

marks_t sequential_sieve(int n)
{
    marks_t marked(n);
    for (int k = 2; k * k <= n; k++)
    {
        if (marked[k - 1] == true)
        {
            continue;
        }

        int i = k * 2;
        while (i <= n)
        {
            marked[i - 1] = true;
            i += k;
        }
    }
    return marked;
}

void thread_func(void *args)
{
    thread_info_t info = *((thread_info_t *)args);
    int start = info.start;
    int end = info.end;
    marks_t *marks = info.marks;
    int seed_end = info.seed_end;

    for (int p = 2; p <= seed_end; p++)
    {
        if ((*marks)[p - 1] == true)
        {
            // Is not prime
            continue;
        }
        int smallest_k = (int)ceil((double)start / p);
        if (smallest_k == 1)
        {
            smallest_k = 2;
        }
        int curr = p * smallest_k;

        while (curr <= end)
        {
            (*marks)[curr - 1] = true;
            curr += p;
        }
    }
}

int main(int argc, char *argv[])
{
    marks_t seqPrimes = sequential_sieve((int)sqrt(MAX));
    marks_t shared_marks(MAX);
    for (size_t i = 0; i < sqrt(MAX); i++)
    {
        shared_marks[i] = seqPrimes[i];
    }
    shared_marks[0] = true;

    int start_i = sqrt(MAX) + 1;
    int end_i = MAX;
    int work = end_i - start_i;
    int num_threads = THREAD_COUNT;
    int chunk_size = work / num_threads;

    auto start = std::chrono::steady_clock::now();
#pragma omp parallel for num_threads(THREAD_COUNT)
    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        thread_info_t info;
        info.id = i;
        info.start = start_i + chunk_size * i;
        if (i + 1 != THREAD_COUNT)
        {
            info.end = info.start + chunk_size;
        }
        else
        {
            info.end = end_i;
        }
        info.marks = &shared_marks;
        info.seed_end = sqrt(MAX);

        thread_func(&info);
    }
    auto end = std::chrono::steady_clock::now();
    int count = 0;
    for (size_t i = 1; i <= MAX; i++)
    {
        if (shared_marks[i - 1] == false)
        {
            std::cout << "p: " << i << "\n";
            count++;
        }
    }
    std::cout << "Primes found: " << count << "\n";
    std::cout << "Thread count: " << THREAD_COUNT << "\n";
    std::cout << "Max: " << MAX << "\n";

    // Calculate duration in milliseconds
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Elapsed time: " << duration.count() << " ms\n";
}