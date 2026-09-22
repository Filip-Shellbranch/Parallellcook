#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include "benchmark.hpp"

#include <stdlib.h>
#include <mutex>

// David Olmedo, Filip Hellgren
// Fine Mutex implementation
// Compile this benchmark by running:
// "g++ -Wall -std=c++17 -pthread -O3 fine_mutex_bench.cpp -o bench.out"
// Run by typing:
// "./bench.out <thread_count>"
// where <thread_count> is the number of threads to use.

// -------------------------------------------------------
// OUR sorted_list IMPLEMENTATION
// -------------------------------------------------------

template <typename T>
struct node
{
    T value;
    node<T> *next = nullptr;
    std::mutex lock;
};

template <typename T>
class sorted_list
{
    node<T> *first = nullptr;
    std::mutex first_lock;

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
        node<T> *curr = first;
        while (curr != nullptr)
        {
            node<T> *next = curr->next;
            delete curr;
            curr = next;
        }
    }

    // insert v into the list
    void insert(T v)
    {
        node<T> *new_node = new node<T>();
        new_node->value = v;

        first_lock.lock();
        // If empty list or less than first, set first to the new node
        if (first == nullptr || v < first->value)
        {
            new_node->next = first;
            first = new_node;
            first_lock.unlock();
            return;
        }

        node<T> *pred = first;
        pred->lock.lock();
        first_lock.unlock();

        node<T> *succ = pred->next;
        if (succ != nullptr)
        {
            succ->lock.lock();
        }

        while (succ != nullptr && succ->value < v)
        {
            pred->lock.unlock();
            pred = succ;
            succ = pred->next;
            if (succ != nullptr)
            {
                succ->lock.lock();
            }
        }

        // Create new node between pred and succ.
        new_node->next = succ;
        pred->next = new_node;

        pred->lock.unlock();
        if (succ != nullptr)
        {
            succ->lock.unlock();
        }
    }

    void remove(T v)
    {
        first_lock.lock();
        if (first == nullptr)
        {
            first_lock.unlock();
            return;
        }

        node<T> *current = first;
        current->lock.lock();

        // If first is the removed value.
        if (current->value == v)
        {
            first = current->next;
            current->lock.unlock();
            first_lock.unlock();

            delete current;
            return;
        }

        first_lock.unlock();

        node<T> *pred = current;
        current = pred->next;
        if (current != nullptr)
        {
            current->lock.lock();
        }
        while (current != nullptr && current->value < v)
        {
            pred->lock.unlock();
            pred = current;
            current = pred->next;
            if (current != nullptr)
            {
                current->lock.lock();
            }
        }

        if (current != nullptr && current->value == v)
        {
            pred->next = current->next;
            pred->lock.unlock();
            current->lock.unlock();
            delete current;
            return;
        }
        pred->lock.unlock();
        if (current != nullptr)
        {
            current->lock.unlock();
        }
    }

    // count elements with value v in the list
    std::size_t count(T v)
    {
        first_lock.lock();
        if (first == nullptr)
        {
            first_lock.unlock();
            return 0;
        }

        node<T> *current = first;
        current->lock.lock();
        first_lock.unlock();

        // first go to value v
        while (current != nullptr && current->value < v)
        {
            node<T> *next_node = current->next;
            if (next_node != nullptr)
            {
                next_node->lock.lock();
            }
            current->lock.unlock();
            current = next_node;
        }

        std::size_t cnt = 0;
        // count elements
        while (current != nullptr && current->value == v)
        {
            cnt++;
            node<T> *next_node = current->next;
            if (next_node != nullptr)
            {
                next_node->lock.lock();
            }
            current->lock.unlock();
            current = next_node;
        }
        if (current != nullptr)
        {
            current->lock.unlock();
        }
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
        benchmark(threadcnt, u8"Fine Mutex read", [&l1](int random)
                  { read(l1, random); });
        benchmark(threadcnt, u8"Fine Mutex update", [&l1](int random)
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
        benchmark(threadcnt, u8"Fine Mutex mixed", [&l1](int random)
                  { mixed(l1, random); });
    }
    return EXIT_SUCCESS;
}
