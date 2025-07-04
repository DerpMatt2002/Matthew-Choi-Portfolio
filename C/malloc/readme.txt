#############################
# CS:APP Malloc Lab
# Handout files for students
#############################

***********
Main Files:
***********

mdriver, mdriver-emulate
        Once you've run make, run ./mdriver to test
        your solution.  Run ./mdriver-emulate to make sure your
        solution can handle 64-bit allocations

traces/
        Directory that contains the trace files that the driver uses
        to test your solution. Files with names of the form XXX-short.rep
        contain very short traces that you can use for debugging.

**********************************
Other support files for the driver
**********************************
config.h        Configures the malloc lab driver
clock.{c,h}     Low-level timing functions
fcyc.{c,h}      Function-level timing functions
memlib.{c,h}    Models the heap and sbrk function
stree.{c,h}     Data structure used by the driver to check for
                overlapping allocations
MLabInst.so     Code that combines with LLVM compiler infrastructure
                to enable sparse memory emulation
macro-check.pl  Code to check for disallowed macro definitions
driver.pl       Runs both mdriver and mdriver-emulate and generates
                the autolab result.  (Not included with checkpoint)
calibrate.pl   Code to generate benchmark throughput
throughputs.txt Benchmark throughputs, indexed by CPU type

***********************
Example malloc packages
***********************
mm.c            Implicit-list allocator to use as starting point
mm-naive.c      Fast but extremely memory-inefficient package

*******************************
Building and running the driver
*******************************
To build the driver, type "make" to the shell.

To run the driver on a tiny test trace:

        unix> ./mdriver -V -f traces/malloc.rep

To get a list of the driver flags:

        unix> ./mdriver -h

The -V option prints out helpful tracing information

You can use mdriver-dbg to test your code with the DEBUG preprocessor
flag set to 1. This enables the dbg_* macros such as dbg_printf, which
you can use to print debugging output. It also uses the optimization
level -Og, which helps GDB display more meaningful debug information.

        unix> ./mdriver-dbg

You can use mdriver-emulate to test the correctness of your code in
handling 64-bit addresses:

        unix> ./mdriver-emulate

You should see the exact same utilization numbers as you did with the
regular driver.  No timing is done, and so the time and throughput
numbers show up as zeros.

You can use mdriver-uninit to test your code using MemorySanitizer,
a tool that detects uses of uninitialized memory.

        unix> ./mdriver-uninit

