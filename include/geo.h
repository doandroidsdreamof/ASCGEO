#ifndef GEO_H
#define GEO_H

#include "hashmap.h"

void geo_batch_lookup(char ips[][INET6_ADDRSTRLEN], int count, HashMap *cache);

#endif