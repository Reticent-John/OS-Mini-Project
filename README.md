# Producer-Consumer Multithreaded Application
In this project we created a multithreaded Producer-Consumer simulation written in C code using the POSIX `pthread` library, mutexes, and semaphores as mentioned in the rubric. It uses two circular bounded buffers (urgent and normal) to co-ordinate producer and consumer threads safely with priority handling, and it terminates consumers cleanly using the Poison Pill technique.

## Overiew of the project
The program creates a modifiable amount of producer and consumer threads. Each producer thread generates a fixed number of random integer items with assigned priorities (urgent or normal) and inserts them into the appropriate circular buffer. Consumers prioritize urgent items, consuming them before normal ones while maintaining FIFO order within each priority level. The assignment requires a circular queue, semaphore blocking for both full and empty conditions, mutex-based mutual exclusion, and graceful termination using 1 poison pill per consumer.

## Features added
- Circular bounded buffer implementation with separate urgent and normal buffers. 
- Priority handling: items are assigned urgent (priority 1) or normal (priority 0) priority, with consumers processing urgent items first.
- Configurable number of producers, consumers, and buffer size via command-line arguments. 
- Synchronization using semaphores for both empty and full slots on each buffer. 
- Using mutexes to prevent race conditions. 
- Graceful termination using the Poison Pill technique. 
- Input validation check for invalid producer, consumer, and buffer values.
- Latency and throughput reports to measure the performance. 

## File Name
Save the source code as:
```bash
producer_consumer_main.c
```

## Requirements to run this
This project is intended to run on a Debian Linux environment since it uses POSIX threads and semaphores. A GCC compiler is required to build this program. Da project instructions specify the C code language with the POSIX `pthread` library and show compiling using GCC with the `-pthread` flag. This helps us time the compilation of our code, allowing us to visualize its effeciency.

Install the required tools on Ubuntu Linux with:

```bash
sudo apt update
sudo apt install build-essential
```

## Compilation
Open a terminal in the folder containing `producer_consumer.c` and compile the program with:
```bash
gcc -o producer_consumer producer_consumer.c -pthread
```

This command compiles the source file `producer_consumer.c` and creates an executable file named `producer_consumer`. The `-pthread` flag enables support for POSIX threads, which are required for `pthread_create`, `pthread_join`, mutexes, and semaphore-related threading behavior used by the project. 

## How to Run
Run the program using:
```bash
./producer_consumer <num_producers> <num_consumers> <buffer_size>
```

Example:
```bash
./producer_consumer 3 2 10
```

This example starts with 3 producer threads, 2 consumer threads, and creates a circular buffer with 10 slots for them. 

## Command-Line Arguments used
- `num_producers`: Shows number of producer threads to create.
- `num_consumers`: Shows number of consumer threads to create.
- `buffer_size`: Shows number of slots in the circular bounded buffer.

This program validates these inputs and rejects invalid values. In the current implementation, the accepted ranges are from 1 to 100 producers, consumers, and for buffer size. These checks satisfy the assignment requirement for input validation and graceful error handling. 

## Program Logic
### Producer behavior
Each producer thread generates a total of 20 random integer items. Before inserting an item, the producer first waits on the `empty` semaphore and checks if the buffer is full. Once a slot is available, the producer then locks the buffer mutex, inserts the item at the current circular index, then records the enqueue timestamp, updates the `in` pointer, unlocks the mutex, and signals the `full` semaphore. The assignment requires producers to block when the buffer is full and to produce a fixed number of items. 

### Consumer behavior
Each consumer waits on the `full` semaphore if the buffer is empty. When an item is available, the consumer locks the buffer mutex, then it removes the item from the current circular index, records the dequeue timestamp, updates the `out` pointer, then unlocks the mutex, and lastly signals the `empty` semaphore. The consumer then prints the consumed item and updates via shared statistics. The assignment requires consumers to block when the buffer is empty and avoid busy waiting. 

### Poison Pill termination
After all producer threads finish and are joined by the main thread, the main thread inserts one `POISON_PILL` value for every individual consumer. When a consumer removes a poison pill, it exits its loop and terminates. This is the exact GRACEFUL termination mechanism that required in the project document. 

### Performance metrics
The program measures enqueue and dequeue timestamps for items and also reports average latency and overall throughput at the end of execution. here the assignment lists latency and throughput metrics as an optional bonus feature and asks for two runs with different buffer sizes for comparison. 

## Priority Handling

This implementation supports item priorities:
- **Urgent items (priority 1):** Inserted into a dedicated urgent buffer. Consumers always check and consume urgent items first, maintaining FIFO order among urgent items.
- **Normal items (priority 0):** Inserted into a separate normal buffer. These are only consumed when no urgent items are present, also in FIFO order.
- **Producer assignment:** Each item produced has a 25% chance of being urgent (priority 1) and 75% chance of being normal (priority 0).
- **Poison pills:** Inserted as urgent items to guarantee prompt consumer termination.

This design ensures that urgent work is always prioritized, while normal work is not starved and FIFO is preserved within each priority class.

### Example Output (with priorities)
```
[Producer - 1] Produced the Item 42 (priority 1) at index 0
[Producer - 2] Produced the Item 17 (priority 0) at index 0
[Consumer - 1] Consumed the item 42 at index 0
[Consumer - 1] Consumed the item 17 at index 0
```
Urgent items are always consumed before normal items, even if produced later.

## Output
Run:
```text

=== Producer-Consumer Simulation ===
Producers: 3, Consumers: 2, Buffer Size: 10
Each producer will generate 20 items
=====================================

[Producer-1] Produced Item 27 at index 0
[Producer-1] Produced Item 99 at index 1
[Producer-1] Produced Item 26 at index 2
[Producer-1] Produced Item 57 at index 3
[Producer-1] Produced Item 56 at index 4
[Producer-1] Produced Item 52 at index 5
[Producer-1] Produced Item 81 at index 6
...
[Consumer-1] Consumed item 28 at index 7
[Consumer-1] Consumed item 47 at index 8
[Consumer-1] Consumed item 74 at index 9

=== All producers finished ===
Inserting 2 poison pills...

[Consumer-2] Received poison pill. Exiting.
[Consumer-2] Finished consuming 30 items.
[Consumer-1] Received poison pill. Exiting.
[Consumer-1] Finished consuming 30 items.

=== Simulation Complete ===
Total items produced: 60
Total items consumed: 60
Expected items: 60

=== Latency Statistics ===
Average latency: 0.031 ms
Throughput: 175947.40 items/sec
```
The exact item values, interleaving order, latency, and throughput will vary from run to run becuz thread scheduling and random number generation are nondeterministic.

## Test Cases
Run this test case to verify correctness:

```bash
./producer_consumer 3 2 10
```

## Error Handling
The program should display error messages if the following conditions occur bro:

- The wrong number of command-line arguments is provided.
- The number of producers is invalid.
- The number of consumers is invalid.
- The buffer size is invalid.
- Memory allocation fails.
- Thread or synchronization primitive initialization fails.

Input validation and error handling are required deliverables in the project specification. 

## Clean Termination and Resource Cleanup

At the end of execution, this program releases dynamically allocated memory, destroys the semaphores and the mutexes. This satisfies the requirement to ensure all resources are appropriately and properly released after producers and consumers terminate.

## Quick Start
```bash
sudo apt update
sudo apt install build-essential
git clone https://github.com/Akil-Qayem/OS-Mini-Project.git
gcc -o producer_consumer producer_consumer.c -pthread
./producer_consumer 3 2 10
```

## Summary of Deliverable
This README covers all of da following: compilation, execution, testing, expected behavior, input parameters, and output examples, which are explicitly required in the deliverables section of the project handout.
