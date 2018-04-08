#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

/*
** Get string length, including null terminator.
*/
#define STRLEN_P1(pzStr) (strlen(pzStr) + 1)

/*******************************************************************************
** Structs
*******************************************************************************/
typedef struct ht_entry
{
    void*           pKey;
    ht_size_t       key_size;
    void*           pData;
    ht_size_t       data_size;
    ht_free_func_t  pfnFree;
    ht_hash_t       hash;
    struct ht_entry *pNext;
} ht_entry;


struct hash_table
{
    struct ht_entry **ppEntries;
    ht_size_t       size;
    ht_size_t       capacity;
    ht_hash_func_t  pfnHash;
};


/*******************************************************************************
** Func defs
*******************************************************************************/
static ht_hash_t default_hash_impl(const void *key, ht_size_t key_size);

static int grow_table(hash_table *pHt);

static void place_item(ht_entry **ppEntries, ht_size_t capacity, ht_entry *pEntry);

static void free_item(ht_entry *pEntry);

static int hash_exists(hash_table *pHt, ht_hash_t hash, ht_entry **ppFound_entry);


/*******************************************************************************
** Methods
*******************************************************************************/

/*
** Default hash implementation to hash the bytes of the given key.
** Hash name: "SDBM"
*/
static ht_hash_t default_hash_impl(const void *key, ht_size_t key_size)
{
    const uint8_t *raw;
    ht_hash_t hash;
    ht_size_t i;

    assert(key != NULL);
    assert(key_size != HT_NO_SIZE);

    hash = 0;
    raw = (uint8_t*)key;

    for(i = 0; i < key_size; ++i)
    {
        hash = raw[i] + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

/*
** Free a hash table entry. Does nothing if pEntry null.
*/
static void free_item(ht_entry *pEntry)
{
    if(pEntry == NULL)
    {
        return;
    }

    if(pEntry->pData != NULL)
    {
        /*
        ** Free data memory based on free function.
        */
        if(pEntry->pfnFree == HT_TRANSIENT)
        {
            free(pEntry->pData);
        }
        else if(pEntry->pfnFree != HT_STATIC)
        {
            /*
            ** Note: This assert should never fail because HT_TRANSIENT should always
            ** be the same value as NULL.
            */
            assert(pEntry->pfnFree != NULL);
            pEntry->pfnFree(pEntry->pData);
        }
    }

    if(pEntry->pKey != NULL)
    {
        free(pEntry->pKey);
    }

    free(pEntry);
}

/*
** Grow the given hash table to a new capacity. Replaces the entry array with a larger array and
** replaces items into a new spot.
*/
static int grow_table(hash_table *pHt)
{
    ht_entry **new_array;
    ht_entry *entry;
    ht_entry *tmp_next;
    ht_size_t new_cap;
    ht_size_t i;

    assert(pHt != NULL);

    new_cap = pHt->capacity * HT_GROW_FACT;
    new_array = calloc(new_cap, sizeof(ht_entry*));
    if(new_array == NULL)
    {
        return HT_ALLOC_ERR;
    }

    for(i = 0; i < pHt->capacity; ++i)
    {
        entry = pHt->ppEntries[i];

        while(entry != NULL)
        {
            /*
            ** Save a temporary pointer to the next value and set the entry's next to NULL
            ** to make it the end of a linked list. Then place the item into the new entry array.
            */
            tmp_next = entry->pNext;
            entry->pNext = NULL;

            place_item(new_array, new_cap, entry);

            /*
            ** Go to next item. If null, loop stops.
            */
            entry = tmp_next;;
        }
    }

    free(pHt->ppEntries);

    pHt->capacity = new_cap;
    pHt->ppEntries = new_array;

    return HT_OK;
}

/*
** Returns 1 if given hash exists in table, otherwise 0.
*/
static int hash_exists(hash_table *pHt, ht_hash_t hash, ht_entry **ppFound_entry)
{
    int rc;
    ht_size_t hash_pos;
    ht_entry *entry;

    assert(pHt != NULL);

    hash_pos = hash % pHt->capacity;
    entry = pHt->ppEntries[hash_pos];

    rc = 0;

    while(entry != NULL && rc == 0)
    {
        if(entry->hash == hash)
        {
            /*
            ** Output entry optional, set if not null.
            */
            if(ppFound_entry != NULL)
            {
                *ppFound_entry = entry;
            }

            rc = 1;
        }
        entry = entry->pNext;
    }

    return rc;
}

/*
** Free memory. If pointer NULL, then no-op.
*/
HTPUBAPI void ht_free(hash_table *pHt)
{
    ht_size_t i;

    if(pHt != NULL)
    {
        if(pHt->ppEntries != NULL)
        {
            /*
            ** Loop through element array, freeing memory of elements. Some
            ** elements may not be used.
            */
            for(i = 0; i < pHt->capacity; ++i)
            {
                if(pHt->ppEntries[i] != NULL)
                {
                    free_item(pHt->ppEntries[i]);
                }
            }

            free(pHt->ppEntries);
        }

        free(pHt);
    }
}

/*
** Try to find an item and set output parameters.
*/
HTPUBAPI int ht_get_item(hash_table *pHt, const void *pKey, ht_size_t key_size, void **ppData, ht_size_t *pData_size)
{
    ht_hash_t hash;
    ht_entry *pEntry;
    int rc;

    /*
    ** Guard: Make sure pointer are not null.
    */
    if(pHt == NULL || pKey == NULL || ppData == NULL || pData_size == NULL)
    {
        return HT_ARG_NULL;
    }

    rc = HT_NOT_FOUND;
    hash = pHt->pfnHash(pKey, key_size);

    if(hash_exists(pHt, hash, &pEntry))
    {
        *ppData = pEntry->pData;
        *pData_size = pEntry->data_size;
        rc = HT_OK;
    }
    else
    {
        *ppData = NULL;
        *pData_size = HT_NO_SIZE;
    }

    return rc;
}

/*
** Simple getter for hash_table size.
*/
HTPUBAPI ht_size_t ht_get_size(hash_table *pHt)
{
    if(pHt == NULL)
    {
        return HT_NO_SIZE;
    }
    else
    {
        return pHt->size;
    }
}

/*
** Attempts to find the given key. Returns 1 if the key exists, 0 if not. 
*/
HTPUBAPI int ht_has_key(hash_table *pHt, const void *pKey, ht_size_t key_size)
{
    ht_hash_t hash;
    int rc;

    /*
    ** Return 0/false if any pointer is null.
    */
    if(pHt == NULL || pKey == NULL)
    {
        return 0;
    }

    hash = pHt->pfnHash(pKey, key_size);
    rc = hash_exists(pHt, hash, NULL);

    return rc;
}

/*
** Allocate dynamic memory and set default values.
*/
HTPUBAPI int ht_init(hash_table **ppHt)
{
    if(ppHt == NULL)
    {
        return HT_ARG_NULL;
    }

    *ppHt = malloc(sizeof(hash_table));
    if(ppHt == NULL)
    {
        goto error;
    }

    (*ppHt)->ppEntries = calloc(HT_DEFAULT_CAP, sizeof(ht_entry*));
    if((*ppHt)->ppEntries == NULL)
    {
        goto error;
    }

    (*ppHt)->capacity = HT_DEFAULT_CAP;
    (*ppHt)->size = 0;
    (*ppHt)->pfnHash = &default_hash_impl;

    return HT_OK;

error:
    /*
    ** Free potentially allocated memory on an error.
    */
    if(ppHt != NULL)
    {
        if((*ppHt)->ppEntries != NULL)
        {
            free((*ppHt)->ppEntries);
        }
        free(*ppHt);
    }
    return HT_ALLOC_ERR;
}

/*
** Insert an item into the hash table. If an item with the same key alreay exists, then HT_DUPLICATE is returned.
*/
HTPUBAPI int ht_insert(hash_table *pHt, const void *pKey, ht_size_t key_size, void* pData, ht_size_t data_size, ht_free_func_t pfnFree)
{
    ht_hash_t hash;
    ht_entry *new_entry;
    int rc;

    /*
    ** Guard: Make sure pointers are not null.
    */
    if(pHt == NULL || pKey == NULL || pData == NULL)
    {
        return HT_ARG_NULL;
    }

    /*
    ** Make sure hash funtion set, then compute hash.
    */
    if(pHt->pfnHash == NULL)
    {
        return HT_ARG_NULL;
    }

    hash = pHt->pfnHash(pKey, key_size);

    /*
    ** Make sure an item with the given key doesn't already exist.
    */
    if(hash_exists(pHt, hash, NULL))
    {
        return HT_DUPLICATE;
    }

    /*
    ** Determine if the table needs to grow. Return an error code if 
    ** growth fails.
    */
    if((pHt->size + 1) > pHt->capacity * HT_MAX_CAP)
    {
        rc = grow_table(pHt);
        if(rc != HT_OK)
        {
            return rc;
        }
    }

    new_entry = malloc(sizeof(ht_entry));
    if(new_entry == NULL)
    {
        goto error;
    }

    new_entry->key_size = key_size;
    new_entry->data_size = data_size;
    new_entry->hash = hash;
    new_entry->pNext = NULL;

    /*
    ** Allocate memory and copy key to new entry.
    */
    new_entry->pKey = malloc(key_size);
    if(new_entry->pKey == NULL)
    {
        goto error;
    }
    memcpy(new_entry->pKey, pKey, key_size);

    /*
    ** Allocation strategy - If TRANSIENT, then make a copy of the data.
    ** Otherwise, the given free function (or HT_STATIC) is used with no copy.
    */
    new_entry->pfnFree = pfnFree;
    if(pfnFree == HT_TRANSIENT)
    {
        new_entry->pData = malloc(data_size);
        if(new_entry == NULL)
        {
            goto error;
        }
        memcpy(new_entry->pData, pData, data_size);
    }
    else
    {
        new_entry->pData = pData;
    }

    /*
    ** Place the item into the hash table.
    */
    place_item(pHt->ppEntries, pHt->capacity, new_entry);
    pHt->size += 1;

    return HT_OK;

error:
    /*
    ** Free newly allocated memory on error.
    */
    if(new_entry != NULL)
    {
        if(new_entry->pKey != NULL)
        {
            free(new_entry->pKey);
        }

        /* 
        ** Only free data if pFree is set to TRANSIENT (copy data).
        */
        if(new_entry->pData != NULL && pfnFree == HT_TRANSIENT)
        {
            free(new_entry->pData);
        }

        free(new_entry);
    }
    return HT_ALLOC_ERR;
}

/*
** Remove an item from the hash table. If the item is not found, return HT_NOT_FOUND.
*/
HTPUBAPI int ht_remove(hash_table *pHt, const void *pKey, ht_size_t key_size)
{
    ht_hash_t hash;
    ht_size_t hash_pos;
    ht_entry *pEntry;
    ht_entry *pTmp;
    int done;
    int rc;

    if(pHt == NULL || pKey == NULL)
    {
        return HT_ARG_NULL;
    }

    hash = pHt->pfnHash(pKey, key_size);
    hash_pos = hash % pHt->capacity;

    pEntry = pHt->ppEntries[hash_pos];

    /*
    ** If pEntry is NULL, then somehow the Hash Table was corrupted.
    */
    if(pEntry == NULL)
    {
        rc = HT_NOT_FOUND;
    }
    /*
    ** Simple case: The first item's hash matches, set the entry array pointer to next.
    ** Everything is still good if next is NULL.
    */
    if(pEntry->hash == hash)
    {
        pHt->ppEntries[hash_pos] = pEntry->pNext;
        free_item(pEntry);
        pHt->size -= 1;

        rc = HT_OK;
    }
    else
    {
        /*
        ** Walk the linked list, trying to find the hash. Walk using pNext because pEntry has 
        ** already been tested.
        */
        done = 0;
        while(pEntry->pNext != NULL && !done)
        {
            /*
            ** If the next entry's hash matches, then set the current entry's next to Next's next.
            ** so e -> n -> (n + 1) => e => (n + 1).
            ** If (n + 1) is NULL, then no more entries exist in the linked list.
            */
            if(pEntry->pNext->hash == hash)
            {
                /*
                ** Need to store a temporary pointer to pEntry->Next for deletion becuase
                ** pEntry->Next needs to be assigned to the next entry before the "old"
                ** pEntry->Next is deleted. So the reuslt will look like:
                **     pEntry -> pNext -> pNextNext => pEntry -> pNextNext
                **     pTmp = pNext.
                */
                pTmp = pEntry->pNext;
                pEntry->pNext = pEntry->pNext->pNext;

                /*
                ** Now free the item that was removed from the linked list.
                */
                free_item(pTmp);
                pHt->size -= 1;

                rc = HT_OK;
                done = 1;
            }

            pEntry = pEntry->pNext;
        }

        if(!done)
        {
            rc = HT_NOT_FOUND;
        }
    }

    return rc;;
}

/*
** Update the hashing function on pHt to the given function pointer. If data exists
** in the table, then return HT_MISUSE becuase existing items would potentially not 
** be accessable from the new hash function.
*/
HTPUBAPI int ht_set_hash_func(hash_table *pHt, ht_hash_func_t pFn)
{
    if(pHt == NULL || pFn == NULL)
    {
        return HT_ARG_NULL;
    }
    
    if(pHt->size > 0)
    {
        return HT_MISUSE;
    }

    pHt->pfnHash = pFn;

    return HT_OK;
}

/*
** The following functions are convenience functions that forward a null terminator string to 
** a "normal" hash function.
*/
HTPUBAPI int ht_strk_get_item(hash_table *pHt, const char *pzKey, void **ppData, ht_size_t *pData_size)
{
    if(pzKey == NULL)
    {
        return HT_ARG_NULL;
    }

    return ht_get_item(pHt, pzKey, STRLEN_P1(pzKey), ppData, pData_size);
}

HTPUBAPI int ht_strk_has_key(hash_table *pHt, const char *pzKey)
{
    if(pzKey == NULL)
    {
        return HT_ARG_NULL;
    }

    return ht_has_key(pHt, pzKey, STRLEN_P1(pzKey));
}

HTPUBAPI int ht_strk_insert(hash_table *pHt, const char *pzKey, void *pData, ht_size_t data_size, ht_free_func_t pfnFree)
{
    if(pzKey == NULL)
    {
        return HT_ARG_NULL;
    }

    return ht_insert(pHt, pzKey, STRLEN_P1(pzKey), pData, data_size, pfnFree);
}

HTPUBAPI int ht_strk_remove(hash_table *pHt, const char *pzKey)
{
    if(pzKey == NULL)
    {
        return HT_ARG_NULL;
    }

    return ht_remove(pHt, pzKey, STRLEN_P1(pzKey));
}

/*
** Place an item into the hash table. The linked list items in ht_entry
** structures will be traversed if a hash collision is found.
*/
static void place_item(ht_entry **ppEntries, ht_size_t capacity, ht_entry *pEntry)
{
    ht_size_t hash_pos;
    ht_entry *exist_entry;

    assert(ppEntries != NULL);
    assert(pEntry != NULL);
    assert(capacity != HT_NO_SIZE);

    hash_pos = pEntry->hash % capacity;

    /*
    ** If entry at the hash position is NULL, then do a simple pointer assignment.
    */
    if(ppEntries[hash_pos] == NULL)
    {
        ppEntries[hash_pos] = pEntry;
    }
    /*
    ** Otherwise, there was a hash collision and the linked list needs to be traversed
    ** so the new entry can be added at the end.
    */
    else
    {
        exist_entry = ppEntries[hash_pos];
        while(exist_entry->pNext != NULL)
        {
            exist_entry = exist_entry->pNext;
        }

        exist_entry->pNext = pEntry;
    }
}