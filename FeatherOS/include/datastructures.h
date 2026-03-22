/* FeatherOS - Kernel Data Structures Library
 * Sprint 4: Kernel Data Structures
 * 
 * This library provides essential data structures for the kernel:
 * - Vector: Dynamic array with O(1) amortized append, O(1) random access
 * - Linked List: Doubly-linked list with O(1) insert/remove at known position
 * - Hash Table: Open-addressing hash table with O(1) average lookup
 * - Red-Black Tree: Self-balancing BST with O(log n) operations
 * - Ring Buffer: FIFO queue with O(1) enqueue/dequeue
 * - kmalloc/kfree: Simple memory allocator wrapper
 */

#ifndef FEATHEROS_DATASTRUCTURES_H
#define FEATHEROS_DATASTRUCTURES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*============================================================================
 * VECTOR - Dynamic Array
 * 
 * Time Complexity:
 *   - Access: O(1)
 *   - Append: O(1) amortized
 *   - Insert at position: O(n)
 *   - Remove at position: O(n)
 *   - Search: O(n)
 * 
 * Space Complexity: O(n)
 *============================================================================*/

#define VECTOR_INITIAL_CAPACITY 16

typedef struct {
    void **data;
    size_t size;
    size_t capacity;
} vector_t;

/* Vector operations */
void vector_init(vector_t *vec);
void vector_destroy(vector_t *vec, void (*free_fn)(void*));
size_t vector_size(const vector_t *vec);
bool vector_is_empty(const vector_t *vec);
void vector_push_back(vector_t *vec, void *item);
void *vector_pop_back(vector_t *vec);
void *vector_get(const vector_t *vec, size_t index);
void vector_set(vector_t *vec, size_t index, void *item);
void vector_insert(vector_t *vec, size_t index, void *item);
void *vector_remove(vector_t *vec, size_t index);
int vector_find(const vector_t *vec, const void *item, int (*cmp)(const void*, const void*));
void vector_foreach(vector_t *vec, void (*fn)(void*));

/*============================================================================
 * LINKED LIST - Doubly Linked List
 * 
 * Time Complexity:
 *   - Insert at head: O(1)
 *   - Insert at tail: O(1) with tail pointer
 *   - Insert at position: O(n)
 *   - Remove at head: O(1)
 *   - Remove at position: O(n)
 *   - Search: O(n)
 * 
 * Space Complexity: O(n)
 *============================================================================*/

typedef struct list_node {
    struct list_node *next;
    struct list_node *prev;
    void *data;
} list_node_t;

typedef struct {
    list_node_t *head;
    list_node_t *tail;
    size_t size;
} list_t;

/* List operations */
void list_init(list_t *list);
void list_destroy(list_t *list, void (*free_fn)(void*));
size_t list_size(const list_t *list);
bool list_is_empty(const list_t *list);
void list_push_front(list_t *list, void *data);
void list_push_back(list_t *list, void *data);
void *list_pop_front(list_t *list);
void *list_pop_back(list_t *list);
void *list_front(const list_t *list);
void *list_back(const list_t *list);
void list_insert(list_t *list, size_t index, void *data);
void *list_remove(list_t *list, size_t index);
void *list_at(const list_t *list, size_t index);
int list_find(const list_t *list, const void *data, int (*cmp)(const void*, const void*));
void list_foreach(list_t *list, void (*fn)(void*));

/*============================================================================
 * HASH TABLE - Open Addressing Hash Table
 * 
 * Time Complexity (average):
 *   - Insert: O(1)
 *   - Lookup: O(1)
 *   - Delete: O(1)
 * 
 * Worst case O(n) when many collisions
 * Space Complexity: O(n)
 *============================================================================*/

#define HASH_TABLE_LOAD_FACTOR 75  /* Percent */

typedef struct hash_entry {
    uint64_t key;
    void *value;
    bool occupied;
    bool deleted;
} hash_entry_t;

typedef struct {
    hash_entry_t *entries;
    size_t capacity;
    size_t size;
    size_t max_load;
} hash_table_t;

/* Hash table operations */
void hash_init(hash_table_t *table, size_t initial_capacity);
void hash_destroy(hash_table_t *table, void (*free_fn)(void*));
bool hash_put(hash_table_t *table, uint64_t key, void *value);
void *hash_get(const hash_table_t *table, uint64_t key);
void *hash_remove(hash_table_t *table, uint64_t key);
bool hash_contains(const hash_table_t *table, uint64_t key);
size_t hash_size(const hash_table_t *table);
bool hash_is_empty(const hash_table_t *table);
void hash_foreach(hash_table_t *table, void (*fn)(uint64_t key, void *value));

/* String key hash table */
typedef struct {
    char **keys;
    void **values;
    size_t capacity;
    size_t size;
} str_hash_table_t;

void str_hash_init(str_hash_table_t *table, size_t initial_capacity);
void str_hash_destroy(str_hash_table_t *table, void (*free_fn)(void*));
bool str_hash_put(str_hash_table_t *table, const char *key, void *value);
void *str_hash_get(const str_hash_table_t *table, const char *key);
void *str_hash_remove(str_hash_table_t *table, const char *key);
bool str_hash_contains(const str_hash_table_t *table, const char *key);
size_t str_hash_size(const str_hash_table_t *table);

/*============================================================================
 * RED-BLACK TREE - Self-Balancing Binary Search Tree
 * 
 * Time Complexity:
 *   - Insert: O(log n)
 *   - Delete: O(log n)
 *   - Search: O(log n)
 *   - Traversal: O(n)
 * 
 * Space Complexity: O(n)
 *============================================================================*/

typedef enum {
    RB_BLACK = 0,
    RB_RED = 1
} rb_color_t;

typedef struct rb_node {
    struct rb_node *left;
    struct rb_node *right;
    struct rb_node *parent;
    void *key;
    void *value;
    rb_color_t color;
} rb_node_t;

typedef struct {
    rb_node_t *root;
    size_t size;
    int (*compare)(const void *a, const void *b);
} rb_tree_t;

/* Red-Black Tree operations */
void rb_init(rb_tree_t *tree, int (*compare)(const void*, const void*));
void rb_destroy(rb_tree_t *tree, void (*free_fn)(void*));
bool rb_insert(rb_tree_t *tree, void *key, void *value);
bool rb_delete(rb_tree_t *tree, const void *key);
void *rb_search(const rb_tree_t *tree, const void *key);
bool rb_contains(const rb_tree_t *tree, const void *key);
size_t rb_size(const rb_tree_t *tree);
bool rb_is_empty(const rb_tree_t *tree);
void rb_foreach(const rb_tree_t *tree, void (*fn)(void *key, void *value));

/*============================================================================
 * RING BUFFER - Circular FIFO Queue
 * 
 * Time Complexity:
 *   - Enqueue: O(1)
 *   - Dequeue: O(1)
 *   - Peek: O(1)
 *   - Size: O(1)
 * 
 * Space Complexity: O(n) where n is capacity
 *============================================================================*/

typedef struct {
    void **buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t size;
    bool full;
} ring_buffer_t;

/* Ring buffer operations */
void ring_init(ring_buffer_t *ring, size_t capacity);
void ring_destroy(ring_buffer_t *ring);
bool ring_is_empty(const ring_buffer_t *ring);
bool ring_is_full(const ring_buffer_t *ring);
size_t ring_size(const ring_buffer_t *ring);
size_t ring_capacity(const ring_buffer_t *ring);
bool ring_enqueue(ring_buffer_t *ring, void *item);
void *ring_dequeue(ring_buffer_t *ring);
void *ring_peek(const ring_buffer_t *ring);
void ring_clear(ring_buffer_t *ring);

/*============================================================================
 * KMALLOC/KFREE - Simple Memory Allocator Wrapper
 * 
 * For now, uses the standard malloc/free.
 * Will be replaced with a proper kernel allocator later.
 *============================================================================*/

void *kmalloc(size_t size);
void *kzalloc(size_t size);
void *krealloc(void *ptr, size_t new_size);
void kfree(void *ptr);

/*============================================================================
 * ASSERTIONS - Kernel Assertion Framework
 *============================================================================*/

/* Assertion levels */
typedef enum {
    ASSERT_DEBUG = 0,
    ASSERT_WARN = 1,
    ASSERT_ERROR = 2
} assert_level_t;

extern assert_level_t current_assert_level;

/* Assertion macros */
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            kernel_assert(#condition, __FILE__, __LINE__, __func__); \
        } \
    } while (0)

#define ASSERT_MSG(condition, msg) \
    do { \
        if (!(condition)) { \
            kernel_assert_msg(#condition, msg, __FILE__, __LINE__, __func__); \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            kernel_assert_eq((a), (b), #a " == " #b, __FILE__, __LINE__, __func__); \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            kernel_assert_eq((a), (b), #a " != " #b, __FILE__, __LINE__, __func__); \
        } \
    } while (0)

#define ASSERT_LT(a, b) \
    do { \
        if (!((a) < (b))) { \
            kernel_assert_cmp((a), (b), "<", __FILE__, __LINE__, __func__); \
        } \
    } while (0)

#define ASSERT_GT(a, b) \
    do { \
        if (!((a) > (b))) { \
            kernel_assert_cmp((a), (b), ">", __FILE__, __LINE__, __func__); \
        } \
    } while (0)

#define ASSERT_LE(a, b) \
    do { \
        if (!((a) <= (b))) { \
            kernel_assert_cmp((a), (b), "<=", __FILE__, __LINE__, __func__); \
        } \
    } while (0)

#define ASSERT_GE(a, b) \
    do { \
        if (!((a) >= (b))) { \
            kernel_assert_cmp((a), (b), ">=", __FILE__, __LINE__, __func__); \
        } \
    } while (0)

#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            kernel_assert(#ptr " == NULL", __FILE__, __LINE__, __func__); \
        } \
    } while (0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            kernel_assert(#ptr " != NULL", __FILE__, __LINE__, __func__); \
        } \
    } while (0)

/* Assertion functions */
void kernel_assert(const char *condition, const char *file, int line, const char *func);
void kernel_assert_msg(const char *condition, const char *msg, const char *file, int line, const char *func);
void kernel_assert_eq(int64_t a, int64_t b, const char *expr, const char *file, int line, const char *func);
void kernel_assert_cmp(int64_t a, int64_t b, const char *op, const char *file, int line, const char *func);

/*============================================================================
 * UNIT TEST FRAMEWORK
 *============================================================================*/

typedef struct {
    const char *name;
    void (*test_fn)(void);
    bool passed;
    const char *error_msg;
} test_case_t;

typedef struct {
    const char *suite_name;
    test_case_t *tests;
    size_t num_tests;
    size_t num_passed;
    size_t num_failed;
    size_t num_skipped;
} test_suite_t;

/* Test macros */
#define TEST_CASE(name) static void test_##name(void)
#define RUN_TEST(suite, name) run_test(suite, #name, test_##name)

void run_test(test_suite_t *suite, const char *name, void (*fn)(void));
int run_all_tests(void);

extern test_suite_t data_structures_suite;

#endif /* FEATHEROS_DATASTRUCTURES_H */
