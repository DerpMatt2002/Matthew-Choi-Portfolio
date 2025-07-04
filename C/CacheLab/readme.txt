This is the handout directory for the 15-213 Cache Lab.

************************
Running the autograders:
************************

Before running the autograders, compile your code:
    linux> make

Check the correctness of your simulator:
    linux> ./test-csim

Check the correctness and performance of your transpose functions:
    linux> ./test-trans -M 32 -N 32
    linux> ./test-trans -M 1024 -N 1024

Check everything at once (this is the program that Autolab runs):
    linux> ./driver.py

******
Files:
******

# You will handing in these two files
csim.c                  Your cache simulator [You must create this file]
trans.c                 Your transpose function(s) [Starter version included]

# Tools for evaluating your simulator and transpose function
Makefile                Builds the simulator and tools
README                  This file
cachelab.c              Required helper functions
cachelab.h              Required header file
csim-ref*               The executable reference cache simulator
driver.py*              The cache lab driver program, runs test-csim and test-trans
test-csim.c             Tests your cache simulator
test-trans.c            Tests your transpose function
ct/                     Code to support address tracing when running the transpose code
tracegen-ct.c           Helper program used by test-trans, which you can run directly.
traces-driver.py        The driver to test the traces you write
traces/                 All trace files used in cachelab
traces/traces           Trace you write for the traces portion of the assignment
traces/csim             Traces used by the driver to test your cache simulator (Part A)

