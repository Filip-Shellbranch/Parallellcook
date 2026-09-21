#include <stdlib.h>
#include <mutex>

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
    node<T> *next = nullptr;
    TATAS_Lock lock;
};

template <typename T>
class sorted_list
{
    node<T> *first = nullptr;
    TATAS_Lock first_lock;

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
