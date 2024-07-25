/*
 * File:   eatmemory.c
 * Author: Julio Viera <julio.viera@gmail.com>
 * Modified by Kushal Azim Ekram <kforeverisback@gmail.com>
 *
 * Created on August 27, 2012, 2:23 PM
 * Modified on July 27, 2024, 4:20 PM
 */

#define VERSION "0.2.00"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include "args/src/args.h"

// Helper macros for converting bytes to MB and GB
#define B2MB(x) ((x) / (1024 * 1024))
#define B2GB(x) (B2MB(x) / 1024)

#if defined(_SC_PHYS_PAGES) && defined(_SC_AVPHYS_PAGES) && defined(_SC_PAGE_SIZE)
#define MEMORY_PERCENTAGE
#endif

#ifdef MEMORY_PERCENTAGE
size_t getTotalSystemMemory(){
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

size_t getFreeSystemMemory(){
    long pages = sysconf(_SC_AVPHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
#endif

ArgParser* configure_cmd() {
    ArgParser* parser = ap_new_parser();
    ap_add_flag(parser, "help h ?");
    ap_add_int_opt(parser, "timeout t", -1);
    ap_add_int_opt(parser, "gradual g", -1);
    // ap_add_str_opt(parser, "alloc-type -a", "malloc");
    return parser;
}

void print_help() {
    printf("eatmemory %s - %s\n\n", VERSION, "https://github.com/julman99/eatmemory");
    printf("Usage: eatmemory [-t <seconds>] <size>\n");
    printf("Size can be specified in megabytes or gigabytes in the following way:\n");
    printf("#             # Bytes      example: 1024\n");
    printf("#M            # Megabytes  example: 15M\n");
    printf("#G            # Gigabytes  example: 2G\n");
#ifdef MEMORY_PERCENTAGE
    printf("#%%           # Percent    example: 50%%\n");
#endif
    printf("\n");
    printf("Options:\n");
    printf("-t <seconds>  Exit after specified number of seconds\n");
    printf("-g <seconds>  Gradual memory allocation till <gradual> second upto <size>.\n");
    printf("              Must be less than <timeout> if set.\n");
    printf("\n");
}

short** eat(long total,int chunk){
	long i;
    short** allocations = malloc(sizeof(short*) * (total/chunk));
    memset(allocations, 0, sizeof(short*) * (total/chunk));
	for(i=0;i<total/chunk;i++){
		short *buffer=malloc(sizeof(char)*chunk);
        if(buffer==NULL){
            return NULL;
        }
		memset(buffer,0,chunk);
        allocations[i] = buffer;
	}
    return allocations;
}

void digest(short** eaten, long total,int chunk) {
    
    for (long i = 0; i < total / chunk; i++) {
        free(eaten[i]);
    }
    // Free the actual eaten variable
    free(eaten);
}

int main(int argc, char *argv[]){

#ifdef MEMORY_PERCENTAGE
    printf("Currently total memory: %zd GB\n",B2GB(getTotalSystemMemory()));
    printf("Currently avail memory: %zd GB\n",B2GB(getFreeSystemMemory()));
#endif

    ArgParser* parser = configure_cmd();
    ap_parse(parser, argc, argv);
    if(ap_found(parser, "help")) {
        print_help();
        exit(0);
    }
    int pos_arg_count = ap_count_args(parser);
    if(pos_arg_count != 1) {
        print_help();
        exit(1);
    }

    int timeout = ap_get_int_value(parser, "timeout");
    int gradual_timeout = ap_get_int_value(parser, "gradual");
    char** pos_args = ap_get_args(parser);
    char* memory_to_eat = pos_args[0];
    ap_free(parser);

    int len=strlen(memory_to_eat);
    char unit=memory_to_eat[len - 1];
    long size=-1;
    int chunk=1024;
    if(!isdigit(unit) ){
        if(unit=='M' || unit=='G'){
            memory_to_eat[len-1]=0;
            size=atol(memory_to_eat) * (unit=='M'?1024*1024:1024*1024*1024);
        }
#ifdef MEMORY_PERCENTAGE
        else if (unit=='%') {
            size = (atol(memory_to_eat) * (long)getFreeSystemMemory())/100;
        }
#endif
        else{
            fprintf(stderr, "Invalid size format\n");
            exit(0);
        }
    }else{
        size=atoi(memory_to_eat);
    }
    if(size <=0 ) {
        fprintf(stderr, "ERROR: Size must be a positive integer");
        exit(1);
    }

    free(pos_args);

    // Timeout should should be equal to or greater than the gradual timeout
    // Since gradual_timeout will alloc mem each sec, then wait for timeout sec
    if(timeout < gradual_timeout) {
      printf("<gradual_timeout> must be less-than or equal-to <timeout>");
      exit(1);
    }

    // Initial non-gradual allocation values
    int alloc_count = 1;
    long alloc_size = size;
    printf("Eating %ld MB ", B2MB(size));
    if (gradual_timeout > 0){
        // Allocate memory each second, so count = gradual_timeout
        alloc_count = gradual_timeout;
        // each second, allocate size/gradual_timeout
        alloc_size = size/gradual_timeout;
        printf("(gradually %ld MB at a time) in %d seconds ", B2MB(alloc_size), gradual_timeout);
    }
    printf("in chunks of %d bytes\n", chunk);
    // Allocate memory in chunks that is gradually allocated
    short*** all_eaten_chunks = malloc(sizeof(short**) * alloc_count);
    // Make sure they are zeroed out, it'll help check the free process
    memset(all_eaten_chunks, 0, sizeof(short**) * alloc_count);
    for(long i=0;i<alloc_count;i+=1){
        short** eaten = eat(alloc_size, chunk);
        all_eaten_chunks[i] = eaten;
        if(!eaten){
            fprintf(stderr, "ERROR: Could'nt allocate %ld MB of memory\n", B2MB(alloc_size));
            timeout = 0;
            break;
        }else {
            printf("%ld: Allocated %ld MB of memory\n",i, B2MB(alloc_size));
        }
        sleep(1);
        timeout--;
	}
    if(all_eaten_chunks){
#ifdef MEMORY_PERCENTAGE
        printf("Currently avail memory (after alloc): %zd GB\n", B2GB(getFreeSystemMemory()));
#endif
        if(timeout < 0 && isatty(fileno(stdin))) {
            printf("Done, press ENTER to free the memory\n");
            getchar();
        } else if (timeout >= 0) {
            printf("Done, sleeping for %d seconds before exiting...\n", timeout);
            sleep(timeout);
        } else {
            printf("Done, kill this process to free the memory\n");
            while(true) {
                sleep(1);
            }
        }
        for(long i=0;i<alloc_count;i+=1){
            digest(all_eaten_chunks[i], alloc_size, chunk);
            all_eaten_chunks[i] = NULL;
        }
        free(all_eaten_chunks);

    }else{
        printf("ERROR: Could not allocate the memory");
    }

}

