#include <cmath>
#include <iostream>
#include <format>
#include <bits/chrono.h>
#include <vector>
#include <cstdint>
#include <mpi.h>

// Written by Filip Hellgren, David Olmedo
#define MAX 100000000

typedef std::vector<uint8_t> marks_t;

typedef struct p_info
{
    int id;
    int start;
    int end;
    marks_t *seq_marks;
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
    marks_t *marks = info.seq_marks;
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

            int index = curr - start;
            (*info.local_marks)[index] = true;
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

    MPI_Init(&argc, &argv);
    int num_ranks;
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    int id;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    int start_i = sqrt(MAX) + 1;
    int end_i = MAX;
    int work = end_i - start_i;

    int chunk_size = work / num_ranks;
    int seq_end = start_i;

    auto start = std::chrono::steady_clock::now();

    p_info_t info;
    info.id = id;
    info.start = start_i + chunk_size * id;
    if (id + 1 != num_ranks)
    {
        info.end = info.start + chunk_size;
    }
    else
    {
        info.end = end_i;
    }
    info.seq_marks = &shared_marks;
    info.seed_end = sqrt(MAX);

    int last_process_chunk_size = end_i - chunk_size * (num_ranks - 1) - seq_end + 1;
    int this_chunk_size = chunk_size;
    if (id == num_ranks - 1)
    {
        this_chunk_size = last_process_chunk_size;
    }
    marks_t local_marks(this_chunk_size);
    info.local_marks = &local_marks;

    sieve_func(&info);

    // Collect results
    auto collect_start = std::chrono::steady_clock::now();
    if (id == 0)
    {
        for (size_t i = 0; i < this_chunk_size; i++)
        {

            int local_index = i;
            int shared_index = seq_end + i;
            shared_marks[shared_index - 1] = local_marks[local_index];
            // Lägg till i stora arrayen
        }
        for (size_t rank = 1; rank < num_ranks; rank++)
        {
            int num_values = chunk_size;
            if (rank == num_ranks - 1)
            {
                // Chunk size kommer vara större
                num_values = last_process_chunk_size;
            }
            marks_t local_marks(num_values, 0);
            MPI_Status status;

            MPI_Recv(local_marks.data(), num_values, MPI_UINT8_T, rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            for (size_t i = 0; i < num_values; i++)
            {
                int local_index = i;
                int shared_index = seq_end + (rank * chunk_size) + i - 1;
                // Add to shared array
                shared_marks[shared_index] = local_marks[local_index];
            }
        }
    }
    else
    {
        MPI_Send(local_marks.data(), this_chunk_size, MPI_UINT8_T, 0, 0, MPI_COMM_WORLD);
    }
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
                // std::cout << "IS PRIME: " << i << "\n";
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