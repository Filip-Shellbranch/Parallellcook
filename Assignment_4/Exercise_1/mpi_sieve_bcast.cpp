#include <cmath>
#include <iostream>
#include <format>
#include <bits/chrono.h>
#include <vector>
#include <cstdint>
#include <mpi.h>

// Written by Filip Hellgren, David Olmedo
// This MPI version of sieve.cpp is based on our POSIX version written together for Assignment 2
// Compile by running:
// "mpic++ Exercise_1/mpi_sieve_bcase.cpp -o sieve"
// Run by typing:
// "mpiexec hostfile=hosts ./sieve"
#define MAX 100000000

typedef std::vector<uint8_t> marks_t;

typedef struct p_info
{
    int start;
    int end;
    marks_t *local_marks;
    int seed_end;
} p_info_t;

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

void sieve_func(p_info_t *info_p)
{
    p_info_t info = *((p_info_t *)info_p);
    int start = info.start;
    int end = info.end;
    marks_t *marks = info.local_marks;
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
            int index = curr - 1;
            (*info.local_marks)[index] = true;
            curr += p;
        }
    }
}

int main(int argc, char *argv[])
{
    marks_t shared_marks(MAX);

    MPI_Init(&argc, &argv);

    int num_ranks;
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    int id;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    int start_i = sqrt(MAX) + 1;
    int end_i = MAX;

    int seq_end = start_i - 1;

    marks_t local_marks(MAX, 0); // Alla har samma storlek vektor (OH YEAHHH)

    if (id == 0)
    {
        marks_t seq_marks = sequential_sieve(seq_end);

        for (size_t i = 0; i < seq_end; i++)
        {
            local_marks[i] = seq_marks[i];
        }
        local_marks[0] = true;
    }

    auto start = std::chrono::steady_clock::now();
    MPI_Bcast(local_marks.data(), seq_end, MPI_UINT8_T, 0, MPI_COMM_WORLD);

    int work = end_i - start_i + 1;
    int chunk_size = work / num_ranks;
    int my_start = start_i + chunk_size * id;

    p_info_t info;
    info.start = start_i + chunk_size * id;
    if (id + 1 != num_ranks)
    {
        info.end = info.start + chunk_size - 1;
    }
    else
    {
        info.end = end_i;
    }
    info.seed_end = seq_end;

    int last_process_chunk_size = end_i - chunk_size * (num_ranks - 1) - seq_end + 1;
    int this_chunk_size = chunk_size;
    if (id == num_ranks - 1)
    {
        this_chunk_size = last_process_chunk_size;
    }
    info.local_marks = &local_marks;

    sieve_func(&info);

    // Collect results
    auto collect_start = std::chrono::steady_clock::now();

    MPI_Reduce(local_marks.data(), shared_marks.data(), MAX, MPI_UINT8_T, MPI_BOR, 0, MPI_COMM_WORLD);

    auto collect_end = std::chrono::steady_clock::now();
    MPI_Finalize();

    auto end = std::chrono::steady_clock::now();
    int count = 0;

    if (id == 0)
    {
        for (size_t i = 1; i <= MAX; i++)
        {
            if (shared_marks[i - 1] == false)
            {
                std::cout << "IS PRIME: " << i << "\n";
                count++;
            }
        }
        std::cout << "Primes found: " << count << "\n";
        std::cout << "Number of ranks: " << num_ranks << "\n";
        std::cout << "Max: " << MAX << "\n";

        // Calculate duration in milliseconds
        std::chrono::duration<double, std::milli> duration = end - start;
        std::chrono::duration<double, std::milli> collection_duration = end - collect_start;

        std::cout << "Parallel compute time: " << duration.count() - collection_duration.count() << " ms\n";
        std::cout << "Serial data collection time: " << collection_duration.count() << " ms\n";
        std::cout << "Elapsed time: " << duration.count() << " ms\n";
    }
    return 0;
}