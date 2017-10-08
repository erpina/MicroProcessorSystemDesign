/*
 ==========================================================================================
 Name        :L1CacheSimulator.c
 Author      :Erpina Satya Raviteja,Arka Kilaru,Meril John
 Description :Simualating L1 Level Cache in Software
 ===========================================================================================
 */

// Standard libraries
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>

#define SILENT 

// Cache Configurations
#ifndef CAPACITY
#define CAPACITY 16 //in KB
#endif

#ifndef LINESIZE
#define LINESIZE 64 // in bytes
#endif

#ifndef ASSOCIATIVITY
#define ASSOCIATIVITY  8
#endif

// Cache Configurations

#define LINES (CAPACITY * 1024 / LINESIZE)
#define SETS  (LINES / ASSOCIATIVITY)
#define ADD_BITS 32


// define commands
#define READ				0
#define WRITE				1

// Input Trace file name
#define MAX_INPUT_FILENAME_SIZE 100


// Cache line structure
typedef struct {
	int LRUposition;
	int tag;
	int validbit;
	int dirtybit;
} cacheline;

cacheline cache[SETS][ASSOCIATIVITY];

/*
 * Func  :  checkTag
 Description  :  function to find whether the given address is in the cache
 */
int checkTag(int index, int tag) {
	int way;
	for (way = 0; way < ASSOCIATIVITY; way++) {
		if (cache[index][way].tag == tag)
			return way;
	}
	return ASSOCIATIVITY;
}

/*
 * Func  :  findLRU
Description: Find the way in a set that is least recently accessed to evict that line
 */
int findLRU(int index) {
	int way;
	for (way = 0; way < ASSOCIATIVITY; way++) {
		if (cache[index][way].LRUposition == 0)
			return way;
	}
	return ASSOCIATIVITY;
}

/*
 * Func  :  updateLRU
Description: Updates the line that we recently used with LRU Position 7

 */
void updateLRU(int index, int myway) {
	int way;
	int myLRUbit = cache[index][myway].LRUposition;

	for (way = 0; way < ASSOCIATIVITY; way++) {
		if (cache[index][way].LRUposition > myLRUbit) {
			cache[index][way].LRUposition = cache[index][way].LRUposition - 1;
		} else if (cache[index][way].LRUposition == myLRUbit) {
			cache[index][myway].LRUposition = ASSOCIATIVITY - 1;
		} else if (cache[index][way].LRUposition < myLRUbit) {
			cache[index][way].LRUposition = cache[index][way].LRUposition;
		}
	}
}

/*
 * Func  :  updateTag
 * Description  :Function to replace the evicted line tag(LRU) with tag in the address
 */
void updateTag(int index, int way, int tag) {
	cache[index][way].tag = tag;
	cache[index][way].validbit = 1;
}


/* Used to generate tag, index and byte select bits
 *
 */ /*
unsigned int address_generator(int tag, int TAG_BITS, int index, int INDEX_BITS,
		int BYTE_SELECT, int BYTE_OFFSET_bits) {
	unsigned int dec_value = 0, i = 0;
	int arr[ADD_BITS];
	for (int a = 0; a < ADD_BITS; a++) {
		arr[a] = 0;
	}

	while (BYTE_OFFSET_bits != 0) {
		arr[i] = BYTE_SELECT % 2;
		i++;
		BYTE_SELECT = BYTE_SELECT / 2;
		BYTE_OFFSET_bits = BYTE_OFFSET_bits - 1;
	}
	while (INDEX_BITS != 0) {
		arr[i] = index % 2;
		i++;
		index = index / 2;
		INDEX_BITS = INDEX_BITS - 1;
	}
	while (TAG_BITS != 0) {
		arr[i] = tag % 2;
		i++;
		tag = tag / 2;
		TAG_BITS = TAG_BITS - 1;
	}
	for (i = 0; i < ADD_BITS; i++) {
		dec_value = dec_value + (arr[i] * (pow(2, i)));
	}
	//printf("dec_value : %d\n", dec_value);
	return dec_value;
} */
/*
 * Func: gettag
 * To get the tag from given index and way
 */
int gettag(int index, int way) {
	return cache[index][way].tag;
}

/*  Initial Cache specifications
 *
 */
void initiatecache() {
	int index, way;
	for (index = 0; index < SETS; index++) {
		for (way = 0; way < ASSOCIATIVITY; way++) {
			cache[index][way].LRUposition = way;
			cache[index][way].tag = 0;
			cache[index][way].validbit = 0;
			cache[index][way].dirtybit=0;
		}
	}
}


/*
 Hexa Decimal to Decimal coversion and return the decimal value of the tag/index/byteselect
 */
unsigned int decodeAddress(int start, int stop, unsigned int address) {
	unsigned int dec_value = 0;
	int i = 0;
	int arr[ADD_BITS];
	if (address >= 0) {
		while (address > 0) {
			arr[i] = address % 2;
			i++;
			address = address / 2;
		}
		while (i < ADD_BITS) {
			arr[i] = 0;
			i++;
		}
	} else {
		printf("Invalidbit Address\n");
	}

	for (i = start; i <= stop; i++) {
		dec_value = dec_value + (arr[i] * (pow(2, i - start)));// decimal value of the tag
	}
	return dec_value;
}

int main() {
	FILE *fp;
	int operation;
	unsigned int address;
	unsigned int tag, index, BYTE_SELECT;
	int total_reads = 0, total_writes = 0, total_hits = 0, total_misses = 0, evictions = 0, writebacks = 0;

	// compute offsets based on given configs
	int BYTE_OFFSET = log2(LINESIZE);
	int INDEX_BITS =  log2(SETS);
	int TAG_BITS = ADD_BITS - BYTE_OFFSET - INDEX_BITS;

	// Initialize cache
	initiatecache();

	// Open the trace file in read mode
	char input_file[MAX_INPUT_FILENAME_SIZE];
	printf("Enter the input file name\n");
	scanf("%s", input_file);
	fp = fopen(input_file, "r");

#ifndef	SILENT
	printf("TAG_BITS:%d\tindex bits:%d\n", TAG_BITS, INDEX_BITS);
#endif

	// Start reading the trace file line by line

	while (fscanf(fp, "%d ", &operation) != EOF) {
			fscanf(fp, "%X ", &address);
			printf("operation - %d\taddress - %X\n", operation, address);
			int way, lruway;
			tag = decodeAddress(ADD_BITS - TAG_BITS, ADD_BITS - 1, address);
			index = decodeAddress(ADD_BITS - TAG_BITS - INDEX_BITS,
			ADD_BITS - TAG_BITS - 1, address);
			BYTE_SELECT = decodeAddress(0, ADD_BITS - TAG_BITS - INDEX_BITS - 1,
					address);
			//printf("BYTE_OFFSET: %d\n", BYTE_OFFSET);
			//printf("tag: %d, index:%d, BYTE_SELECT: %d", tag, index,	BYTE_SELECT);
			switch (operation) {
			case READ:
				total_reads++;
				way = checkTag(index, tag); // check the way in which our tag matches with a tag in a way	
				if (way < ASSOCIATIVITY ) {
					if(cache[index][way].dirtybit == 0 && cache[index][way].validbit == 0){
						cache[index][way].dirtybit == 0;
						cache[index][way].validbit == 1;
						total_misses++;	
						updateLRU(index, way); // update the recently accessed ways, here the way that is hit is the most recently accessed way
					}else if(cache[index][way].dirtybit == 0 && cache[index][way].validbit == 1){ //could be case of memory read
						cache[index][way].dirtybit == 0;
						cache[index][way].validbit == 1;
						total_hits++;
					    updateLRU(index, way); // update the recently accessed ways, here the way that is hit is the most recently accessed way
					}else if(cache[index][way].dirtybit == 1 && cache[index][way].validbit == 1){ 
						cache[index][way].dirtybit == 1;
						cache[index][way].validbit == 1;
						total_hits++;
						updateLRU(index, way); // update the recently accessed ways, here the way that is hit is the most recently accessed way
				}}else {
					lruway = findLRU(index); // find the way that is least recently accessed for evicting
					if(cache[index][lruway].dirtybit == 0 && cache[index][lruway].validbit == 0){
					total_misses++;
					updateTag(index, lruway, tag); //evict the line by updating our tag in LRU position
					cache[index][lruway].dirtybit=0;
					cache[index][lruway].validbit=1;
					updateLRU(index, lruway);// update the recently accessed ways, here lruway is the most recently accessed way 
					}else if (cache[index][lruway].dirtybit == 0 && cache[index][lruway].validbit == 1){ //...
					total_misses++;
					evictions++;
					updateTag(index, lruway, tag); //evict the line by updating our tag in LRU position
					cache[index][lruway].dirtybit=0;
					cache[index][lruway].validbit=1;
					updateLRU(index, lruway);// update the recently accessed ways, here lruway is the most recently accessed way 
					}else if (cache[index][lruway].dirtybit == 1 && cache[index][lruway].validbit == 1){
					total_misses++;
					evictions++;
					writebacks++;
					updateTag(index, lruway, tag); //evict the line by updating our tag in LRU position
					cache[index][lruway].dirtybit=0;
					cache[index][lruway].validbit=1;
					updateLRU(index, lruway);// update the recently accessed ways, here lruway is the most recently accessed way 
				}}
				break;
			case WRITE:
				total_writes++;	
				way = checkTag(index, tag); // check the way in which our tag matches with a tag in a way	
				if (way < ASSOCIATIVITY ) {
					if(cache[index][way].dirtybit == 0 && cache[index][way].validbit == 0){
						cache[index][way].dirtybit == 1;
						cache[index][way].validbit == 1;
						total_misses++;		
						updateLRU(index, way); // update the recently accessed ways, here the way that is hit is the most recently accessed way
					}else if(cache[index][way].dirtybit == 0 && cache[index][way].validbit == 1){ //could be case of memory read
						cache[index][way].dirtybit == 1;
						cache[index][way].validbit == 1;
						total_hits++;
					    updateLRU(index, way); // update the recently accessed ways, here the way that is hit is the most recently accessed way
					}else if(cache[index][way].dirtybit == 1 && cache[index][way].validbit == 1){ 
						cache[index][way].dirtybit == 1;
						cache[index][way].validbit == 1;		
						total_hits++;
						updateLRU(index, way); // update the recently accessed ways, here the way that is hit is the most recently accessed way
				}}else {
					lruway = findLRU(index); // find the way that is least recently accessed for evicting
					if(cache[index][lruway].dirtybit == 0 && cache[index][lruway].validbit == 0){
					total_misses++;
					updateTag(index, lruway, tag); //evict the line by updating our tag in LRU position
					cache[index][lruway].dirtybit=1;
					cache[index][lruway].validbit=1;
					updateLRU(index, lruway);// update the recently accessed ways, here lruway is the most recently accessed way 
					}else if (cache[index][lruway].dirtybit == 0 && cache[index][lruway].validbit == 1){
					total_misses++;
					evictions++;
					updateTag(index, lruway, tag); //evict the line by updating our tag in LRU position
					cache[index][lruway].dirtybit=1;
					cache[index][lruway].validbit=1;
					updateLRU(index, lruway);// update the recently accessed ways, here lruway is the most recently accessed way 
					}else if (cache[index][lruway].dirtybit == 1 && cache[index][lruway].validbit == 1){
					total_misses++;
					evictions++;
					writebacks++;
					updateTag(index, lruway, tag); //evict the line by updating our tag in LRU position
					cache[index][lruway].dirtybit=1;
					cache[index][lruway].validbit=1;
					updateLRU(index, lruway);// update the recently accessed ways, here lruway is the most recently accessed way 
				}}
				break;
		default:
		printf(" I am in default");
		
		}}
		
	//printing the cache parameters
	printf(" CACHE PARAMETERS \n Number of sets=%d\n Associativity= %d\n Cache line size = %d\n",SETS,ASSOCIATIVITY,LINESIZE);

	/*
	 * Calculating all the statistics
	 */
	unsigned int cache_accesses = total_reads + total_writes;
	float hitratio = (float) total_hits / cache_accesses;
	float missratio = (float) total_misses / cache_accesses;
	printf("\n===============================\n CACHE STATISTICS\n");
	printf("Number of cache accesses: %d\n",cache_accesses );
	printf("Number of cache reads: %d\n", total_reads);
	printf("Number of cache writes: %d\n", total_writes);
	printf("Number of cache hits: %d\n", total_hits);
	printf("Number of cache misses: %d\n", total_misses);
	printf("Cache hit ratio: %f %\n", hitratio*100);
	printf("Cache miss ratio: %f%\n", missratio*100);
	printf("Number of evictions: %d\n", evictions);
	printf("Number of writebacks: %d\n", writebacks);
}

