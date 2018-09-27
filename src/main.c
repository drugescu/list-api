/*
 * main.c - Practice 1
 *
 * 2018 drugescu <drugescu@drugescu-VirtualBox>
 *
 */

#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <assert.h>

#define NUM_THREADS     3

pthread_t threads[ NUM_THREADS ];

struct list_node;

typedef void (*callback_print)(struct list_node* list);

typedef struct list_node {
    struct list_node *next;
    callback_print callback;
    int val;
} list_node_t;

pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t threads_initialized;

list_node_t *head = NULL;

void pf(list_node_t *item) {
    printf("%d", item->val);
}

// Adds a new element to the end of the list
void add_element(int val, int tid) {

    // Hold the list
    pthread_mutex_lock(&list_mutex);

    list_node_t *current = head;

    // If list uninitialized
    if (current == NULL) {
        head = malloc(sizeof(list_node_t));
        assert(head != NULL);
        head->val = val;
        head->callback = pf;
        head->next = NULL;

        printf("[Thread %d] Added first value to list, %d\n", tid, val);
    }
    else {
        // Iterate all through the list
        while (current->next != NULL) {
            current = current->next;
        }

        // Now add the value
        current->next = malloc(sizeof(list_node_t));
        assert(current->next != NULL);
        current->next->val = val;
        current->next->callback = pf;
        current->next->next = NULL;

        printf("[Thread %d] Added value to list, %d\n", tid, val);
    }

    // Release the list
    pthread_mutex_unlock(&list_mutex);
}

// Delete by value from list
void delete_element(int val, int tid) {

    // Lock the list
    pthread_mutex_lock(&list_mutex);

    list_node_t *current = head;
    list_node_t *temp_next = NULL;
    list_node_t *temp_prev = NULL;

    while (current != NULL) {

        // Erase element if found
        if (val == current->val) {

            // Save the current pointers
            temp_next = current->next;

            // Figure out case
            int isHead = (current == head);
            int isTail = (current->next == NULL) & (!isHead);
            int isMid = (!isHead) && (current->next != NULL);

            // If head, just delete and reset head to next
            if (isHead) {
                head = temp_next;
                printf("[Thread %d] Deleted head (%d), head now points to %d\n", tid, val, head->val);
            }
            // If middle, reconnect nodes
            if (isMid) {
                temp_prev->next = temp_next;
                printf("[Thread %d] Deleted mid (%d), %d now points to %d\n", tid, val, temp_prev->val, temp_next->val);
            }
            // If node is at tail, remove it, point previous to NULL
            if (current->next == NULL) {
                temp_prev->next = NULL;
                printf("[Thread %d] Deleted tail (%d), %d now points to NULL\n", tid, val, temp_prev->val);
            }

            // Deallocate
            free(current);
            current = NULL;

            // Only one deletion allowed
            // Release the list and return
            pthread_mutex_unlock(&list_mutex);
            return;
        }

        temp_prev = current;
        current = current->next;
    }

    printf("[Thread %d] Could not find element %d to delete in list.\n", tid, val);

    // Release the list
    pthread_mutex_unlock(&list_mutex);
}

// Count in O(n)
int _count_elements(list_node_t *head) {
    int count = 0;
    list_node_t *current = head;

    // Iterate all through the list
    while (current->next != NULL) {
        current = current->next;
        count++;
    }

    return count;
}

// Modifies <<first>> and <<second>> by splitting <<lst>> equally
void _split_list(list_node_t *lst, list_node_t **first, list_node_t **second) {
    list_node_t *fast = NULL;
    list_node_t *slow = NULL;

    // Split in O(n) instead of O(2n) by counting twice (fast-slow ptr)
    slow = lst;
    fast = lst->next;

    // Fast advances two nodes, slow advances one
    while (fast != NULL) {
        fast = fast->next;
        if (fast != NULL) {
            slow = slow->next;
            fast = fast->next;
        }
    }

    // Slow is before midpoint, split here
    *first = lst;
    *second = slow->next;
    slow->next = NULL;
}

list_node_t *_sorted_merge(list_node_t *first, list_node_t *second) {
    // Point end of first to beginning of second
    list_node_t *result = NULL;

    // Simplest cases
    if (first == NULL)
        return second;
    else if (second == NULL)
        return first;

    // Pick either, or recurse
    if (first->val <= second->val) {
        result = first;
        result->next = _sorted_merge(first->next, second);
    }
    else {
        result = second;
        result->next = _sorted_merge(first, second->next);
    }

    return result;
}

// Prints the list using the callback function of each element
void print_list(list_node_t *head, int tid) {

    // Lock the list
    pthread_mutex_lock(&list_mutex);

    list_node_t *current = head;

    if (head != NULL)
        printf("[Thread %d] Printing list: \n", tid);

    while(current != NULL) {
        printf("          (*) List element: ");
        (current->callback(current));
        printf("\n");
        current = current->next;
    }

    // Release the list
    pthread_mutex_unlock(&list_mutex);
}

// Quick prints a list without locking it (assumes a lock was already set!)
void _print_list_nolock(list_node_t *head, int tid) {
    list_node_t *current = head;

    printf("       [%d] List ( ", tid);

    while (current != NULL) {
        current->callback(current);
        printf(" ");
        current = current->next;
    }

    printf(")\n");
}

void _merge_sort(list_node_t **headRef, int tid) {
    printf ("[Thread %d] Started MergeSort\n", tid);

    list_node_t *head = *headRef;

    // Don't sort empty list or one element list
    int isEmpty = (head == NULL);
    int isLengthOne = (head->next == NULL);
    if (isEmpty || isLengthOne) {
        // Don't forget about the lock
        printf ("[Thread %d] List empty!\n", tid);
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    // Divide list in two halves
    list_node_t *first = NULL;
    list_node_t *second = NULL;

    _split_list(head, &first, &second);
    printf ("[Thread %d] Split list (first, second):\n", tid);
    _print_list_nolock(first, tid);
    _print_list_nolock(second, tid);

    // Sort the two halves
    _merge_sort(&first, tid);
    _merge_sort(&second, tid);

    // Merge the sorted halves
    *headRef = _sorted_merge(first, second);
    printf ("[Thread %d] Merged ordered sublists:\n", tid);
    _print_list_nolock(head, tid);
}

// Merge sort list by pointer rearrangement and slow/fast pointer split midway
void sort_list(list_node_t **headRef, int tid) {
    // Lock the list
    pthread_mutex_lock(&list_mutex);

    _merge_sort(headRef, tid);

    // Release the list
    pthread_mutex_unlock(&list_mutex);
}

// Deletes all elements in list
void flush_list(list_node_t **headRef) {
    list_node_t *current = *headRef;
    list_node_t *next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    // Cleanup real head
    *headRef = NULL;
}

// Per thread operations on linked list
void *work_function(void *thread_id) {
    int tid;

    tid = *((int*) thread_id);

    // Ambiguity in call result for first successful call of pthread_create
    int isHighTID = (tid > NUM_THREADS);
    if (isHighTID)
        tid = 0;

    // Wait for initialization of all threads
    pthread_barrier_wait(&threads_initialized);
    if (tid == 0)
        printf ("|Thread %d| Master barrier reached, syncronized threads!\n", getpid());

    printf ("[Thread %d] Hello world!\n", tid);

    // Operate on list - insert sleeps for round-robin thread scheduler
    if (tid == 1) {
        add_element(11, tid);
        usleep(1);
        add_element(1, tid);
        usleep(1);
        delete_element(11, tid);
        usleep(1);
        add_element(8, tid);
        usleep(1);
        print_list(head, tid);
        usleep(1);
    }
    else if (tid == 2) {
        add_element(30, tid);
        usleep(1);
        add_element(25, tid);
        usleep(1);
        add_element(100, tid);
        usleep(1);
        sort_list(&head, tid);
        usleep(1);
        delete_element(100, tid);
        usleep(1);
    }
    else {
        add_element(2, tid);
        usleep(1);
        add_element(4, tid);
        usleep(1);
        add_element(10, tid);
        usleep(1);
        delete_element(2, tid);
        usleep(1);
        sort_list(&head, tid);
        usleep(1);
        delete_element(10, tid);
        usleep(1);
        delete_element(5, tid);
        usleep(1);
    }

    return NULL;
}

// Initialize thread array
int build_threads()
{
    int thread_args[ NUM_THREADS ];
    int result_code;
    unsigned index;
    int s;

    s = pthread_barrier_init(&threads_initialized, NULL, NUM_THREADS);
    assert(s == 0);

    for (index = 0; index < NUM_THREADS; index++) {
        thread_args[ index ] = index;
        printf("Parent: creating thread %d\n", index);
        result_code = pthread_create(&threads[index], NULL, work_function, &thread_args[index]);
        assert(!result_code);
    }
}

// Join all threads and report
int cleanup()
{
    int result_code;
    unsigned index;

    for (index = 0; index < NUM_THREADS; index++) {
        result_code = pthread_join(threads[index], NULL);
        assert(!result_code);
        printf("Parent: thread %d has joined.\n", index);
    }

    printf("All threads completed successfully!\n");

    print_list(head, getpid());

    printf("Now flushing the list.\n");

    flush_list(&head);

    print_list(head, getpid());

    pthread_barrier_destroy(&threads_initialized);
}

int main(int argc, char **argv)
{
    printf("Main pid is %d\n", getpid());

    build_threads();

    cleanup();

    return 0;
}
