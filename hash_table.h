#ifndef _HASH_TABLE_H
#define _HASH_TABLE_H

#include <inttypes.h>

#define HTPUBAPI

/*******************************************************************************
** Return codes
*******************************************************************************/
#define HT_OK            0
#define HT_NOT_FOUND    10
#define HT_DUPLICATE    11
#define HT_ALLOC_ERR    12
#define HT_ARG_NULL     13
#define HT_CORRUPTED    14
#define HT_MISUSE       15


/*******************************************************************************
** Constants
*******************************************************************************/
#define HT_DEFAULT_CAP  31
#define HT_GROW_FACT     2
#define HT_MAX_CAP       0.7
#define HT_NO_SIZE      -1

#define HT_TRANSIENT    ((void*)0)
#define HT_STATIC       ((void*)-1)

/*******************************************************************************
** Type aliases
*******************************************************************************/
typedef uint32_t ht_size_t;
typedef uint64_t ht_hash_t;

/* Function to free hash table entry data. */
typedef void(*ht_free_func_t)(void*);

/* Function to generate a hash from key data. */
typedef ht_hash_t(*ht_hash_func_t)(const void*, ht_size_t);

/*******************************************************************************
** Structs
*******************************************************************************/
typedef struct hash_table hash_table;


/*******************************************************************************
** Methods
*******************************************************************************/

/*
** Frees memory used by the hash table.
*/
HTPUBAPI void ht_free(hash_table *pHt);

/*
** Get an item with given key from hash table and set data and data_size pointers to the entry's 
** value and size.
*/
HTPUBAPI int ht_get_item(hash_table *pHt, const void *pKey, ht_size_t key_size, void **ppData, ht_size_t *pData_size);

/*
** Get the size of the hash table. If hash_table pointer is null, then HT_NO_SIZE is returned.
*/
HTPUBAPI ht_size_t ht_get_size(hash_table *pHt);

/*
** Checks if the key is contained in the hash table. Returns 1 if key is found, or 0 if key was not found.
** Also returns 0 if any pointer is null.
*/
HTPUBAPI int ht_has_key(hash_table *pHt, const void *pKey, ht_size_t key_size);

/*
** Initializes a hash table and places result in pHt. Returns HT_OK if create successfully.
*/
HTPUBAPI int ht_init(hash_table **ppHt);

/*
** Insert an item into the hash table. If an item alreay exists, then HT_DUPLICATE is returned.
** The final parameter is the deallocation method to deallocate the data. Pass HT_TRANSIENT to
** make a copy.
*/
HTPUBAPI int ht_insert(hash_table *pHt, const void *pKey, ht_size_t key_size, void *pData, ht_size_t data_size, ht_free_func_t pfnFree);

/*
** Remove the item with the given key from the hash table.
*/
HTPUBAPI int ht_remove(hash_table *pHt, const void *pKey, ht_size_t key_size);

/*
** Set the function to use when hashing items. This can only be changed if there are no items in the hash table, in which
** case HT_MISUSE will be returned.
*/
HTPUBAPI int ht_set_hash_func(hash_table *pHt, ht_hash_func_t pFn);

/*
** Helper functions that use a null-terminated string as a key. These simply forward the key to the
** "normal" functions. The null terminator is included in the key when calling "normal" functions.
*/
HTPUBAPI int ht_strk_get_item(hash_table *pHt, const char *pzKey, void **ppData, ht_size_t *pData_size);

HTPUBAPI int ht_strk_has_key(hash_table *pHt, const char *pzKey);

HTPUBAPI int ht_strk_insert(hash_table *pHt, const char *pzKey, void *pData, ht_size_t data_size, ht_free_func_t pfnFree);

HTPUBAPI int ht_strk_remove(hash_table *pHt, const char *pzKey);


#endif /* _HASH_TABLE_H */