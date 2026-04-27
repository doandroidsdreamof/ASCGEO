#ifndef HASHMAP_H
#define HASHMAP_H

#include <netinet/in.h>
#include <inttypes.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define INITIAL_HASH_CAPACITY (64) // const usage cause multiple definition err

typedef struct
{
    char hostname[256]; // reverse DNS result
    char domain[256];   // actual domain from DNS watcher (later)
    char country[64];
    char city[128];
    double lat;
    double lon;
    bool is_pending;
    time_t timestamp; // when this entry was cached (epoch seconds)
} CacheEntry;

typedef struct Entry
{
    char key[INET6_ADDRSTRLEN]; // IP string as key (max 45 chars + null)
    CacheEntry value;
    struct Entry *next;
} Entry;

typedef struct
{
    Entry **buckets;
    int capacity; // size of buckets array
    int count;    // number of entries (for load factor)
} HashMap;

void hashmap_print(HashMap *hashmap);
HashMap *hashmap_create(const int capacity);
uint32_t hashmap_hash(const char *key);
void hashmap_set(HashMap *hashmap, const char *key, const CacheEntry *value);
Entry *hashmap_get(HashMap *hashmap, const char *key);
bool hashmap_delete(HashMap *hashmap, const char *key);
void hashmap_destroy(HashMap *hashmap);

#endif