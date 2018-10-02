# Practice 1

## Multi-Threaded Single Linked-List API

Implements a single-linked API with the following supported operations:

* add_node - adds a new node to the end of the list
* delete_node - deletes the specified element
* print_list - prints the nodes of the list using a callback_print pointer contained within said node.
* sort_list - re-arranges the list from lower to higher value of nodes using merge-sort.
* flush_list - resets the list (deleting all nodes).

### Threads

The application creates 3 threads which execute the following sequences:

Thread|Operation
------|---------
1|add_node(2)
1|add_node(4)
1|add_node(10)
1|delete_node(2)
1|sort_list()
1|delete_node(10)
1|delete_node(5)
2|add_node(11)
2|add_node(1)
2|delete_node(11)
2|add_node(8)
2|print_list()
3|add_node(30)
3|add_node(25)
3|add_node(100)
3|sort_list()
3|delete_node(100)

### Node information

* Next pointer
* Callback for the print function
* Value

### Further considerations

The threads syncronise after creation with a barrier.
All operations are atomic, with usleep(1) inserted for scheduling illustration.
All operations are printed with the thread_id.
List is flushed from main pid at end, after being displayed.
