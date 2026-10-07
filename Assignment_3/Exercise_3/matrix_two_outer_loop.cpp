#include <cmath>
#include <iostream>
#include <bits/chrono.h>
#include <omp.h>
#include <vector>

// Written by Filip Hellgren, David Olmedo
// Compile by running:
// "g++ -fopenmp Exercise_3/matrix_two_outer_loop.cpp -o a.out"

// Run by typing:
// "OMP_NUM_THREADS=<ThreadCount> ./a.out"

#define DIM 512

void print_matrix(const std::vector<std::vector<int>> &matrix, const char *name)
{
    // Print matrix
    std::cout << "\nMatrix '" << name << "': \n";

    for (int i = 0; i < DIM; i++)
    {
        for (int j = 0; j < DIM; j++)
        {
            std::cout << matrix[i][j] << "\t";
        }
        std::cout << '\n';
        for (size_t i = 0; i < DIM; i++)
        {
            std::cout << "--------";
        }

        std::cout << '\n';
    }
}

int main(int argc, char *argv[])
{
    std::vector<std::vector<int>> a(DIM, std::vector<int>(DIM));
    std::vector<std::vector<int>> b(DIM, std::vector<int>(DIM));
    std::vector<std::vector<int>> c(DIM, std::vector<int>(DIM));

    for (size_t i = 0; i < DIM; i++)
    {
        for (size_t j = 0; j < DIM; j++)
        {
            a[i][j] = i + j;
            b[i][j] = i + j;
            c[i][j] = 0;
        }
    }

    auto start = std::chrono::steady_clock::now();
#pragma omp parallel default(private) shared(a, b, c)
#pragma omp for schedule(static) collapse(2)
    for (int i = 0; i < DIM; i++)
    {
        for (int j = 0; j < DIM; j++)
        {
            for (int k = 0; k < DIM; k++)
            {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }

    auto end = std::chrono::steady_clock::now();

    // print_matrix(a, "A");
    // print_matrix(b, "B");
    // print_matrix(c, "C");
    //  Calculate duration in milliseconds
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Thread count: " << omp_get_max_threads() << " Matrix size: " << DIM << "x" << DIM << "\n";
    std::cout << "Elapsed time: " << duration.count() << " ms\n";

    return 0;
}