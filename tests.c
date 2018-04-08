#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

/*******************************************************************************
** Test Harness
******************************************************************************/
#define TEST_CASE

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define REQUIRE_TRUE(expr) do{ if(!_require_true(__LINE__, #expr, expr)) return; } while(0)

#define REQUIRE_STR_EQ(s1, s2) do{ if(!_require_str_equal(__LINE__, #s1, #s2, s1, s2)) return; } while(0)

#define REQUIRE_NOT_NULL(expr) do{ if(!_require_not_null(__LINE__, #expr, expr)) return; } while(0)

static int passed;
static int failed;

/*
** Tests that the given expression evaluates to true.
*/
static int _require_true(int line, const char *expr, int result)
{
    if(!result)
    {
        ++failed;

        printf("REQUIRE_TRUE failed! \n", line, expr);
        printf("    Line: %d \n", line);
        printf("    Expr: %s\n", expr);
        printf("\n");

        return 0;
    }
    else
    {
        ++passed;
        return 1;
    }
}

/*
** Tests that two strings are equal.
*/
static int _require_str_equal(int line, const char *expr1, const char *expr2, const char *str1, const char *str2)
{
    if(strcmp(str1, str2) != 0)
    {
        ++failed;

        printf("REQUIRE_STR_EQ failed! \n", line);
        printf("    Line: %d \n", line);
        printf("    Expr: %s != %s \n", expr1, expr2);
        printf("    s1  : \"%s\" \n", str1);
        printf("    s2  : \"%s\" \n", str2);
        printf("\n");

        return 0;
    }
    else
    {
        ++passed;
        return 1;
    }
}

/*
** Tests that a pointer is not null.
*/
static int _require_not_null(int line, const char *expr, void* ptr)
{
    if(ptr == NULL)
    {
        ++failed;

        printf("REQUIRE_NOT_NULL failed! \n", line);
        printf("    Line: %d \n", line);
        printf("    Expr: %s \n", expr);
        printf("\n");

        return 0;
    }
    else
    {
        ++passed;
        return 1;
    }
}

/*
** Print test results.
*/
static void print_test_results()
{
    printf("----------------------------- \n");
    printf("%d / %d Passed \n", passed, (passed + failed));
    printf("\n");
}

/*******************************************************************************
** Helper Functions for Testing
*******************************************************************************/
/*
** This function is deisgned to cause hash collisions for specific keys of
** "Test", "Test2", and "Test3". The hashes generated here will make the
** items enter into the bucket at index 1, assuming the hash table does not 
** grow.
*/
ht_hash_t dummy_hahser(const void *pKey, ht_size_t key_size)
{
    if(strcmp(pKey, "Test") == 0)
    {
        return 1;
    }
    else if(strcmp(pKey, "Test2") == 0)
    {
        return 1 + HT_DEFAULT_CAP;
    }
    else if(strcmp(pKey, "Test3") == 0)
    {
        return 1 + (HT_DEFAULT_CAP * 2);
    }
    else
    {
        printf("Warning: dummy_hahser misuse!\n");
        return 0;
    }
}

/*
** This is to help test custom free functions.
*/
static int s_dummy_free_called = 0;
void dummy_free(void *data)
{
    s_dummy_free_called = 1;
    free(data);
}

/*******************************************************************************
** Test Cases
*******************************************************************************/
TEST_CASE static void test_ht_init()
{
    hash_table *ht;
    int rc;

    rc = ht_init(&ht);

    REQUIRE_TRUE(rc == HT_OK);
    REQUIRE_NOT_NULL(ht);

    ht_free(ht);
}

TEST_CASE static void test_insert_transient()
{
    hash_table *ht;
    int rc;
    char data[] = "Data";

    char *actual_data;
    ht_size_t actual_data_size;

    rc = ht_init(&ht);
    REQUIRE_TRUE(rc == HT_OK);

    rc = ht_insert(ht, "Test", 5, data, 5, HT_TRANSIENT);
    REQUIRE_TRUE(rc == HT_OK);
    REQUIRE_TRUE(ht_get_size(ht) == 1);

    /*
    ** Clear local data to make sure hash table copied data.
    */
    memset(data, 0, sizeof(data));

    rc = ht_get_item(ht, "Test", 5, &actual_data, &actual_data_size);

    REQUIRE_TRUE(rc == HT_OK);
    REQUIRE_TRUE(actual_data_size == 5);
    REQUIRE_STR_EQ(actual_data, "Data");

    ht_free(ht);
}

TEST_CASE static void test_has_key()
{
    hash_table *ht;
    int rc;
    char data[] = "Data";

    rc = ht_init(&ht);
    REQUIRE_TRUE(rc == HT_OK);

    rc = ht_insert(ht, "Test", 5, data, 5, HT_TRANSIENT);
    REQUIRE_TRUE(rc == HT_OK);
    REQUIRE_TRUE(ht_get_size(ht) == 1);

    rc = ht_has_key(ht, "Test", 5);
    REQUIRE_TRUE(rc == 1);

    rc = ht_has_key(ht, "Nope", 5);
    REQUIRE_TRUE(rc == 0);

    ht_free(ht);
}

TEST_CASE static void test_remove()
{
    hash_table *ht;
    int rc;
    char data[] = "Data";

    rc = ht_init(&ht);
    REQUIRE_TRUE(rc == HT_OK);

    rc = ht_insert(ht, "Test", 5, data, 5, HT_TRANSIENT);
    REQUIRE_TRUE(rc == HT_OK);
    REQUIRE_TRUE(ht_get_size(ht) == 1);

    rc = ht_remove(ht, "Test", 5);
    REQUIRE_TRUE(rc == HT_OK);
    REQUIRE_TRUE(ht_get_size(ht) == 0);

    ht_free(ht);
}

TEST_CASE static void test_set_hash_func()
{
    hash_table *ht;
    int rc;
    char data[] = "Data";

    rc = ht_init(&ht);
    REQUIRE_TRUE(rc == HT_OK);

    rc = ht_set_hash_func(ht, &dummy_hahser);
    REQUIRE_TRUE(rc == HT_OK);

    rc = ht_insert(ht, "Test", 5, data, 5, HT_STATIC);
    REQUIRE_TRUE(rc == HT_OK);
    REQUIRE_TRUE(ht_get_size(ht) == 1);

    rc = ht_insert(ht, "Test2", 6, data, 5, HT_STATIC);
    REQUIRE_TRUE(rc == HT_OK);
    REQUIRE_TRUE(ht_get_size(ht) == 2);

    rc = ht_insert(ht, "Test3", 6, data, 5, HT_STATIC);
    REQUIRE_TRUE(rc == HT_OK);
    REQUIRE_TRUE(ht_get_size(ht) == 3);

    /*
    ** Removing the middle item should force a walk down a linked list. 
    ** This will test removing a middle item in a linked list like
    ** (test -> test2 -> test3)
    */
    rc = ht_remove(ht, "Test2", 5);
    REQUIRE_TRUE(rc == HT_OK);
    REQUIRE_TRUE(ht_get_size(ht) == 2);

    /*
    ** Ensure the two keys that were not removed still exist in the table.
    */
    REQUIRE_TRUE(ht_has_key(ht, "Test", 5) == 1);
    REQUIRE_TRUE(ht_has_key(ht, "Test3", 6) == 1);

    /* This should fail. */
    rc = ht_set_hash_func(ht, dummy_hahser);
    REQUIRE_TRUE(rc == HT_MISUSE);

    ht_free(ht);
}

TEST_CASE static void test_item_free_func()
{
    hash_table *ht;
    int rc;
    char *data;

    data = malloc(25);
    strcpy(data, "Data");

    rc = ht_init(&ht);
    REQUIRE_TRUE(rc == HT_OK);

    rc = ht_insert(ht, "Test", 5, data, 5, &dummy_free);
    REQUIRE_TRUE(rc == HT_OK);
    REQUIRE_TRUE(ht_get_size(ht) == 1);

    ht_free(ht);

    REQUIRE_TRUE(s_dummy_free_called == 1);
}

TEST_CASE static void test_strk_funcs()
{
    hash_table *ht;
    int rc;
    char data[] = "1234567890";
    char *out_data;
    ht_size_t out_data_size;

    rc = ht_init(&ht);
    REQUIRE_TRUE(rc == HT_OK);

    rc = ht_strk_insert(ht, "Key One", data, ARRAY_SIZE(data), HT_STATIC);
    REQUIRE_TRUE(rc == HT_OK);

    rc = ht_strk_has_key(ht, "Key One");
    REQUIRE_TRUE(rc == 1);

    rc = ht_strk_has_key(ht, "Not in it");
    REQUIRE_TRUE(rc == 0);

    rc = ht_strk_get_item(ht, "Key One", &out_data, &out_data_size);
    REQUIRE_TRUE(rc == HT_OK);
    REQUIRE_TRUE(out_data == data); /* Data is STATIC, so pointers will be same memory address. */
    REQUIRE_TRUE(out_data_size = ARRAY_SIZE(data));

    rc = ht_strk_remove(ht, "Key One");
    REQUIRE_TRUE(rc == HT_OK);

    ht_free(ht);
}

/*******************************************************************************
** Main Method
*******************************************************************************/
void main()
{
    printf("Hash Table Tests BEGIN.\n\n");

    test_ht_init();
    test_insert_transient();
    test_has_key();
    test_remove();
    test_set_hash_func();
    test_item_free_func();
    test_strk_funcs();

    print_test_results();
}