#include "geo.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include "hashmap.h"

void geo_batch_lookup(HashMap *cache)
{

    cJSON *array = cJSON_CreateArray();
     int count = 0;
     
    for (int i = 0; i < cache->capacity; i++)
    {
        Entry *entry = cache->buckets[i];
        while (entry != NULL)
        {
            if (entry->value.is_pending)
            {

                cJSON *obj = cJSON_CreateObject();
                cJSON_AddStringToObject(obj, "query", entry->key);
                cJSON_AddStringToObject(obj, "fields", "status,country,city,lat,lon,query");
                cJSON_AddItemToArray(array, obj);
                count++;
            }

            entry = entry->next;
        }
    }
  if (count == 0)
    {
        cJSON_Delete(array);
        return;
    }
    char *json_str = cJSON_PrintUnformatted(array);
    cJSON_Delete(array);
    if (json_str == NULL)
    {
        fprintf(stderr, "[GEO] cJSON_PrintUnformatted failed\n");
        return;
    }

    // Write to temp file
    FILE *tmp = fopen("/tmp/ascgeo_batch.json", "w");
    if (tmp == NULL)
    {
        fprintf(stderr, "[GEO] failed to open temp file\n");
        free(json_str);
        return;
    }
    fputs(json_str, tmp);
    fclose(tmp);
    free(json_str);

    def_prog_mode(); // Save current ncurses terminal settings

    FILE *fp = popen(
        "curl -s -X POST 'http://ip-api.com/batch' "
        "-H 'Content-Type: application/json' "
        "-d @/tmp/ascgeo_batch.json 2>/dev/null",
        "r");

    if (fp == NULL)
    {
        reset_prog_mode(); // Restore if popen fails
        refresh();
        fprintf(stderr, "[GEO] popen failed\n");
        return;
    }

    char response[16384];
    size_t total = 0;
    size_t bytes;
    while ((bytes = fread(response + total, 1, sizeof(response) - total - 1, fp)) > 0)
    {
        total += bytes;
    }
    response[total] = '\0';
    pclose(fp);

    reset_prog_mode(); // Return terminal to ncurses state
    refresh();         // Force a complete redraw of the UI

    // fprintf(stderr, "[GEO] response (%zu bytes): %.500s\n", total, response);

    cJSON *results = cJSON_Parse(response);
    if (results == NULL || !cJSON_IsArray(results))
    {
        fprintf(stderr, "[GEO] JSON parse failed or not an array\n");
        if (results)
            cJSON_Delete(results);
        return;
    }

    int size = cJSON_GetArraySize(results);
    for (int i = 0; i < size; i++)
    {
        cJSON *item = cJSON_GetArrayItem(results, i);
        cJSON *status = cJSON_GetObjectItem(item, "status");
        cJSON *query = cJSON_GetObjectItem(item, "query");

        if (!cJSON_IsString(status) || strcmp(status->valuestring, "success") != 0)
            continue;

        if (!cJSON_IsString(query))
            continue;

        CacheEntry entry;
        memset(&entry, 0, sizeof(entry));

        cJSON *country = cJSON_GetObjectItem(item, "country");
        cJSON *city = cJSON_GetObjectItem(item, "city");
        cJSON *lat = cJSON_GetObjectItem(item, "lat");
        cJSON *lon = cJSON_GetObjectItem(item, "lon");

        if (cJSON_IsString(country))
            strncpy(entry.country, country->valuestring, sizeof(entry.country) - 1);
        if (cJSON_IsString(city))
            strncpy(entry.city, city->valuestring, sizeof(entry.city) - 1);
        if (cJSON_IsNumber(lat))
            entry.lat = lat->valuedouble;
        if (cJSON_IsNumber(lon))
            entry.lon = lon->valuedouble;
        entry.timestamp = time(NULL);
        entry.is_pending = false; 
        hashmap_set(cache, query->valuestring, &entry);
    }

    cJSON_Delete(results);
}