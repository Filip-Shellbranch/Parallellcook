#include <cmath>
#include <vector>
#include <bits/chrono.h>
#include <omp.h>
#include <iostream>

#define VARS 3 // Ska testa med 42.000 vars 🤯🦧

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
    std::vector<std::vector<double>> A(VARS, std::vector<double>(VARS));
    std::vector<double> b(VARS);
    std::vector<double> x(VARS);

    for (size_t row = 0; row < VARS; row++)
    {
        x[row] = 0;
        b[row] = row;
        for (size_t col = 0; col < VARS; col++)
        {
            int val = (col >= row) ? row + col + 1 : 0;
            A[row][col] = val;
        }
    }

    sys.A = A;
    sys.b = b;
    sys.x = x;

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
    system_t sys = generate_system();
    std::vector<std::vector<double>> &A = sys.A;
    std::vector<double> &b = sys.b;
    std::vector<double> &x = sys.x;

    print_system(sys);
    for (int row = 0; row < VARS; row++)
    {
        x[row] = b[row];
    }
    for (int col = VARS - 1; col >= 0; col--)
    {
        x[col] /= A[col][col];
        for (int row = 0; row < col; row++)
            x[row] -= A[row][col] * x[col];
    }
    print_result(sys);
}

int main(int argc, char *argv[])
{
    // std::cout << "Row substitution:\n";
    auto start = std::chrono::steady_clock::now();
    row_subs();
    auto end = std::chrono::steady_clock::now();
    // std::cout << "Row substitution time (ms): << start - end\n";

    column_subs();

    return 0;
}