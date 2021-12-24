# Malloc-Custom-Implementation

This program demonstrates heap memory management by custom implementation of malloc and free. The heap is implemented as a doubly linked list.
Blocks that are larger than requested memory are split to avoid ***internal fragmentation*** whereas adjacent free blockes are combined to a single large block to avoid ***external fragmentation***.\

Four implementations of malloc are avaialble using four .so files:

1. First Fit (libmalloc-ff.so)\
2. Next Fit (libmalloc-nf.so)\
3. Best Fit (libmalloc-bf.so)\
4. Worst Fit (libmalloc-wf.so)

The .so have to be used to override the system's default malloc function. Test files in C are provided in the /tests folder that call malloc and free.

***The code can be complied in the following way:\
$ make\
$ env LD_PRELOAD=lib/libmalloc-ff.so tests/<testfile>***
