#include <atomic>

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
