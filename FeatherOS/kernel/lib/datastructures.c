/* FeatherOS - Kernel Data Structures Implementation
 * Sprint 4: Kernel Data Structures
 */

#include <datastructures.h>
#include <string.h>

/* External memory functions - will be implemented properly in kernel */
extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *realloc(void *ptr, size_t new_size);

/* Forward declarations */
extern void console_print(const char *fmt, ...);
extern void *kmalloc(size_t size);
extern void *kzalloc(size_t size);
extern void kfree(void *ptr);

/*============================================================================
 * VECTOR IMPLEMENTATION
 *============================================================================*/

void vector_init(vector_t *vec) {
    vec->capacity = VECTOR_INITIAL_CAPACITY;
    vec->size = 0;
    vec->data = kmalloc(sizeof(void*) * vec->capacity);
    ASSERT_NOT_NULL(vec->data);
}

void vector_destroy(vector_t *vec, void (*free_fn)(void*)) {
    if (free_fn) {
        for (size_t i = 0; i < vec->size; i++) {
            free_fn(vec->data[i]);
        }
    }
    kfree(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

size_t vector_size(const vector_t *vec) { return vec->size; }
bool vector_is_empty(const vector_t *vec) { return vec->size == 0; }

static void vector_resize(vector_t *vec, size_t new_capacity) {
    void **new_data = kmalloc(sizeof(void*) * new_capacity);
    ASSERT_NOT_NULL(new_data);
    for (size_t i = 0; i < vec->size; i++) new_data[i] = vec->data[i];
    kfree(vec->data);
    vec->data = new_data;
    vec->capacity = new_capacity;
}

void vector_push_back(vector_t *vec, void *item) {
    if (vec->size >= vec->capacity) vector_resize(vec, vec->capacity * 2);
    vec->data[vec->size++] = item;
}

void *vector_pop_back(vector_t *vec) {
    return vec->size == 0 ? NULL : vec->data[--vec->size];
}

void *vector_get(const vector_t *vec, size_t index) {
    return index >= vec->size ? NULL : vec->data[index];
}

void vector_set(vector_t *vec, size_t index, void *item) {
    if (index < vec->size) vec->data[index] = item;
}

void vector_insert(vector_t *vec, size_t index, void *item) {
    if (index > vec->size) return;
    if (vec->size >= vec->capacity) vector_resize(vec, vec->capacity * 2);
    for (size_t i = vec->size; i > index; i--) vec->data[i] = vec->data[i - 1];
    vec->data[index] = item;
    vec->size++;
}

void *vector_remove(vector_t *vec, size_t index) {
    if (index >= vec->size) return NULL;
    void *item = vec->data[index];
    for (size_t i = index; i < vec->size - 1; i++) vec->data[i] = vec->data[i + 1];
    vec->size--;
    return item;
}

int vector_find(const vector_t *vec, const void *item, int (*cmp)(const void*, const void*)) {
    for (size_t i = 0; i < vec->size; i++) {
        if (cmp(vec->data[i], item) == 0) return i;
    }
    return -1;
}

void vector_foreach(vector_t *vec, void (*fn)(void*)) {
    for (size_t i = 0; i < vec->size; i++) fn(vec->data[i]);
}

/*============================================================================
 * LINKED LIST IMPLEMENTATION
 *============================================================================*/

void list_init(list_t *list) {
    list->head = list->tail = NULL;
    list->size = 0;
}

void list_destroy(list_t *list, void (*free_fn)(void*)) {
    list_node_t *current = list->head;
    while (current) {
        list_node_t *next = current->next;
        if (free_fn) free_fn(current->data);
        kfree(current);
        current = next;
    }
    list->head = list->tail = NULL;
    list->size = 0;
}

size_t list_size(const list_t *list) { return list->size; }
bool list_is_empty(const list_t *list) { return list->size == 0; }

void list_push_front(list_t *list, void *data) {
    list_node_t *node = kmalloc(sizeof(list_node_t));
    ASSERT_NOT_NULL(node);
    node->data = data;
    node->prev = NULL;
    node->next = list->head;
    if (list->head) list->head->prev = node;
    list->head = node;
    if (!list->tail) list->tail = node;
    list->size++;
}

void list_push_back(list_t *list, void *data) {
    list_node_t *node = kmalloc(sizeof(list_node_t));
    ASSERT_NOT_NULL(node);
    node->data = data;
    node->next = NULL;
    node->prev = list->tail;
    if (list->tail) list->tail->next = node;
    list->tail = node;
    if (!list->head) list->head = node;
    list->size++;
}

void *list_pop_front(list_t *list) {
    if (list->size == 0) return NULL;
    list_node_t *node = list->head;
    void *data = node->data;
    list->head = node->next;
    if (list->head) list->head->prev = NULL;
    else list->tail = NULL;
    kfree(node);
    list->size--;
    return data;
}

void *list_pop_back(list_t *list) {
    if (list->size == 0) return NULL;
    list_node_t *node = list->tail;
    void *data = node->data;
    list->tail = node->prev;
    if (list->tail) list->tail->next = NULL;
    else list->head = NULL;
    kfree(node);
    list->size--;
    return data;
}

void *list_front(const list_t *list) { return list->size == 0 ? NULL : list->head->data; }
void *list_back(const list_t *list) { return list->size == 0 ? NULL : list->tail->data; }

void list_insert(list_t *list, size_t index, void *data) {
    if (index == 0) { list_push_front(list, data); return; }
    if (index >= list->size) { list_push_back(list, data); return; }
    list_node_t *node = kmalloc(sizeof(list_node_t));
    ASSERT_NOT_NULL(node);
    node->data = data;
    list_node_t *current = list->head;
    for (size_t i = 0; i < index - 1; i++) current = current->next;
    node->next = current->next;
    node->prev = current;
    current->next->prev = node;
    current->next = node;
    list->size++;
}

void *list_remove(list_t *list, size_t index) {
    if (index >= list->size) return NULL;
    if (index == 0) return list_pop_front(list);
    if (index == list->size - 1) return list_pop_back(list);
    list_node_t *current = list->head;
    for (size_t i = 0; i < index; i++) current = current->next;
    current->prev->next = current->next;
    current->next->prev = current->prev;
    void *data = current->data;
    kfree(current);
    list->size--;
    return data;
}

void *list_at(const list_t *list, size_t index) {
    if (index >= list->size) return NULL;
    list_node_t *current = list->head;
    for (size_t i = 0; i < index; i++) current = current->next;
    return current->data;
}

int list_find(const list_t *list, const void *data, int (*cmp)(const void*, const void*)) {
    list_node_t *current = list->head;
    size_t index = 0;
    while (current) {
        if (cmp(current->data, data) == 0) return index;
        current = current->next;
        index++;
    }
    return -1;
}

void list_foreach(list_t *list, void (*fn)(void*)) {
    list_node_t *current = list->head;
    while (current) { fn(current->data); current = current->next; }
}

/*============================================================================
 * HASH TABLE IMPLEMENTATION
 *============================================================================*/

static size_t hash_function(uint64_t key, size_t capacity) {
    (void)capacity;
    key = ((key >> 16) ^ key) * 0x45d9f3b;
    key = ((key >> 16) ^ key) * 0x45d9f3b;
    return (key >> 16) ^ key;
}

void hash_init(hash_table_t *table, size_t initial_capacity) {
    table->capacity = initial_capacity > 0 ? initial_capacity : 16;
    table->size = 0;
    table->max_load = (table->capacity * HASH_TABLE_LOAD_FACTOR) / 100;
    table->entries = kmalloc(sizeof(hash_entry_t) * table->capacity);
    ASSERT_NOT_NULL(table->entries);
    for (size_t i = 0; i < table->capacity; i++) {
        table->entries[i].occupied = false;
        table->entries[i].deleted = false;
    }
}

void hash_destroy(hash_table_t *table, void (*free_fn)(void*)) {
    if (free_fn) {
        for (size_t i = 0; i < table->capacity; i++) {
            if (table->entries[i].occupied && !table->entries[i].deleted) free_fn(table->entries[i].value);
        }
    }
    kfree(table->entries);
    table->entries = NULL;
    table->size = 0;
}

bool hash_put(hash_table_t *table, uint64_t key, void *value) {
    if (table->size >= table->max_load) return false;  /* Would need resize */
    size_t index = hash_function(key, table->capacity) % table->capacity;
    size_t original_index = index;
    while (true) {
        if (!table->entries[index].occupied || table->entries[index].deleted) {
            table->entries[index].key = key;
            table->entries[index].value = value;
            table->entries[index].occupied = true;
            table->entries[index].deleted = false;
            table->size++;
            return true;
        }
        if (table->entries[index].key == key && !table->entries[index].deleted) {
            table->entries[index].value = value;
            return true;
        }
        index = (index + 1) % table->capacity;
        if (index == original_index) return false;
    }
}

void *hash_get(const hash_table_t *table, uint64_t key) {
    size_t index = hash_function(key, table->capacity) % table->capacity;
    size_t original_index = index;
    while (true) {
        if (!table->entries[index].occupied) return NULL;
        if (table->entries[index].key == key && !table->entries[index].deleted) return table->entries[index].value;
        index = (index + 1) % table->capacity;
        if (index == original_index) return NULL;
    }
}

void *hash_remove(hash_table_t *table, uint64_t key) {
    size_t index = hash_function(key, table->capacity) % table->capacity;
    size_t original_index = index;
    while (true) {
        if (!table->entries[index].occupied) return NULL;
        if (table->entries[index].key == key && !table->entries[index].deleted) {
            void *value = table->entries[index].value;
            table->entries[index].deleted = true;
            table->size--;
            return value;
        }
        index = (index + 1) % table->capacity;
        if (index == original_index) return NULL;
    }
}

bool hash_contains(const hash_table_t *table, uint64_t key) { return hash_get(table, key) != NULL; }
size_t hash_size(const hash_table_t *table) { return table->size; }
bool hash_is_empty(const hash_table_t *table) { return table->size == 0; }

void hash_foreach(hash_table_t *table, void (*fn)(uint64_t key, void *value)) {
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->entries[i].occupied && !table->entries[i].deleted) fn(table->entries[i].key, table->entries[i].value);
    }
}

/*============================================================================
 * STRING HASH TABLE IMPLEMENTATION
 *============================================================================*/

static size_t str_hash_function(const char *key, size_t capacity) {
    size_t hash = 5381;
    while (*key) { hash = ((hash << 5) + hash) + *key++; }
    return hash % capacity;
}

void str_hash_init(str_hash_table_t *table, size_t initial_capacity) {
    table->capacity = initial_capacity > 0 ? initial_capacity : 16;
    table->size = 0;
    table->keys = kmalloc(sizeof(char*) * table->capacity);
    table->values = kmalloc(sizeof(void*) * table->capacity);
    ASSERT_NOT_NULL(table->keys);
    ASSERT_NOT_NULL(table->values);
    for (size_t i = 0; i < table->capacity; i++) table->keys[i] = NULL;
}

void str_hash_destroy(str_hash_table_t *table, void (*free_fn)(void*)) {
    if (free_fn) {
        for (size_t i = 0; i < table->capacity; i++) {
            if (table->keys[i]) free_fn(table->values[i]);
        }
    }
    kfree(table->keys);
    kfree(table->values);
    table->keys = NULL;
    table->values = NULL;
    table->size = 0;
}

bool str_hash_put(str_hash_table_t *table, const char *key, void *value) {
    size_t index = str_hash_function(key, table->capacity);
    size_t original_index = index;
    while (table->keys[index] != NULL) {
        if (strcmp(table->keys[index], key) == 0) { table->values[index] = value; return true; }
        index = (index + 1) % table->capacity;
        if (index == original_index) return false;
    }
    table->keys[index] = (char*)key;
    table->values[index] = value;
    table->size++;
    return true;
}

void *str_hash_get(const str_hash_table_t *table, const char *key) {
    size_t index = str_hash_function(key, table->capacity);
    size_t original_index = index;
    while (table->keys[index] != NULL) {
        if (strcmp(table->keys[index], key) == 0) return table->values[index];
        index = (index + 1) % table->capacity;
        if (index == original_index) return NULL;
    }
    return NULL;
}

void *str_hash_remove(str_hash_table_t *table, const char *key) {
    size_t index = str_hash_function(key, table->capacity);
    size_t original_index = index;
    while (table->keys[index] != NULL) {
        if (strcmp(table->keys[index], key) == 0) {
            void *value = table->values[index];
            table->keys[index] = NULL;
            table->size--;
            return value;
        }
        index = (index + 1) % table->capacity;
        if (index == original_index) return NULL;
    }
    return NULL;
}

bool str_hash_contains(const str_hash_table_t *table, const char *key) { return str_hash_get(table, key) != NULL; }
size_t str_hash_size(const str_hash_table_t *table) { return table->size; }

/*============================================================================
 * RING BUFFER IMPLEMENTATION
 *============================================================================*/

void ring_init(ring_buffer_t *ring, size_t capacity) {
    ring->capacity = capacity;
    ring->buffer = kmalloc(sizeof(void*) * capacity);
    ASSERT_NOT_NULL(ring->buffer);
    ring->head = ring->tail = ring->size = 0;
    ring->full = false;
}

void ring_destroy(ring_buffer_t *ring) {
    kfree(ring->buffer);
    ring->buffer = NULL;
    ring->size = ring->capacity = 0;
}

bool ring_is_empty(const ring_buffer_t *ring) { return ring->size == 0 && !ring->full; }
bool ring_is_full(const ring_buffer_t *ring) { return ring->full; }
size_t ring_size(const ring_buffer_t *ring) { return ring->size; }
size_t ring_capacity(const ring_buffer_t *ring) { return ring->capacity; }

bool ring_enqueue(ring_buffer_t *ring, void *item) {
    if (ring->full) return false;
    ring->buffer[ring->tail] = item;
    ring->tail = (ring->tail + 1) % ring->capacity;
    ring->size++;
    if (ring->tail == ring->head) ring->full = true;
    return true;
}

void *ring_dequeue(ring_buffer_t *ring) {
    if (ring_is_empty(ring)) return NULL;
    void *item = ring->buffer[ring->head];
    ring->head = (ring->head + 1) % ring->capacity;
    ring->size--;
    ring->full = false;
    return item;
}

void *ring_peek(const ring_buffer_t *ring) {
    return ring_is_empty(ring) ? NULL : ring->buffer[ring->head];
}

void ring_clear(ring_buffer_t *ring) {
    ring->head = ring->tail = ring->size = 0;
    ring->full = false;
}

/*============================================================================
 * KMALLOC/KFREE IMPLEMENTATION - Simple Bump Allocator
 *============================================================================*/

/* Simple bump allocator for kernel memory */
#define HEAP_START 0x200000
#define HEAP_SIZE  (1024 * 1024)  /* 1MB heap */
static uint8_t *heap_ptr = (uint8_t*)HEAP_START;
static size_t heap_left = HEAP_SIZE;

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    /* Align to 8 bytes */
    size = (size + 7) & ~7;
    if (size > heap_left) return NULL;
    void *ptr = heap_ptr;
    heap_ptr += size;
    heap_left -= size;
    return ptr;
}

void *kzalloc(size_t size) {
    void *ptr = kmalloc(size);
    if (ptr) memset(ptr, 0, size);
    return ptr;
}

void *krealloc(void *ptr, size_t new_size) {
    (void)ptr;
    return kmalloc(new_size);  /* Can't easily resize, just allocate new */
}

void kfree(void *ptr) {
    (void)ptr;  /* Bump allocator - can't free individual blocks */
    /* Could implement a simple free list later */
}

/*============================================================================
 * ASSERTIONS IMPLEMENTATION
 *============================================================================*/

assert_level_t current_assert_level = ASSERT_DEBUG;

void kernel_assert(const char *condition, const char *file, int line, const char *func) {
    console_print("ASSERTION FAILED: %s\n", condition);
    console_print("  File: %s, Line: %d, Function: %s\n", file, line, func);
}

void kernel_assert_msg(const char *condition, const char *msg, const char *file, int line, const char *func) {
    console_print("ASSERTION FAILED: %s\n", condition);
    console_print("  Message: %s\n", msg);
    console_print("  File: %s, Line: %d, Function: %s\n", file, line, func);
}

void kernel_assert_eq(int64_t a, int64_t b, const char *expr, const char *file, int line, const char *func) {
    console_print("ASSERTION FAILED: %s\n", expr);
    console_print("  Expected: %ld, Got: %ld\n", a, b);
    console_print("  File: %s, Line: %d, Function: %s\n", file, line, func);
}

void kernel_assert_cmp(int64_t a, int64_t b, const char *op, const char *file, int line, const char *func) {
    console_print("ASSERTION FAILED: %ld %s %ld\n", a, op, b);
    console_print("  File: %s, Line: %d, Function: %s\n", file, line, func);
}
