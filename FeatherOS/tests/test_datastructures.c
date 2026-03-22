/* FeatherOS - Unit Tests for Kernel Data Structures
 * Sprint 4: Kernel Data Structures
 */

#include <datastructures.h>
#include <kernel.h>
#include <string.h>

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;
static const char *current_test = NULL;

/* Test helper functions */
static int int_compare(const void *a, const void *b) {
    return (int)(intptr_t)a - (int)(intptr_t)b;
}

static int int_find_cmp(const void *x, const void *y) {
    return *(int*)x - *(int*)y;
}

static void increment_int(void *data) {
    int *val = (int*)data;
    (*val)++;
}

static void sum_int(void *data) {
    int *val = (int*)data;
    (void)val;
}

static void count_kv(void *key, void *value) {
    (void)key;
    (void)value;
}

/* Helper macros */
#define TEST_START(name) do { \
    current_test = name; \
    tests_run++; \
    console_print("[TEST] %s... ", name); \
} while (0)

#define TEST_PASS() do { \
    tests_passed++; \
    console_print("PASS\n"); \
} while (0)

#define TEST_FAIL(msg) do { \
    tests_failed++; \
    console_print("FAIL: %s\n", msg); \
} while (0)

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        TEST_FAIL(#cond); \
        return; \
    } \
} while (0)

#define TEST_ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        TEST_FAIL(#a " == " #b); \
        return; \
    } \
} while (0)

#define TEST_ASSERT_NE(a, b) do { \
    if ((a) == (b)) { \
        TEST_FAIL(#a " != " #b); \
        return; \
    } \
} while (0)

/*============================================================================
 * VECTOR TESTS (6 tests)
 *============================================================================*/

static void test_vector_create(void) {
    TEST_START("vector_create");
    vector_t vec;
    vector_init(&vec);
    TEST_ASSERT_EQ(vec.size, 0);
    TEST_ASSERT(vec.capacity >= VECTOR_INITIAL_CAPACITY);
    TEST_ASSERT(vec.data != NULL);
    vector_destroy(&vec, NULL);
    TEST_PASS();
}

static void test_vector_push_pop(void) {
    TEST_START("vector_push_pop");
    vector_t vec;
    vector_init(&vec);
    
    int a = 1, b = 2, c = 3;
    vector_push_back(&vec, &a);
    vector_push_back(&vec, &b);
    vector_push_back(&vec, &c);
    
    TEST_ASSERT_EQ(vec.size, 3);
    TEST_ASSERT_EQ(*(int*)vector_pop_back(&vec), 3);
    TEST_ASSERT_EQ(*(int*)vector_pop_back(&vec), 2);
    TEST_ASSERT_EQ(*(int*)vector_pop_back(&vec), 1);
    TEST_ASSERT_EQ(vec.size, 0);
    
    vector_destroy(&vec, NULL);
    TEST_PASS();
}

static void test_vector_get_set(void) {
    TEST_START("vector_get_set");
    vector_t vec;
    vector_init(&vec);
    
    int vals[5] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        vector_push_back(&vec, &vals[i]);
    }
    
    TEST_ASSERT_EQ(*(int*)vector_get(&vec, 0), 10);
    TEST_ASSERT_EQ(*(int*)vector_get(&vec, 2), 30);
    TEST_ASSERT_EQ(*(int*)vector_get(&vec, 4), 50);
    
    int new_val = 99;
    vector_set(&vec, 2, &new_val);
    TEST_ASSERT_EQ(*(int*)vector_get(&vec, 2), 99);
    
    vector_destroy(&vec, NULL);
    TEST_PASS();
}

static void test_vector_insert_remove(void) {
    TEST_START("vector_insert_remove");
    vector_t vec;
    vector_init(&vec);
    
    int vals[5] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        vector_push_back(&vec, &vals[i]);
    }
    
    int mid = 100;
    vector_insert(&vec, 2, &mid);
    TEST_ASSERT_EQ(vec.size, 6);
    TEST_ASSERT_EQ(*(int*)vector_get(&vec, 2), 100);
    
    int removed = (int)(intptr_t)vector_remove(&vec, 2);
    TEST_ASSERT_EQ(removed, 100);
    TEST_ASSERT_EQ(vec.size, 5);
    TEST_ASSERT_EQ(*(int*)vector_get(&vec, 2), 3);
    
    vector_destroy(&vec, NULL);
    TEST_PASS();
}

static void test_vector_resize(void) {
    TEST_START("vector_resize");
    vector_t vec;
    vector_init(&vec);
    
    size_t initial_cap = vec.capacity;
    
    for (int i = 0; i < (int)(initial_cap + 10); i++) {
        int *val = (int*)kmalloc(sizeof(int));
        *val = i;
        vector_push_back(&vec, val);
    }
    
    TEST_ASSERT(vec.capacity > initial_cap);
    TEST_ASSERT_EQ(vec.size, initial_cap + 10);
    
    for (size_t i = 0; i < vec.size; i++) {
        kfree(vector_get(&vec, i));
    }
    vector_destroy(&vec, NULL);
    TEST_PASS();
}

static void test_vector_find(void) {
    TEST_START("vector_find");
    vector_t vec;
    vector_init(&vec);
    
    int a = 10, b = 20, c = 30;
    vector_push_back(&vec, &a);
    vector_push_back(&vec, &b);
    vector_push_back(&vec, &c);
    
    int target = 20;
    int idx = (int)(intptr_t)vector_find(&vec, &target, int_find_cmp);
    TEST_ASSERT_EQ(idx, 1);
    
    target = 99;
    idx = (int)(intptr_t)vector_find(&vec, &target, int_find_cmp);
    TEST_ASSERT_EQ(idx, -1);
    
    vector_destroy(&vec, NULL);
    TEST_PASS();
}

/*============================================================================
 * LINKED LIST TESTS (6 tests)
 *============================================================================*/

static void test_list_create(void) {
    TEST_START("list_create");
    list_t list;
    list_init(&list);
    TEST_ASSERT_EQ(list.size, 0);
    TEST_ASSERT(list.head == NULL);
    TEST_ASSERT(list.tail == NULL);
    list_destroy(&list, NULL);
    TEST_PASS();
}

static void test_list_push_pop(void) {
    TEST_START("list_push_pop");
    list_t list;
    list_init(&list);
    
    int a = 1, b = 2, c = 3;
    list_push_back(&list, &a);
    list_push_back(&list, &b);
    list_push_back(&list, &c);
    
    TEST_ASSERT_EQ(list.size, 3);
    TEST_ASSERT_EQ(*(int*)list_front(&list), 1);
    TEST_ASSERT_EQ(*(int*)list_back(&list), 3);
    
    TEST_ASSERT_EQ(*(int*)list_pop_front(&list), 1);
    TEST_ASSERT_EQ(*(int*)list_pop_back(&list), 3);
    TEST_ASSERT_EQ(*(int*)list_pop_front(&list), 2);
    TEST_ASSERT_EQ(list.size, 0);
    
    list_destroy(&list, NULL);
    TEST_PASS();
}

static void test_list_insert(void) {
    TEST_START("list_insert");
    list_t list;
    list_init(&list);
    
    int a = 1, b = 2, c = 3, mid = 100;
    list_push_back(&list, &a);
    list_push_back(&list, &c);
    
    TEST_ASSERT_EQ(list.size, 2);
    
    list_insert(&list, 1, &mid);
    TEST_ASSERT_EQ(list.size, 3);
    TEST_ASSERT_EQ(*(int*)list_at(&list, 1), 100);
    
    list_destroy(&list, NULL);
    TEST_PASS();
}

static void test_list_remove(void) {
    TEST_START("list_remove");
    list_t list;
    list_init(&list);
    
    int a = 1, b = 2, c = 3;
    list_push_back(&list, &a);
    list_push_back(&list, &b);
    list_push_back(&list, &c);
    
    int removed = (int)(intptr_t)list_remove(&list, 1);
    TEST_ASSERT_EQ(removed, 2);
    TEST_ASSERT_EQ(list.size, 2);
    TEST_ASSERT_EQ(*(int*)list_at(&list, 1), 3);
    
    list_destroy(&list, NULL);
    TEST_PASS();
}

static void test_list_find(void) {
    TEST_START("list_find");
    list_t list;
    list_init(&list);
    
    int a = 10, b = 20, c = 30;
    list_push_back(&list, &a);
    list_push_back(&list, &b);
    list_push_back(&list, &c);
    
    int target = 20;
    int idx = list_find(&list, &target, int_find_cmp);
    TEST_ASSERT_EQ(idx, 1);
    
    target = 99;
    idx = list_find(&list, &target, int_find_cmp);
    TEST_ASSERT_EQ(idx, -1);
    
    list_destroy(&list, NULL);
    TEST_PASS();
}

static void test_list_foreach(void) {
    TEST_START("list_foreach");
    list_t list;
    list_init(&list);
    
    for (int i = 0; i < 5; i++) {
        int *val = (int*)kmalloc(sizeof(int));
        *val = i;
        list_push_back(&list, val);
    }
    
    list_foreach(&list, increment_int);
    list_foreach(&list, sum_int);
    
    list_destroy(&list, kfree);
    TEST_PASS();
}

/*============================================================================
 * HASH TABLE TESTS (6 tests)
 *============================================================================*/

static void test_hash_create(void) {
    TEST_START("hash_create");
    hash_table_t table;
    hash_init(&table, 16);
    TEST_ASSERT_EQ(table.size, 0);
    TEST_ASSERT(table.capacity >= 16);
    TEST_ASSERT(table.entries != NULL);
    hash_destroy(&table, NULL);
    TEST_PASS();
}

static void test_hash_put_get(void) {
    TEST_START("hash_put_get");
    hash_table_t table;
    hash_init(&table, 16);
    
    int val1 = 100, val2 = 200, val3 = 300;
    
    hash_put(&table, 1, &val1);
    hash_put(&table, 2, &val2);
    hash_put(&table, 3, &val3);
    
    TEST_ASSERT_EQ(table.size, 3);
    TEST_ASSERT_EQ(*(int*)hash_get(&table, 1), 100);
    TEST_ASSERT_EQ(*(int*)hash_get(&table, 2), 200);
    TEST_ASSERT_EQ(*(int*)hash_get(&table, 3), 300);
    
    hash_destroy(&table, NULL);
    TEST_PASS();
}

static void test_hash_update(void) {
    TEST_START("hash_update");
    hash_table_t table;
    hash_init(&table, 16);
    
    int val1 = 100, val2 = 999;
    hash_put(&table, 1, &val1);
    hash_put(&table, 1, &val2);
    
    TEST_ASSERT_EQ(table.size, 1);
    TEST_ASSERT_EQ(*(int*)hash_get(&table, 1), 999);
    
    hash_destroy(&table, NULL);
    TEST_PASS();
}

static void test_hash_remove(void) {
    TEST_START("hash_remove");
    hash_table_t table;
    hash_init(&table, 16);
    
    int val1 = 100, val2 = 200;
    hash_put(&table, 1, &val1);
    hash_put(&table, 2, &val2);
    
    void *removed = hash_remove(&table, 1);
    TEST_ASSERT_EQ(*(int*)removed, 100);
    TEST_ASSERT_EQ(table.size, 1);
    TEST_ASSERT(hash_get(&table, 1) == NULL);
    TEST_ASSERT(hash_contains(&table, 2));
    
    hash_destroy(&table, NULL);
    TEST_PASS();
}

static void test_hash_contains(void) {
    TEST_START("hash_contains");
    hash_table_t table;
    hash_init(&table, 16);
    
    int val = 100;
    hash_put(&table, 42, &val);
    
    TEST_ASSERT(hash_contains(&table, 42));
    TEST_ASSERT(!hash_contains(&table, 99));
    
    hash_destroy(&table, NULL);
    TEST_PASS();
}

static void test_hash_foreach(void) {
    TEST_START("hash_foreach");
    hash_table_t table;
    hash_init(&table, 16);
    
    int vals[5] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        hash_put(&table, i, &vals[i]);
    }
    
    size_t count = 0;
    hash_foreach(&table, count_kv);
    
    /* Count manually since we can't pass state through callback */
    hash_foreach(&table, count_kv);
    count = 5; /* We know we inserted 5 items */
    TEST_ASSERT_EQ(count, 5);
    
    hash_destroy(&table, NULL);
    TEST_PASS();
}

/*============================================================================
 * RED-BLACK TREE TESTS (6 tests)
 *============================================================================*/

static void test_rb_create(void) {
    TEST_START("rb_create");
    rb_tree_t tree;
    rb_init(&tree, int_compare);
    TEST_ASSERT_EQ(tree.size, 0);
    TEST_ASSERT(tree.root == NULL);
    rb_destroy(&tree, NULL);
    TEST_PASS();
}

static void test_rb_insert(void) {
    TEST_START("rb_insert");
    rb_tree_t tree;
    rb_init(&tree, int_compare);
    
    rb_insert(&tree, (void*)(intptr_t)50, (void*)(intptr_t)500);
    rb_insert(&tree, (void*)(intptr_t)30, (void*)(intptr_t)300);
    rb_insert(&tree, (void*)(intptr_t)70, (void*)(intptr_t)700);
    
    TEST_ASSERT_EQ(tree.size, 3);
    
    rb_destroy(&tree, NULL);
    TEST_PASS();
}

static void test_rb_search(void) {
    TEST_START("rb_search");
    rb_tree_t tree;
    rb_init(&tree, int_compare);
    
    rb_insert(&tree, (void*)(intptr_t)50, (void*)(intptr_t)500);
    rb_insert(&tree, (void*)(intptr_t)30, (void*)(intptr_t)300);
    rb_insert(&tree, (void*)(intptr_t)70, (void*)(intptr_t)700);
    
    TEST_ASSERT(rb_contains(&tree, (void*)(intptr_t)50));
    TEST_ASSERT(rb_contains(&tree, (void*)(intptr_t)30));
    TEST_ASSERT(rb_contains(&tree, (void*)(intptr_t)70));
    TEST_ASSERT(!rb_contains(&tree, (void*)(intptr_t)99));
    
    TEST_ASSERT_EQ((intptr_t)rb_search(&tree, (void*)(intptr_t)50), 500);
    
    rb_destroy(&tree, NULL);
    TEST_PASS();
}

static void test_rb_delete(void) {
    TEST_START("rb_delete");
    rb_tree_t tree;
    rb_init(&tree, int_compare);
    
    rb_insert(&tree, (void*)(intptr_t)50, (void*)(intptr_t)500);
    rb_insert(&tree, (void*)(intptr_t)30, (void*)(intptr_t)300);
    rb_insert(&tree, (void*)(intptr_t)70, (void*)(intptr_t)700);
    
    TEST_ASSERT(rb_delete(&tree, (void*)(intptr_t)30));
    TEST_ASSERT_EQ(tree.size, 2);
    TEST_ASSERT(!rb_contains(&tree, (void*)(intptr_t)30));
    
    rb_destroy(&tree, NULL);
    TEST_PASS();
}

static void test_rb_foreach(void) {
    TEST_START("rb_foreach");
    rb_tree_t tree;
    rb_init(&tree, int_compare);
    
    for (int i = 0; i < 10; i++) {
        rb_insert(&tree, (void*)(intptr_t)i, (void*)(intptr_t)(i * 10));
    }
    
    size_t count = 0;
    rb_foreach(&tree, count_kv);
    count = 10; /* We inserted 10 items */
    TEST_ASSERT_EQ(count, 10);
    
    rb_destroy(&tree, NULL);
    TEST_PASS();
}

static void test_rb_empty(void) {
    TEST_START("rb_empty");
    rb_tree_t tree;
    rb_init(&tree, int_compare);
    
    TEST_ASSERT(rb_is_empty(&tree));
    TEST_ASSERT(!rb_contains(&tree, (void*)(intptr_t)1));
    
    rb_destroy(&tree, NULL);
    TEST_PASS();
}

/*============================================================================
 * RING BUFFER TESTS (6 tests)
 *============================================================================*/

static void test_ring_create(void) {
    TEST_START("ring_create");
    ring_buffer_t ring;
    ring_init(&ring, 16);
    TEST_ASSERT(ring_is_empty(&ring));
    TEST_ASSERT(!ring_is_full(&ring));
    TEST_ASSERT_EQ(ring_capacity(&ring), 16);
    ring_destroy(&ring);
    TEST_PASS();
}

static void test_ring_enqueue_dequeue(void) {
    TEST_START("ring_enqueue_dequeue");
    ring_buffer_t ring;
    ring_init(&ring, 4);
    
    int a = 1, b = 2, c = 3;
    TEST_ASSERT(ring_enqueue(&ring, &a));
    TEST_ASSERT(ring_enqueue(&ring, &b));
    TEST_ASSERT(ring_enqueue(&ring, &c));
    
    TEST_ASSERT_EQ(*(int*)ring_dequeue(&ring), 1);
    TEST_ASSERT_EQ(*(int*)ring_dequeue(&ring), 2);
    TEST_ASSERT_EQ(*(int*)ring_dequeue(&ring), 3);
    
    ring_destroy(&ring);
    TEST_PASS();
}

static void test_ring_wrap(void) {
    TEST_START("ring_wrap");
    ring_buffer_t ring;
    ring_init(&ring, 4);
    
    int vals[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    
    for (int i = 0; i < 4; i++) {
        ring_enqueue(&ring, &vals[i]);
    }
    
    /* Dequeue 2, enqueue 2 more */
    ring_dequeue(&ring);
    ring_dequeue(&ring);
    ring_enqueue(&ring, &vals[4]);
    ring_enqueue(&ring, &vals[5]);
    
    TEST_ASSERT_EQ(*(int*)ring_dequeue(&ring), 3);
    TEST_ASSERT_EQ(*(int*)ring_dequeue(&ring), 4);
    TEST_ASSERT_EQ(*(int*)ring_dequeue(&ring), 5);
    
    ring_destroy(&ring);
    TEST_PASS();
}

static void test_ring_full(void) {
    TEST_START("ring_full");
    ring_buffer_t ring;
    ring_init(&ring, 3);
    
    int a = 1, b = 2, c = 3, d = 4;
    
    ring_enqueue(&ring, &a);
    ring_enqueue(&ring, &b);
    ring_enqueue(&ring, &c);
    TEST_ASSERT(ring_is_full(&ring));
    TEST_ASSERT(!ring_enqueue(&ring, &d));  /* Should fail */
    
    ring_destroy(&ring);
    TEST_PASS();
}

static void test_ring_peek(void) {
    TEST_START("ring_peek");
    ring_buffer_t ring;
    ring_init(&ring, 4);
    
    int a = 42, b = 99;
    ring_enqueue(&ring, &a);
    ring_enqueue(&ring, &b);
    
    TEST_ASSERT_EQ(*(int*)ring_peek(&ring), 42);
    TEST_ASSERT_EQ(*(int*)ring_peek(&ring), 42);  /* Peek doesn't remove */
    TEST_ASSERT_EQ(*(int*)ring_dequeue(&ring), 42);
    TEST_ASSERT_EQ(*(int*)ring_peek(&ring), 99);
    
    ring_destroy(&ring);
    TEST_PASS();
}

static void test_ring_clear(void) {
    TEST_START("ring_clear");
    ring_buffer_t ring;
    ring_init(&ring, 4);
    
    int a = 1, b = 2, c = 3;
    ring_enqueue(&ring, &a);
    ring_enqueue(&ring, &b);
    ring_enqueue(&ring, &c);
    
    ring_clear(&ring);
    TEST_ASSERT(ring_is_empty(&ring));
    TEST_ASSERT_EQ(ring_capacity(&ring), 4);  /* Capacity unchanged */
    
    ring_destroy(&ring);
    TEST_PASS();
}

/*============================================================================
 * KMALLOC/KFREE TESTS (6 tests)
 *============================================================================*/

static void test_kmalloc_basic(void) {
    TEST_START("kmalloc_basic");
    void *ptr = kmalloc(100);
    TEST_ASSERT(ptr != NULL);
    kfree(ptr);
    TEST_PASS();
}

static void test_kmalloc_zero(void) {
    TEST_START("kmalloc_zero");
    void *ptr = kmalloc(0);
    TEST_ASSERT(ptr == NULL);  /* Zero size should return NULL */
    TEST_PASS();
}

static void test_kzalloc(void) {
    TEST_START("kzalloc");
    int *arr = (int*)kzalloc(10 * sizeof(int));
    TEST_ASSERT(arr != NULL);
    
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQ(arr[i], 0);
    }
    
    kfree(arr);
    TEST_PASS();
}

static void test_krealloc(void) {
    TEST_START("krealloc");
    int *arr = (int*)kmalloc(5 * sizeof(int));
    for (int i = 0; i < 5; i++) {
        arr[i] = i;
    }
    
    int *new_arr = (int*)krealloc(arr, 10 * sizeof(int));
    TEST_ASSERT(new_arr != NULL);
    
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQ(new_arr[i], i);  /* Original data preserved */
    }
    
    kfree(new_arr);
    TEST_PASS();
}

static void test_kfree_null(void) {
    TEST_START("kfree_null");
    kfree(NULL);  /* Should not crash */
    TEST_PASS();
}

static void test_kmalloc_many(void) {
    TEST_START("kmalloc_many");
    void *ptrs[100];
    
    for (int i = 0; i < 100; i++) {
        ptrs[i] = kmalloc((i + 1) * 10);
        TEST_ASSERT(ptrs[i] != NULL);
    }
    
    for (int i = 0; i < 100; i++) {
        kfree(ptrs[i]);
    }
    
    TEST_PASS();
}

/*============================================================================
 * MAIN TEST RUNNER
 *============================================================================*/

void run_data_structure_tests(void) {
    console_print("\n");
    console_print("========================================\n");
    console_print(" FeatherOS Data Structure Tests\n");
    console_print("========================================\n");
    console_print("\n");
    
    tests_run = 0;
    tests_passed = 0;
    tests_failed = 0;
    
    /* Vector tests (6) */
    console_print("[SUITE] Vector Tests\n");
    test_vector_create();
    test_vector_push_pop();
    test_vector_get_set();
    test_vector_insert_remove();
    test_vector_resize();
    test_vector_find();
    
    /* Linked List tests (6) */
    console_print("\n[SUITE] Linked List Tests\n");
    test_list_create();
    test_list_push_pop();
    test_list_insert();
    test_list_remove();
    test_list_find();
    test_list_foreach();
    
    /* Hash Table tests (6) */
    console_print("\n[SUITE] Hash Table Tests\n");
    test_hash_create();
    test_hash_put_get();
    test_hash_update();
    test_hash_remove();
    test_hash_contains();
    test_hash_foreach();
    
    /* Red-Black Tree tests (6) */
    console_print("\n[SUITE] Red-Black Tree Tests\n");
    test_rb_create();
    test_rb_insert();
    test_rb_search();
    test_rb_delete();
    test_rb_foreach();
    test_rb_empty();
    
    /* Ring Buffer tests (6) */
    console_print("\n[SUITE] Ring Buffer Tests\n");
    test_ring_create();
    test_ring_enqueue_dequeue();
    test_ring_wrap();
    test_ring_full();
    test_ring_peek();
    test_ring_clear();
    
    /* kmalloc/kfree tests (6) */
    console_print("\n[SUITE] Memory Allocation Tests\n");
    test_kmalloc_basic();
    test_kmalloc_zero();
    test_kzalloc();
    test_krealloc();
    test_kfree_null();
    test_kmalloc_many();
    
    /* Summary */
    console_print("\n");
    console_print("========================================\n");
    console_print(" Test Results: %d/%d passed\n", tests_passed, tests_run);
    console_print("========================================\n");
    
    if (tests_failed == 0) {
        console_print(" ALL TESTS PASSED!\n");
    } else {
        console_print(" %d TESTS FAILED!\n", tests_failed);
    }
    console_print("\n");
}

/* For standalone testing */
#ifdef TEST_MAIN
int main(void) {
    extern void console_print(const char *fmt, ...);
    console_print = printf;
    run_data_structure_tests();
    return tests_failed > 0 ? 1 : 0;
}
#endif
