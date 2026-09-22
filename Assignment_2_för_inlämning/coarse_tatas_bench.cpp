#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include "benchmark.hpp"
#include <stdlib.h>
#include <atomic>

// David Olmedo, Filip Hellgren
// Coarse TATAS implementation
// Compile this benchmark by running:
// "g++ -Wall -std=c++17 -pthread -O3 coarse_tatas_bench.cpp -o bench.out"
// Run by typing:
// "./bench.out <thread_count>"
// where <thread_count> is the number of threads to use.

// -------------------------------------------------------
// OUR sorted_list IMPLEMENTATION
// -------------------------------------------------------

class TATAS_Lock
{
private:
    std::atomic<bool> state = false;

public:
    void lock()
    {
        while (true)
        {
            while (state == true)
            {
            }
            bool isOurs = !state.exchange(true);
            if (isOurs)
            {
                return;
            }
        }
    }

    void unlock()
    {
        state.store(false);
    }
};

template <typename T>
struct node
{
    T value;
    node<T> *next;
};

template <typename T>
class sorted_list
{
    node<T> *first = nullptr;
    TATAS_Lock lock;

public:
    /* default implementations:
     * default constructor
     * copy constructor (note: shallow copy)
     * move constructor
     * copy assignment operator (note: shallow copy)
     * move assignment operator
     *
     * The first is required due to the others,
     * which are explicitly listed due to the rule of five.
     */
    sorted_list() = default;
    sorted_list(const sorted_list<T> &other) = default;
    sorted_list(sorted_list<T> &&other) = default;
    sorted_list<T> &operator=(const sorted_list<T> &other) = default;
    sorted_list<T> &operator=(sorted_list<T> &&other) = default;
    ~sorted_list()
    {
        while (first != nullptr)
        {
            remove(first->value);
        }
    }

    /* insert v into the list */
    void insert(T v)
    {
        lock.lock();
        node<T> *pred = nullptr;
        node<T> *succ = first;
        while (succ != nullptr && succ->value < v)
        {
            pred = succ;
            succ = succ->next;
        }

        /* construct new node */
        node<T> *current = new node<T>();
        current->value = v;

        /* insert new node between pred and succ */
        current->next = succ;
        if (pred == nullptr)
        {
            first = current;
        }
        else
        {
            pred->next = current;
        }
        lock.unlock();
    }

    void remove(T v)
    {
        lock.lock();
        /* first find position */
        node<T> *pred = nullptr;
        node<T> *current = first;
        while (current != nullptr && current->value < v)
        {
            pred = current;
            current = current->next;
        }
        if (current == nullptr || current->value != v)
        {
            /* v not found */
            lock.unlock();
            return;
        }
        /* remove current */
        if (pred == nullptr)
        {
            first = current->next;
        }
        else
        {
            pred->next = current->next;
        }
        delete current;
        lock.unlock();
        return;
    }

    /* count elements with value v in the list */
    std::size_t count(T v)
    {
        lock.lock();
        std::size_t cnt = 0;
        /* first go to value v */
        node<T> *current = first;
        while (current != nullptr && current->value < v)
        {
            current = current->next;
        }
        /* count elements */
        while (current != nullptr && current->value == v)
        {
            cnt++;
            current = current->next;
        }
        lock.unlock();
        return cnt;
    }
};

// -------------------------------------------------------
// BENCHMARK CODE BELOW HERE (untouched by us)
// -------------------------------------------------------

static const int DATA_VALUE_RANGE_MIN = 0;
static const int DATA_VALUE_RANGE_MAX = 256;
static const int DATA_PREFILL = 512;

template <typename List>
void read(List &l, int random)
{
    /* read operations: 100% count */
    l.count(random % DATA_VALUE_RANGE_MAX);
}

template <typename List>
void update(List &l, int random)
{
    /* update operations: 50% insert, 50% remove */
    auto choice = (random % (2 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
    if (choice == 0)
    {
        l.insert(random % DATA_VALUE_RANGE_MAX);
    }
    else
    {
        l.remove(random % DATA_VALUE_RANGE_MAX);
    }
}

template <typename List>
void mixed(List &l, int random)
{
    /* mixed operations: 6.25% update, 93.75% count */
    auto choice = (random % (32 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
    if (choice == 0)
    {
        l.insert(random % DATA_VALUE_RANGE_MAX);
    }
    else if (choice == 1)
    {
        l.remove(random % DATA_VALUE_RANGE_MAX);
    }
    else
    {
        l.count(random % DATA_VALUE_RANGE_MAX);
    }
}

int main(int argc, char *argv[])
{
    /* get number of threads from command line */
    if (argc < 2)
    {
        std::cerr << u8"Please specify number of worker threads: " << argv[0] << u8" <number>\n";
        std::exit(EXIT_FAILURE);
    }
    std::istringstream ss(argv[1]);
    int threadcnt;
    if (!(ss >> threadcnt))
    {
        std::cerr << u8"Invalid number of threads '" << argv[1] << u8"'\n";
        std::exit(EXIT_FAILURE);
    }
    /* set up random number generator */
    std::random_device rd;
    std::mt19937 engine(rd());
    std::uniform_int_distribution<int> uniform_dist(DATA_VALUE_RANGE_MIN, DATA_VALUE_RANGE_MAX);

    /* example use of benchmarking */
    {
        sorted_list<int> l1;
        /* prefill list with 1024 elements */
        for (int i = 0; i < DATA_PREFILL; i++)
        {
            l1.insert(uniform_dist(engine));
        }
        benchmark(threadcnt, u8"coarse TATAS read", [&l1](int random)
                  { read(l1, random); });
        benchmark(threadcnt, u8"coarse TATAS update", [&l1](int random)
                  { update(l1, random); });
    }
    {
        /* start with fresh list: update test left list in random size */
        sorted_list<int> l1;
        /* prefill list with 1024 elements */
        for (int i = 0; i < DATA_PREFILL; i++)
        {
            l1.insert(uniform_dist(engine));
        }
        benchmark(threadcnt, u8"coarse TATAS mixed", [&l1](int random)
                  { mixed(l1, random); });
    }
    return EXIT_SUCCESS;
}
