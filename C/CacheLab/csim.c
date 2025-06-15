#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// memory address is composed of 64 bits
typedef unsigned long int mem_address_t;

// struct for hits, misses, evictions, dirty bytes and dirty evictions(in
// cachelab.h) all of these variables are unsigned longs
typedef csim_stats_t csim_stat;

mem_address_t setmask;

// stores all addresses in an array
mem_address_t *address_storage;

// b = number of bits for block offset
int b;
// s = number of bits to represent set
int s;
// keep track of LSU
int track;
// keep track of E
int E;

// check if lookup is active
// check if store is active
bool store;

// set B, S and T
unsigned long int B;
unsigned long int S;
unsigned long int T;
// S = 2^s
// int B;
// B = 2^b

// struct cacheline contains the valid bit, the tag, and LRU
typedef struct cache_struct {
    int valid;
    mem_address_t tag;
    int LRU;
    bool dirtybit;
} cache_line;

// cache line set is a set of cache lines
typedef cache_line *cache_line_set_t;
typedef cache_line_set_t *cache_t;
cache_t cache;

// trace file is the file that is being read
FILE *tracefile;

// number of dirty LINES
unsigned long dirty_lines;

// each of these variables determine if they
// store or not

// HELPER FUNCTION AREA

// enter the address to determine its quality
// if it is already in the cache, increase hits by 1
// if it is not in cache, bring into cache and increase miss by 1
// if a line is being evicted, increase evict count by 1

// DIRTY BYTES ARE A MULTIPLE OF 16
// DIRTY EVICTIONS

void determine(mem_address_t address, csim_stats_t *csim_stats) {
    // seperate tag and set_index
    unsigned long set_index = (address >> b) & (setmask);
    unsigned long tag = (address >> (b + s));

    bool hasbeenhit = false;
    for (int i = 0; i < E; i++) {
        if (cache[set_index][i].valid && cache[set_index][i].tag == tag) {
            // determine that this block has been used
            // set found to 1, which determines hit
            hasbeenhit = true;
            if ((!cache[set_index][i].dirtybit) && (store)) {
                cache[set_index][i].dirtybit = true;
                csim_stats->dirty_bytes += B;
            }
            csim_stats->hits += 1;
            cache[set_index][i].LRU = track++;

            break;
        }
    }
    // MISSES
    if (!hasbeenhit) {
        // set LRU tracker;
        int x = 0;
        // x determines the smallest LRU that is to be replaced
        for (int i = 0; i < E; i++) {
            if (cache[set_index][i].LRU < cache[set_index][x].LRU) {
                x = i;
            }
        }
        // if valid bit is 1, evict
        if (cache[set_index][x].valid) {
            csim_stats->evictions += 1;
            // checks if dirty
            if (cache[set_index][x].dirtybit) {
                csim_stats->dirty_evictions += B;
                csim_stats->dirty_bytes -= B;
            }
        }
        csim_stats->misses += 1;
        cache[set_index][x].valid = true;
        cache[set_index][x].dirtybit = false;
        cache[set_index][x].tag = tag;
        cache[set_index][x].LRU = track;

        if (store) {
            cache[set_index][x].dirtybit = true;
            csim_stats->dirty_bytes += B;
        } else {
            cache[set_index][x].dirtybit = false;
        }
    }
}

// determines if instructions are L or S
void process_trace_file(csim_stats_t *csim_stats) {
    int LINELEN;
    // refers to the length of the line(honestly doesnt do much)
    mem_address_t address;
    // refers to the actual address
    char instruction;
    // refers to the instruction(i.e L, S)
    // dirty bit will be found in scan

    while (fscanf(tracefile, "%c %lx,%d\n", &instruction, &address, &LINELEN) ==
           3) {
        switch (instruction) {
        case 'L':

            store = false;
            determine(address, csim_stats);
            // printf("L %lx,X\n", address);
            break;
        case 'S':

            store = true;
            determine(address, csim_stats);
            // printf("S %lx,X\n", address);
            break;
        default:
            break;
        }
    }

    // gets rid of space on trace file to only get either the op or address

    // fgets: char*, int n, FILE*
    /*char* lineread = NULL;
    int maxlen = 17;
    while(fgets(lineread, maxlen, tracefile) != NULL)
    {
        //seperates the space
        char *tok = strtok(lineread," ");
        //tok in this case is the element of the string which
        if(tok == NULL)
        {
            printf("invalid format");
            continue;
        }
        //printf("tok = %s",tok);
        //16 assumes the address is in hex(which it is)
        mem_address_t address = strtoul(tok,NULL,16);
        //seperates the coma from the line
        char* tok2 = strtok(lineread,",");
        if(tok2 == NULL)
        {
            printf("invalid format");
            continue;
        }
        switch(tok[0])
        {
            case 'L':
            {
                load = 1;
                store = 0;
                determine(address,csim_stats);
                break;
            }
            case 'S':
            {
                load = 0;
                store = 1;
                determine(address,csim_stats);
                break;
            }
            default:
            {
                load = 0;
                store = 0;
                break;
            }
        }
    }
    fclose(tracefile);*/
}

// initiates the cache
int cache_init(int x) {
    cache = malloc(sizeof(cache_line_set_t) * S);
    for (unsigned long i = 0; i < S; i++) {
        cache[i] = malloc(sizeof(cache_line) * (unsigned)E);
        for (int j = 0; j < E; j++) {
            // set valid bit to invalid
            cache[i][j].valid = false;
            cache[i][j].tag = 0;
            cache[i][j].LRU = 0;
            cache[i][j].dirtybit = false;
        }
    }
    return x;
}

int help(int x) {
    printf("Usage: ./csim -ref [-v] -s <s> -E <E> -b <b> -t <trace>\n"
           "./csim -ref -h\n"
           "-h Print this help message and exit\n"
           "-v Verbose mode: report effects of each memory operation\n"
           "-s <s> Number of set index bits (there are 2**s sets)\n"
           "-b <b> Number of block bits (there are 2**b blocks)\n"
           "-E <E> Number of lines per set ( associativity )\n"
           "-t <trace > File name of the memory trace to process\n");
    return x;
}

// frees all
int free_cache(int x) {
    for (unsigned long i = 0; i < S; i++) {
        free(cache[i]);
    }
    free(cache);
    return x;
}

// acts as 2**y
unsigned long int power(int y) {
    return 1 << y;
}

// the main function
int main(int argc, char **argv) {
    csim_stats_t *csim_stats = malloc(sizeof(csim_stats_t));
    // set all values in csim_stats to 0
    csim_stats->hits = 0;
    csim_stats->misses = 0;
    csim_stats->evictions = 0;
    csim_stats->dirty_bytes = 0;
    csim_stats->dirty_evictions = 0;

    // s = number of set bits
    s = -1;
    // e = associability
    E = 0;
    // b = bit offsets
    b = -1;
    // t = trace file
    char *t = NULL;
    // retrieves command from getopt
    int command;

    while ((command = getopt(argc, argv, "s:E:b:t:h")) != -1) {
        switch (command) {
        case 'h':
            help(0);
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            t = optarg;
            break;
        default:
            // checks for arguments not given
            printf("Mandatory arguments missing.\n");
            help(0);
            break;
        }
    }

    // check if we missed out on s,E,b
    if (s < 0 || E < 1 || b < 0) {
        // return -1 for failure
        printf("s,b,or e are not positive integers");
        return -1;
    }
    // checks if s,b are too large
    if (s + b >= 64) {
        printf("s,b,are too large");
        return -1;
    }

    // check if file cannot be opened
    tracefile = fopen(t, "r");
    // checks if tracefile is null
    if (!tracefile) {
        // return -1 for failure
        printf("file cannot be opened");
        return -1;
    }

    // use E B S to determine if cache is within
    S = power(s);
    B = power(b);
    if (S * B * (unsigned long)(E) > power(63)) {
        printf("S,B,E do not fit in cache");
        return -1;
    }

    setmask = S - 1;

    // initialize cache
    cache_init(0);
    process_trace_file(csim_stats);
    free_cache(0);
    printSummary(csim_stats);
    free(csim_stats);
    // free(cache_struct);
    return 0;

    // parse the trace

    // dirty byte = every time im writing(Store)
    // keep track of dirty line(if there is a dirty block)
    // dirty evict = every time im evicting a dirty byte(dirty lines * B)
}
