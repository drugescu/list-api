/*
 * main.c - Practice 1
 *
 * 2018 drugescu <drugescu@drugescu-VirtualBox>
 *
 */

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
    printf("(val : %d)", item->val);
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

// Per thread operations on linked list
void *work_function(void *thread_id) {
    int tid;

    tid = *((int*) thread_id);

    // Ambiguity in call result for first successful call of pthread_create
    if (tid > NUM_THREADS)
        tid = 0;

    // Wait for initialization of all threads
    pthread_barrier_wait(&threads_initialized);

    printf ("[Thread %d] Hello world!\n", tid);

    // Operate on list
    if (tid == 1) {
        add_element(11, tid);
        add_element(1, tid);
        delete_element(11, tid);
        add_element(8, tid);
        print_list(head, tid);
    }
    else if (tid == 2) {
        add_element(30, tid);
        add_element(25, tid);
        add_element(100, tid);
        delete_element(100, tid);
        print_list(head, tid);
    }
    else {
        add_element(2, tid);
        add_element(4, tid);
        add_element(10, tid);
        delete_element(2, tid);
        delete_element(10, tid);
        delete_element(5, tid);
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

    pthread_barrier_destroy(&threads_initialized);
}

int main(int argc, char **argv)
{
    printf("Main pid is %d\n", getpid());

    build_threads();

    cleanup();

    return 0;
}
