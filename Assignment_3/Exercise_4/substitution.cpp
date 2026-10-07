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

void set_schedule(omp_sched_t schedule, int chunk_size)
{
    omp_set_schedule(schedule, chunk_size);
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

void row_subs(const char *schedule_name)
{
    system_t sys = generate_system();
    std::cout << "System generated\n";
    auto &A = sys.A;
    auto &b = sys.b;
    auto &x = sys.x;

    auto start = std::chrono::steady_clock::now();
    for (int row = VARS - 1; row >= 0; row--)
    {
        x[row] = b[row];
        int sum = 0;
#pragma omp parallel for shared(A, x, row) reduction(+ : sum) schedule(runtime)
        for (int col = row + 1; col < VARS; col++)
        {
            sum += A[row][col] * x[col];
        }

        x[row] -= sum;
        x[row] /= A[row][row];
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << "Row substitution " << schedule_name << ":\n";
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Thread count: " << omp_get_max_threads() << " System size: " << VARS << "\n";
    std::cout << "Elapsed time: " << duration.count() << " ms\n\n";
}

void row_subs_static(int chunk_size)
{
    set_schedule(omp_sched_static, chunk_size);
    row_subs("static");
}

void row_subs_dynamic(int chunk_size)
{
    set_schedule(omp_sched_dynamic, chunk_size);
    row_subs("dynamic");
}

void row_subs_guided(int chunk_size)
{
    set_schedule(omp_sched_guided, chunk_size);
    row_subs("guided");
}

void row_subs_auto(int chunk_size)
{
    set_schedule(omp_sched_auto, chunk_size);
    row_subs("auto");

    omp_sched_t my_sched;
    int size;

    omp_get_schedule(&my_sched, &size);

    std::cout << "Chosen schedule: " << my_sched << "Size: " << size << "\n";
}

void column_subs(const char *schedule_name)
{
    system_t sys = generate_system();
    std::cout << "System generated\n";
    auto &A = sys.A;
    auto &b = sys.b;
    auto &x = sys.x;

    auto start = std::chrono::steady_clock::now();
    for (int row = 0; row < VARS; row++)
    {
        x[row] = b[row];
    }
    for (int col = VARS - 1; col >= 0; col--)
    {
        x[col] /= A[col][col];
#pragma omp parallel for shared(x, A, col) schedule(runtime)
        for (int row = 0; row < col; row++)
            x[row] -= A[row][col] * x[col];
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << "Column substitution " << schedule_name << ":\n";
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Thread count: " << omp_get_max_threads() << " System size: " << VARS << "\n";
    std::cout << "Elapsed time: " << duration.count() << " ms\n\n";
}

void column_subs_static(int chunk_size)
{
    set_schedule(omp_sched_static, chunk_size);
    column_subs("static");
}

void column_subs_dynamic(int chunk_size)
{
    set_schedule(omp_sched_dynamic, chunk_size);
    column_subs("dynamic");
}

void column_subs_guided(int chunk_size)
{
    set_schedule(omp_sched_guided, chunk_size);
    column_subs("guided");
}

void column_subs_auto(int chunk_size)
{
    set_schedule(omp_sched_auto, chunk_size);
    column_subs("auto");

    omp_sched_t my_sched;
    int size;

    omp_get_schedule(&my_sched, &size);

    std::cout << "Chosen schedule: " << my_sched << "Size: " << size << "\n";
}

void run_columns(int chunk_size)
{
    column_subs_static(chunk_size);
    column_subs_dynamic(chunk_size);
    column_subs_guided(chunk_size);
    column_subs_auto(chunk_size);

    std::cout << "Column chunk size: " << chunk_size << "\n\n";
}

void run_rows(int chunk_size)
{
    row_subs_static(chunk_size);
    row_subs_dynamic(chunk_size);
    row_subs_guided(chunk_size);
    row_subs_auto(chunk_size);

    std::cout << "Row chunk size: " << chunk_size << "\n\n";
}

int main(int argc, char *argv[])
{
    run_columns(0);
    run_rows(0);
    return 0;
}