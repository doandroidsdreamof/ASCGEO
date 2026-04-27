#include <stdio.h>
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

// TODO write tests for collison and resize
void hashmap_print(HashMap *hashmap)
{
    fprintf(stderr, "\n=== HashMap [%d entries, %d buckets, load=%.2f] ===\n",
            hashmap->count, hashmap->capacity,
            (double)hashmap->count / hashmap->capacity);

    for (int i = 0; i < hashmap->capacity; i++)
    {
        Entry *entry = hashmap->buckets[i];
        if (entry == NULL)
        {
            continue;
        }

        fprintf(stderr, "  [%02d] ", i);

        while (entry != NULL)
        {
            fprintf(stderr, "%s {%s, %s, %.4f, %.4f}",
                    entry->key,
                    entry->value.city,
                    entry->value.country,
                    entry->value.lat,
                    entry->value.lon);

            if (entry->next != NULL)
            {
                fprintf(stderr, " -> ");
            }
            entry = entry->next;
        }
        fprintf(stderr, "\n");
    }

    fprintf(stderr, "=== end ===\n\n");
}

HashMap *hashmap_create(const int capacity)
{
    HashMap *hashmap = (HashMap *)malloc(sizeof(HashMap));
    if (hashmap == NULL)
    {
        fprintf(stderr, "hashmap_create Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    hashmap->capacity = capacity;
    hashmap->count = 0;
    hashmap->buckets = calloc(hashmap->capacity, sizeof(Entry *)); //  this one automatically fills buckets as null. learn why pointer size of the Entry used *
    return hashmap;
}

/*
start with offset basis (a magic constant)
for each byte in the key:
    hash = hash XOR byte
    hash = hash * FNV prime (another magic constant)
return hash

  1 ^ 1 = 0  (same → 0)
  0 ^ 1 = 1  (different → 1)
  1 ^ 0 = 1  (different → 1)
  0 ^ 0 = 0  (same → 0)
*/

// uint32_t => It gives you 4.2 billion possible hash value
// learn this algorithm magic numbers and bitwise relationship
uint32_t hashmap_hash(const char *key)
{
    uint32_t hash = 0x811C9DC5; //  32-bit FNV offset basis but I don't have fucking idea
    uint32_t fnv_prime = 0x01000193;
    while (*key != '\0')
    {
        hash = hash ^ *key;
        hash = hash * fnv_prime;
        key++;
    }
    return hash;
}

// Load factor = number of entries / number of buckets
// collison => different keys, same hash
void hashmap_set(HashMap *hashmap, const char *key, const CacheEntry *value)
{
    if (hashmap == NULL)
    {
        printf("hashmap_set hashmap null");
        return;
    }

    double load_factor = (double)hashmap->count / hashmap->capacity; // why (double) cast required?
    double threshold = 0.75;
    /*     fprintf(stderr, "DEBUG: load_factor=%.2f >= threshold=%.2f ? %s\n",
                load_factor, threshold,
                (load_factor >= threshold) ? "YES - RESIZE" : "NO"); */
    // resize bucket
    if (load_factor >= threshold)
    {
        // fprintf(stderr, "\n\n ========= resize beginning =========\n");
        // reindexing because -> 200 % 64 = index_8 200 % 128 = index_72
        Entry **old_buckets = hashmap->buckets;
        int old_capacity = hashmap->capacity;
        hashmap->buckets = calloc(old_capacity * 2, sizeof(Entry *));
        hashmap->capacity = old_capacity * 2;
        for (int i = 0; i < old_capacity; i++)
        {

            Entry *entry = old_buckets[i];
            if (entry == NULL)
            {
                continue;
            }

            while (entry != NULL)
            {
                Entry *next = entry->next; // without this chain is lost because new item added to list
                uint32_t newIndex = hashmap_hash(entry->key) % (old_capacity * 2);
                entry->next = hashmap->buckets[newIndex]; // bind node.next to new chain (NULL or prev_head)
                hashmap->buckets[newIndex] = entry;       // make this node to new head
                entry = next;                             // in case of current->next => it can point previous node due to insert
            }
        }

        free(old_buckets);
    }
    uint32_t index = hashmap_hash(key) % hashmap->capacity; // normalize hash with module and distribute as index
    // printf("hash ========> : %" PRIu32 "\n", index);
    Entry *current = hashmap->buckets[index];
    while (current != NULL)
    {
        // carefully review here
        if (strcmp(current->key, key) == 0)
        {
            current->value = *value;
            return;
        }
        current = current->next;
    }
    Entry *node = malloc(sizeof(Entry));
    if (node == NULL)
    {
        fprintf(stderr, "hashmap_set Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    strncpy(node->key, key, sizeof(node->key) - 1);
    node->key[sizeof(node->key) - 1] = '\0';
    node->value = *value;
    node->next = hashmap->buckets[index]; // node->next[chain in the bucket]
    hashmap->buckets[index] = node;       // assign it as new head
    hashmap->count++;
    return;
}

Entry *hashmap_get(HashMap *hashmap, const char *key)
{
    if (hashmap == NULL)
    {
        printf("hashmap_get hashmap null");
        return NULL;
    }
    uint32_t index = hashmap_hash(key) % hashmap->capacity;
    Entry *current = hashmap->buckets[index];
    while (current != NULL)
    {
        if (strcmp(current->key, key) == 0)
        {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

bool hashmap_delete(HashMap *hashmap, const char *key)
{
    if (hashmap == NULL)
    {
        printf("hashmap_delete hashmap null");
        return false;
    }
    uint32_t index = hashmap_hash(key) % hashmap->capacity;
    Entry *current = hashmap->buckets[index];
    Entry *prev = NULL;
    // A -> B -> C
    while (current != NULL)
    {

        if (strcmp(current->key, key) == 0)
        {
            Entry *temp = current;
            if (prev == NULL) // removing head, promote next node/NULL to head
            {
                hashmap->buckets[index] = current->next;
            }
            else
            {
                prev->next = current->next; // A -> C
            }
            free(temp);
            hashmap->count--;
            return true;
        }
        prev = current;
        current = current->next;
    }

    return false;
}

void hashmap_destroy(HashMap *hashmap)
{
    int bucket_length = hashmap->capacity;
    for (int i = 0; i < bucket_length; i++)
    {
        Entry *entry = hashmap->buckets[i];
        if (entry == NULL)
        {
            continue;
        }
        while (entry != NULL)
        {
            Entry *next = entry->next;
            Entry *temp = entry; // can it be unnecessarily
            free(temp);
            entry = next;
        }
    }
    /*
   aside from linked_list, buckets can be removed without iteration because
   each malloc is a separate allocation, so each needs its own free.
    */
    free(hashmap->buckets);
    free(hashmap);
}

