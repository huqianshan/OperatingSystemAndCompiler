#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <sys/jiffies.h>



typedef unsigned int word_t;
#define KB_SHIFT 10
#define KZALLOC_MAX_BYTES ((128/sizeof(word_t))<<(KB_SHIFT))
int main()
{

    long NVM_SPACE_SIZE = (long)4 * 1024 * 1024 * 1024;
    long BLOCK_SIZE = 512;
    long BLOCK_NUMBERS;

    // calculate
    //printf("%ld\n%lld\n", __LONG_MAX__, __LONG_LONG_MAX__);
    BLOCK_NUMBERS = NVM_SPACE_SIZE / BLOCK_SIZE;
    printf("Block data space: %ld\n", NVM_SPACE_SIZE);
    printf("Block Size: %ld\n", BLOCK_SIZE);
    printf("Block Num: %ld\n", BLOCK_NUMBERS);

        int BIT_SIZE = sizeof(int);
        int BIT_MAP_SIZE = BLOCK_NUMBERS * BIT_SIZE ;
        printf("\nBit size: %d", BIT_SIZE);
        printf("Bit map size in bytes: %d\n", BIT_MAP_SIZE);
        printf("Bit map size in MB: %d\n", BIT_MAP_SIZE/1024/1024);
        

        printf("MapTable size in bytes: %d\n", BIT_MAP_SIZE);
        printf("MapTable size in MB: %d\n", BIT_MAP_SIZE/1024/1024);

        uint32_t s = -1;
        printf("%d\n", s);

        printf("%u\n", KZALLOC_MAX_BYTES);

        unsigned long long current_j = jiffies;
        unsigned long long current_m = jiffies_to_msecs(current_j);
        printf("%llu\n", current_m);
}