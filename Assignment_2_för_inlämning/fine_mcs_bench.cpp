#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include "benchmark.hpp"

#include <atomic>

// David Olmedo, Filip Hellgren
// MCS Queue implementation
// Compile this benchmark by running:
// "g++ -Wall -std=c++17 -pthread -O3 fine_mcs_bench.cpp -o bench.out"
// Run by typing:
// "./bench.out <thread_count>"
// where <thread_count> is the number of threads to use.

// -------------------------------------------------------
// OUR sorted_list IMPLEMENTATION
// -------------------------------------------------------

class MCS_Node
{
public:
    std::atomic<bool> locked = false;
    std::atomic<MCS_Node *> next = nullptr;
};

class MCS_Lock
{
private:
    std::atomic<MCS_Node *> tail = nullptr;

public:
    void lock(MCS_Node *new_node)
    {
        new_node->locked.store(false);
        new_node->next.store(nullptr);
        MCS_Node *pred = tail.exchange(new_node);
        if (pred != nullptr)
        {
            new_node->locked.store(true);
            pred->next.store(new_node);
            while (new_node->locked.load())
            {
            }
        }
    }

    void unlock(MCS_Node *node)
    {
        MCS_Node *succ = node->next.load();
        if (succ == nullptr)
        {
            // We have no new node in next, however still check tail.

            MCS_Node *expected = node;
            // If (tail == our node) then we are the last node, set tail to null
            if (tail.compare_exchange_strong(expected, nullptr))
            {
                return;
            }

            // Tail was not our node, so something was in queue,
            // wait until the next field in our node is updated.
            while ((succ = node->next.load()) == nullptr)
            {
            }
        }

        // Release the lock
        succ->locked.store(false);
    }
};
template <typename T>
struct node
{
    T value;
    node<T> *next = nullptr;
    MCS_Lock lock;
};

template <typename T>
class sorted_list
{
    node<T> *first = nullptr;
    MCS_Lock first_lock;

public:
    /* default implementations:
     * default constructor
     * copy constructor (note: shallow copy)
     * move constructor
     * copy assignment operator (note: shallow copy)
     * move assignment operator
     *
     * The first is required due to the others,
     * which are explicitly listed due to the rule of five.*/
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

        MCS_Node mcs_first, mcs_a, mcs_b;
        MCS_Node *mcs_pred = &mcs_a, *mcs_succ = &mcs_b;

        first_lock.lock(&mcs_first);
        // If empty list or less than first, set first to the new node
        if (first == nullptr || v < first->value)
        {
            new_node->next = first;
            first = new_node;
            first_lock.unlock(&mcs_first);
            return;
        }

        node<T> *pred = first;
        pred->lock.lock(mcs_pred);
        first_lock.unlock(&mcs_first);

        node<T> *succ = pred->next;
        if (succ != nullptr)
        {
            succ->lock.lock(mcs_succ);
        }

        while (succ != nullptr && succ->value < v)
        {
            pred->lock.unlock(mcs_pred);
            pred = succ;

            // Swap pred and succ locks.
            MCS_Node *tmp = mcs_pred;
            mcs_pred = mcs_succ;
            mcs_succ = tmp;

            succ = pred->next;
            if (succ != nullptr)
            {
                succ->lock.lock(mcs_succ);
            }
        }

        // Create new node between pred and succ.
        new_node->next = succ;
        pred->next = new_node;

        pred->lock.unlock(mcs_pred);
        if (succ != nullptr)
        {
            succ->lock.unlock(mcs_succ);
        }
    }

    void remove(T v)
    {
        MCS_Node mcs_first, mcs_a, mcs_b;
        MCS_Node *mcs_pred = &mcs_a, *mcs_succ = &mcs_b;

        first_lock.lock(&mcs_first);
        if (first == nullptr)
        {
            first_lock.unlock(&mcs_first);
            return;
        }

        node<T> *current = first;
        current->lock.lock(mcs_pred);

        // If first is the removed value.
        if (current->value == v)
        {
            first = current->next;
            current->lock.unlock(mcs_pred);
            first_lock.unlock(&mcs_first);

            delete current;
            return;
        }

        first_lock.unlock(&mcs_first);

        node<T> *pred = current; // mcs_pred holds pred's lock
        current = pred->next;
        if (current != nullptr)
        {
            current->lock.lock(mcs_succ);
        }

        while (current != nullptr && current->value < v)
        {
            pred->lock.unlock(mcs_pred);
            pred = current;

            MCS_Node *tmp = mcs_pred;
            mcs_pred = mcs_succ;
            mcs_succ = tmp;

            current = pred->next;
            if (current != nullptr)
            {
                current->lock.lock(mcs_succ);
            }
        }

        if (current != nullptr && current->value == v)
        {
            pred->next = current->next;
            current->lock.unlock(mcs_succ);
            pred->lock.unlock(mcs_pred);
            delete current;
            return;
        }
        pred->lock.unlock(mcs_pred);
        if (current != nullptr)
        {
            current->lock.unlock(mcs_succ);
        }
    }

    // count elements with value v in the list
    std::size_t count(T v)
    {
        MCS_Node mcs_first, mcs_a, mcs_b;
        MCS_Node *mcs_curr = &mcs_a, *mcs_next = &mcs_b;

        first_lock.lock(&mcs_first);
        if (first == nullptr)
        {
            first_lock.unlock(&mcs_first);
            return 0;
        }

        node<T> *current = first;
        current->lock.lock(mcs_curr);
        first_lock.unlock(&mcs_first);

        // first go to value v
        while (current != nullptr && current->value < v)
        {
            node<T> *next_node = current->next;
            if (next_node != nullptr)
            {
                next_node->lock.lock(mcs_next);
            }
            current->lock.unlock(mcs_curr);
            current = next_node;

            MCS_Node *tmp = mcs_curr;
            mcs_curr = mcs_next;
            mcs_next = tmp;
        }

        std::size_t cnt = 0;
        // count elements
        while (current != nullptr && current->value == v)
        {
            cnt++;
            node<T> *next_node = current->next;
            if (next_node != nullptr)
            {
                next_node->lock.lock(mcs_next);
            }
            current->lock.unlock(mcs_curr);
            current = next_node;

            MCS_Node *tmp = mcs_curr;
            mcs_curr = mcs_next;
            mcs_next = tmp;
        }
        if (current != nullptr)
        {
            current->lock.unlock(mcs_curr);
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
        benchmark(threadcnt, u8"Fine MCS Queue read", [&l1](int random)
                  { read(l1, random); });
        benchmark(threadcnt, u8"Fine MCS Queue update", [&l1](int random)
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
        benchmark(threadcnt, u8"Fine MCS Queue mixed", [&l1](int random)
                  { mixed(l1, random); });
    }
    return EXIT_SUCCESS;
}
