/*
 *	bjkeeleydebonis
 *
*/

/*
	Standard Headers
*/
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
//#include <strings.h>

/*
	Custom Headers
*/
#include "cachelab.h"

/*
	Memory address.
*/
typedef unsigned long long int memory_address_t;

/*
	The statistics for a cache.
*/
typedef struct {
	int hits; 	   // number of cache hits
	int misses;	   // number of cache misses
	int evictions; // number of cache evictions using LRU Policy
} CacheStatistics;

/*
	The user specified settings for the cache simulation.
*/
typedef struct {
	int numOfSetIndexBits;   // Number of set index bits (the number of sets is 2 s)
	int numOfBlockBits;   // Number of block bits (the block size is 2 b)
	int linesPerSet; // Associativity (number of lines per set)
	bool showHelp;   // Optional help flag that prints usage info
	bool beVerbose;  // Optional verbose flag that displays trace info
} CacheSettings;

/*
	Container in a cache that holds valid bit, tag bits, and block.
*/
typedef struct {
	bool isValid;
	memory_address_t tag;
	int useCounter;
	// char *block; // dont need this actually
} Line;

/*
	Collection of one or more lines.
*/
typedef struct {
	Line *lines;
} Set;

/*
	A Cache is an array of sets.
*/
typedef struct {
 	Set *sets;
 	CacheStatistics cacheStatistics;
} Cache;

/*
	Creates an empty cache based on the specified parameters
*/
Cache createEmptyCache (int numOfSets, int numOfLines, int blockSize) 
{
	Cache cache;	
	cache.sets = (Set*) malloc(sizeof(Set) * numOfSets);

	int setIndex;
	for (setIndex = 0; setIndex < numOfSets; setIndex++) 
	{
		Set set;
		set.lines =  (Line*) malloc(sizeof(Line) * numOfLines);
		cache.sets[setIndex] = set;

		int lineIndex;
		for (lineIndex = 0; lineIndex < numOfLines; lineIndex++) 
		{
			Line line;
			line.isValid = false;
			line.tag = 0;
			line.useCounter = 0;
			set.lines[lineIndex] = line;	
		}
	}

	cache.cacheStatistics.hits = 0;
	cache.cacheStatistics.misses = 0;
	cache.cacheStatistics.evictions = 0;
	return cache;
}

/*
	Find the index of the LRU line in the query set.
*/
int findLRULineIndex(Set querySet, CacheSettings cs) {
	int numberOfLines = cs.linesPerSet;
	int minIndex = 0;

	int lineIndex;
	for (lineIndex = 1; lineIndex < numberOfLines; lineIndex ++) {
		if (querySet.lines[lineIndex].useCounter >
		 querySet.lines[minIndex].useCounter){
		 	minIndex = lineIndex;
		}
	}
	return minIndex;
}


/*
	Simulate the cache.
*/
Cache simulateCache(Cache cache, CacheSettings cs, memory_address_t address){
	// from the address, extract the tag, index, and offset
	// t = m -( s + b ) where t is tag size
	int tagSize = (64 - (cs.numOfSetIndexBits + cs.numOfBlockBits));

	memory_address_t inputTag = address >> (cs.numOfSetIndexBits + cs.numOfBlockBits);

	unsigned long long setIndex = (address << tagSize) >> (tagSize + cs.numOfSetIndexBits);

	Set querySet = cache.sets[setIndex]; // get the set this address maps to

	int lineIndex;
	bool isCacheFull = true; // assume the cache is full
	int emptyLineIndex = -1;
 
	for (lineIndex = 0; lineIndex < cs.linesPerSet; lineIndex++){
		Line *line = &querySet.lines[lineIndex];

		if (line->isValid){
			if (line->tag == inputTag){ // cache hit
				cache.cacheStatistics.hits++;
				line->useCounter++;
				return cache;
			}
		}
		else {
			isCacheFull = false;
			emptyLineIndex = lineIndex; // this line was empty, store for later use
		}
	}

	/*
	 	Only reach this part of function if there was no cache hit.
	 */

	cache.cacheStatistics.misses++;

	if (isCacheFull){ // have to evict LRU line in the set
		cache.cacheStatistics.evictions++;
		int evictIndex = findLRULineIndex(querySet, cs);
		querySet.lines[evictIndex].tag = inputTag;
		querySet.lines[evictIndex].useCounter = 0;
	}
	else { // use the last found empty line index
		
		querySet.lines[emptyLineIndex].tag = inputTag;
		querySet.lines[emptyLineIndex].isValid = true;
		querySet.lines[emptyLineIndex].useCounter = 0;
	}

	return cache;
}

/*
	The start or entry to the program.
*/
int main(int argc, char **argv)
{
	CacheSettings *cacheSettings = malloc(sizeof(CacheSettings));
	char *traceFileName;// = NULL;
    char option;
	/*
	-h : Optional help flag that prints usage info
	-v : Optional verbose flag that displays trace info
	-s <s> : Number of set index bits (the number of sets is 2 s )
	-E <E> : Associativity (number of lines per set)
	-b <b> : Number of block bits (the block size is 2 b )
	-t <tracefile> : Name of the valgrind trace to replay
	*/
 	while((option = getopt(argc, argv, "s:b:t:E:vh")) != -1) {
         if(option == 's') cacheSettings->numOfSetIndexBits = atoi(optarg);
    else if(option == 'E') cacheSettings->linesPerSet = atoi(optarg);
    else if(option == 'b') cacheSettings->numOfBlockBits = atoi(optarg);
    else if(option == 't') traceFileName = optarg;
    else if(option == 'v') cacheSettings->beVerbose = true;
	}

	int numberOfSets = pow(2.0, cacheSettings->numOfSetIndexBits);
	int blockSize = pow(2.0, cacheSettings->numOfBlockBits); 

	Cache cache = createEmptyCache(numberOfSets,
		cacheSettings->linesPerSet, blockSize);

	FILE *traceFile;
	traceFile  = fopen(traceFileName, "r");

	if(traceFile == NULL) {
	    printf("Failed to read file\n");
	   	exit(1);
	}

	char traceCommand; // one of I M L S
 	memory_address_t address;
  	int size;

	while (fscanf(traceFile, "%c %llx,%d", &traceCommand, &address, &size) > 0) {
		switch(traceCommand) {								// don't really need &size
			case 'I':
				break;
			case 'L':
				cache = simulateCache(cache, *cacheSettings, address);
				break;
			case 'S':
				cache = simulateCache(cache, *cacheSettings, address);
				break;
			case 'M': 
				// data Modify and then Store, simulate twice
				cache = simulateCache(cache, *cacheSettings, address);
				cache = simulateCache(cache, *cacheSettings, address);
				break;
			default:
				break;
		}
	}

	printSummary(cache.cacheStatistics.hits, cache.cacheStatistics.misses,
	 cache.cacheStatistics.evictions);

	fclose(traceFile);
	exit(0);
}
