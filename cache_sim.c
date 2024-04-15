#include<stdlib.h>
#include<stdio.h>
#include<omp.h>
#include<string.h>
#include<ctype.h>

typedef char byte;

#define CACHE_SIZE 2

struct cache {
    byte address; // This is the address in memory.
    byte value; // This is the value stored in cached memory.
    // State for MESI protocol.
    byte state; // 0: Invalid, 1: Modified, 2: Exclusive, 3: Shared
};

struct decoded_inst {
    int type; // 0 is RD, 1 is WR
    byte address;
    byte value; // Only used for WR 
};

typedef struct cache cache;
typedef struct decoded_inst decoded;


// Global memory
byte * memory;


// Decode instruction lines
decoded decode_inst_line(char * buffer){
    decoded inst;
    char inst_type[3];
    sscanf(buffer, "%s", inst_type);
    if(!strcmp(inst_type, "RD")){
        inst.type = 0;
        int addr = 0;
        sscanf(buffer, "%s %d", inst_type, &addr);
        inst.value = -1;
        inst.address = addr;
    } else if(!strcmp(inst_type, "WR")){
        inst.type = 1;
        int addr = 0;
        int val = 0;
        sscanf(buffer, "%s %d %d", inst_type, &addr, &val);
        inst.address = addr;
        inst.value = val;
    }
    return inst;
}

// Helper function to print the cachelines
void print_cachelines(cache * c, int cache_size){
    for(int i = 0; i < cache_size; i++){
        cache cacheline = *(c+i);
        printf("Address: %d, State: %d, Value: %d\n", cacheline.address, cacheline.state, cacheline.value);
    }
}

// This function implements the mock CPU loop that reads and writes data.
void cpu_loop(int thread_num, char* input_file){
    // Initialize a CPU level cache that holds about 2 bytes of data.
    cache * c = (cache *) malloc(sizeof(cache) * CACHE_SIZE);
    
    // Read Input file
    FILE * inst_file = fopen(input_file, "r");
    char inst_line[20];
    // Decode instructions and execute them.
    while (fgets(inst_line, sizeof(inst_line), inst_file)){
        decoded inst = decode_inst_line(inst_line);
        /*
         * Cache Replacement Algorithm
         */
        int hash = inst.address % CACHE_SIZE;
        cache cacheline = *(c + hash);
        
        // MESI protocol
        switch (cacheline.state) {
            case 0: // Invalid
                cacheline.address = inst.address;
                cacheline.state = 2; // Set state to Exclusive
                cacheline.value = memory[inst.address];
                break;
            case 1: // Modified
                memory[cacheline.address] = cacheline.value; // Write back to memory
                // Fall through to Exclusive state
            case 2: // Exclusive
                if (cacheline.address != inst.address) {
                    cacheline.address = inst.address;
                    cacheline.state = 2; // Set state to Exclusive
                    cacheline.value = memory[inst.address];
                }
                break;
            case 3: // Shared
                if (cacheline.address != inst.address) {
                    cacheline.address = inst.address;
                    cacheline.state = 2; // Set state to Exclusive
                    cacheline.value = memory[inst.address];
                }
                break;
        }
        switch(inst.type){
            case 0: // Read
                printf("Thread %d: Reading from address %d: %d\n", thread_num, cacheline.address, cacheline.value);
                break;
            
            case 1: // Write
                printf("Thread %d: Writing to address %d: %d\n", thread_num, cacheline.address, inst.value);
                cacheline.value = inst.value; // Update cache value
                cacheline.state = 1; // Set state to Modified
                break;
        }
        *(c + hash) = cacheline; // Update cache line
    }
    free(c);
    fclose(inst_file);
}

int main(int argc, char * argv[]){
    // Check for correct usage
    if (argc < 2) {
        printf("Usage: %s <input_file_0> <input_file_1> ... <input_file_n>\n", argv[0]);
        return 1;
    }

    // Initialize Global memory
    // Let's assume the memory module holds about 24 bytes of data.
    int memory_size = 24;
    memory = (byte *) malloc(sizeof(byte) * memory_size);

    // Parallelize CPU loop across multiple threads
    #pragma omp parallel
    {
        int num_threads = omp_get_num_threads();
        int thread_num = omp_get_thread_num();
        if (thread_num < argc - 1) {
            cpu_loop(thread_num, argv[thread_num + 1]);
        }
    }

    free(memory);
    return 0;
}
