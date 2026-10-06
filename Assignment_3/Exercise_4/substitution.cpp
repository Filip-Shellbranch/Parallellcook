#include <cmath>
#include <vector>
#include <bits/chrono.h>
#include <omp.h>
#include <iostream>

#define VARS 10000 // Ska testa med 42.000 vars 🤯🦧

typedef struct tri_system
{
    std::vector<std::vector<double>> A; // Coefficient matrix
    std::vector<double> b;              // Reduced right hand side column
    std::vector<double> x;              // Resulting column
} system_t;

void print_system(system_t sys)
{
    size_t n = sys.A.size();

    for (size_t i = 0; i < n; ++i)
    {
        bool first = true;

        for (size_t j = i; j < n; ++j)
        {
            int coeff = sys.A[i][j];

            if (coeff == 0)
                continue;

            if (!first)
            {
                std::cout << (coeff > 0 ? " + " : " - ");
            }
            else if (coeff < 0)
            {
                std::cout << "-";
            }

            int abs_coeff = std::abs(coeff);

            if (abs_coeff != 1)
                std::cout << abs_coeff;

            std::cout << "x" << j;

            first = false;
        }

        std::cout << " = " << sys.b[i] << '\n';
    }
}

void print_result(system_t result)
{
    std::cout << "Result:\n";

    for (int i = 0; i < result.x.size(); i++)
    {
        std::cout << "x" << i << ": " << result.x[i] << '\n';
    }
}

system_t generate_system()
{
    system_t sys;

    sys.A.resize(VARS, std::vector<double>(VARS));
    sys.b.resize(VARS);
    sys.x.resize(VARS);

#pragma omp parallel for num_threads(12)
    for (size_t row = 0; row < VARS; row++)
    {
        sys.x[row] = 0;
        sys.b[row] = row;
    }

#pragma omp parallel for collapse(2) num_threads(12)
    for (size_t row = 0; row < VARS; row++)
    {
        for (size_t col = 0; col < VARS; col++)
        {
            int val = (col >= row) ? row + col + 1 : 0;
            sys.A[row][col] = val;
        }
    }

    return sys;
}

void row_subs()
{
    system_t sys = generate_system();
    auto &A = sys.A;
    auto &b = sys.b;
    auto &x = sys.x;

    for (int row = VARS - 1; row >= 0; row--)
    {
        x[row] = b[row];
        for (int col = row + 1; col < VARS; col++)
            x[row] -= A[row][col] * x[col];
        x[row] /= A[row][row];
    }
}

void column_subs()
{
    auto starty = std::chrono::steady_clock::now();
    system_t sys = generate_system();
    auto endy = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> durationy = endy - starty;
    std::cout << "Elapsed time generating system: " << durationy.count() << " ms\n";
    std::vector<std::vector<double>> &A = sys.A;
    std::vector<double> &b = sys.b;
    std::vector<double> &x = sys.x;

    // print_system(sys);
    auto start = std::chrono::steady_clock::now();
    for (int row = 0; row < VARS; row++)
    {
        x[row] = b[row];
    }
    for (int col = VARS - 1; col >= 0; col--)
    {
        x[col] /= A[col][col];
#pragma omp parallel for shared(x, A, col)
        for (int row = 0; row < col; row++)
            x[row] -= A[row][col] * x[col];
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << "Column substitution:\n";
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Thread count: " << omp_get_max_threads() << " System size: " << VARS << "\n";
    std::cout << "Elapsed time: " << duration.count() << " ms\n";
    // print_result(sys);
}

int main(int argc, char *argv[])
{
    // std::cout << "Row substitution:\n";
    row_subs();
    column_subs();

    return 0;
}