#include "cachelab.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define LINE_SIZE 100

int hit;
int eviction;
int miss;

int v = 0; // inactif
int s = -1;
int E = -1;
int b = -1;
int S;

FILE* fp = NULL;

typedef struct cache_line {
    int count;
    int valid_bit;
    int tag;
}cache_line;


int init(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "vs:E:b:t:")) != -1) {
       switch (opt) {
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
                fp = fopen(optarg, "r");
                if (fp == NULL) {
                    perror("fopen");
                    exit(1);
                }
               break; 
            case 'v':
               v = 1;
               break;
            default:
                exit(1);
        }
     }
    if ((s*E*b)<0 || !fp) {
        printf("Usage: ./csim [-v] -s <s> -E <E> -b <b> -t <tracefile>\n");
        exit(1);
    }
    return 0;
}
void init_cache(cache_line **cache) {
    hit = 0; miss = 0; eviction = 0;
    for (int i = 0; i<S; i++) {
        for (int j=0; j<E; j++) {
            cache[i][j].valid_bit = 0;
        }
    }
}
int power(int a, int b) {
    if (b < 1) return 1;
    return a * power(a , b-1);
}

int extract_tag(int add) {
    return add >> (s+b);
}

int extract_set(int add) {
    int mask = ((1<<(s+b))-1);
    return (mask & add)>>b;
}

void access_memory(int add,  cache_line **cache) {
    int set = extract_set(add);
    int tag = extract_tag(add);
    for (int i =0; i<E; i++) {
        cache_line *line = (cache[set]) + i;
        if (line->valid_bit && line->tag == tag) {
            if (v)
                printf("hit ");
            line->count++;
            hit++;
            return;
        }
    }
    miss++;
    if (v) 
        printf("miss ");
    for (int i = 0; i < E; i++) {
        cache_line *line = (cache[set])+i;
        if (!line->valid_bit) {
            line->valid_bit = 1;
            line->tag = tag;
            line->count = 0;
            return;
        }
    }
    cache_line *line_evict = cache[set];
    for (int i = 1; i<E; i++) { // Eviction
        if (line_evict->count > cache[set][i].count)
            line_evict = cache[set]+i;
    }

    line_evict->tag = tag;
    line_evict->count = 0;
    eviction++;
    if (v)
        printf("eviction ");
}
int hex_to_int(const char *hex_str) {
    int result;
    sscanf(hex_str, "%x", &result);
    return result;
}

void remove_newline(char *str) {
    char *p = strchr(str, '\n');
    if (p) *p = '\0';
}
void execute(char *lineptr,  cache_line **cache) {
    if (lineptr[0] != ' ')
            return;
    lineptr++;
    remove_newline(lineptr);
    if (v) 
        printf("%s ", lineptr);
    char c = lineptr[0];
    lineptr += 2;
    int add = hex_to_int(strtok(lineptr, ",")); 
    switch(c) {
        case 'L':
            //Fall through
        case 'S':
            access_memory(add, cache);
            if (v)
                printf("\n");
            break;
        case 'M':
            access_memory(add, cache);
            access_memory(add, cache);
            if (v)
                printf("\n");
            break;
        default:
            exit(1);

    }
    return; 
}

int main(int argc, char **argv)
{
    init(argc, argv);
    
    S = power(2, s);
    struct cache_line **cache =(struct cache_line **) malloc(sizeof(cache_line *) * S);
    if (!cache) {
        perror("malloc");
        exit(1);
    }    
    for (int i=0; i<S;i++) {
       cache[i] = (cache_line *) malloc(sizeof(cache_line)*E); 
       if (!cache[i]) {
           perror("malloc");
           exit(1);
       }
    } 
    init_cache(cache); // initialize valid bit to 0

    ssize_t rc;
    char *lineptr = NULL;
    size_t n;
    if (!fp) {
        printf("fp is null\n");
       exit(1);
    }
    while((rc = getline(&lineptr, &n, fp)) > 0) {
        if (lineptr[0] != ' ')
            continue;
        execute(lineptr, cache);
    }
    free(lineptr);
    printf("hits = %d, misses = %d, evictions = %d\n",hit, miss, eviction);
    for (int i=0; i<S;i++) 
        free(cache[i]);
    free(cache);
    return 0;
}
