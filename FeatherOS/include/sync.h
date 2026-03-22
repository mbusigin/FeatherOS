/* FeatherOS - Synchronization Primitives
 * Sprint 12: Synchronization Primitives
 */

#ifndef FEATHEROS_SYNC_H
#define FEATHEROS_SYNC_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * MEMORY BARRIERS
 *============================================================================*/

/* Compiler barrier - prevents compiler reordering */
#define barrier() __asm__ __volatile__("" ::: "memory")

/* Full memory barrier */
#define mb() __asm__ __volatile__("mfence" ::: "memory")

/* Read memory barrier */
#define rmb() __asm__ __volatile__("lfence" ::: "memory")

/* Write memory barrier */
#define wmb() __asm__ __volatile__("sfence" ::: "memory")

/*============================================================================
 * ATOMIC OPERATIONS
 *============================================================================*/

typedef struct {
    volatile int32_t counter;
} atomic32_t;

typedef struct {
    volatile int64_t counter;
} atomic64_t;

typedef struct {
    volatile uintptr_t value;
} atomic_ptr_t;

/* Initialize atomic variable */
#define ATOMIC_INIT(i) { (i) }

/* 32-bit atomic operations */
int32_t atomic_read(atomic32_t *v);
void atomic_set(atomic32_t *v, int32_t val);
void atomic_add(atomic32_t *v, int32_t val);
void atomic_sub(atomic32_t *v, int32_t val);
void atomic_inc(atomic32_t *v);
void atomic_dec(atomic32_t *v);
int32_t atomic_add_return(atomic32_t *v, int32_t val);
int32_t atomic_sub_return(atomic32_t *v, int32_t val);
bool atomic_compare_and_swap(atomic32_t *v, int32_t old_val, int32_t new_val);

/* 64-bit atomic operations */
int64_t atomic64_read(atomic64_t *v);
void atomic64_set(atomic64_t *v, int64_t val);
void atomic64_add(atomic64_t *v, int64_t val);
void atomic64_sub(atomic64_t *v, int64_t val);
void atomic64_inc(atomic64_t *v);
void atomic64_dec(atomic64_t *v);
int64_t atomic64_add_return(atomic64_t *v, int64_t val);
bool atomic64_compare_and_swap(atomic64_t *v, int64_t old_val, int64_t new_val);

/*============================================================================
 * SPINLOCK
 *============================================================================*/

typedef struct {
    volatile int32_t lock;
} spinlock_t;

#define SPINLOCK_UNLOCKED { 0 }
#define SPINLOCK_LOCKED   { 1 }

/* Initialize spinlock */
void spin_lock_init(spinlock_t *lock);

/* Acquire spinlock (spin until acquired) */
void spin_lock(spinlock_t *lock);

/* Release spinlock */
void spin_unlock(spinlock_t *lock);

/* Try to acquire spinlock (non-blocking) */
bool spin_trylock(spinlock_t *lock);

/* Check if spinlock is locked */
bool spin_is_locked(spinlock_t *lock);

/* Spin lock with interrupt disable */
void spin_lock_irq(spinlock_t *lock);
void spin_unlock_irq(spinlock_t *lock);

/*============================================================================
 * READ-WRITE LOCK
 *============================================================================*/

typedef struct {
    spinlock_t lock;
    volatile int32_t readers;
    volatile int32_t writers;
} rwlock_t;

#define RWLOCK_UNLOCKED { SPINLOCK_UNLOCKED, 0, 0 }

/* Initialize read-write lock */
void rwlock_init(rwlock_t *lock);

/* Read lock (multiple readers allowed) */
void read_lock(rwlock_t *lock);
void read_unlock(rwlock_t *lock);
bool read_trylock(rwlock_t *lock);

/* Write lock (exclusive access) */
void write_lock(rwlock_t *lock);
void write_unlock(rwlock_t *lock);
bool write_trylock(rwlock_t *lock);

/*============================================================================
 * MUTEX (Sleeping Lock)
 *============================================================================*/

typedef struct {
    volatile int32_t locked;
    void *owner;              /* Task that holds the lock */
    uint32_t count;           /* Recursion count */
} mutex_t;

#define MUTEX_UNLOCKED { 0, NULL, 0 }

/* Initialize mutex */
void mutex_init(mutex_t *mutex);

/* Lock mutex (may sleep) */
void mutex_lock(mutex_t *mutex);

/* Unlock mutex */
void mutex_unlock(mutex_t *mutex);

/* Try to lock mutex (non-blocking) */
bool mutex_trylock(mutex_t *mutex);

/* Check if mutex is locked */
bool mutex_is_locked(mutex_t *mutex);

/*============================================================================
 * SEMAPHORE
 *============================================================================*/

typedef struct {
    atomic32_t count;
    int32_t max_count;
} semaphore_t;

/* Initialize semaphore with count */
void sem_init(semaphore_t *sem, int32_t count, int32_t max_count);

/* Post (signal/v) - increment count */
void sem_post(semaphore_t *sem);

/* Wait (P/down) - decrement count, may sleep */
void sem_wait(semaphore_t *sem);

/* Try to wait (non-blocking) */
bool sem_trywait(semaphore_t *sem);

/* Get current count */
int32_t sem_getcount(semaphore_t *sem);

/*============================================================================
 * CONDITION VARIABLE (simple spin-based)
 *============================================================================*/

typedef struct {
    volatile int32_t signaled;
    spinlock_t lock;
} condvar_t;

/* Initialize condition variable */
void condvar_init(condvar_t *cv);

/* Wait on condition (busy-wait) */
void condvar_wait_spin(condvar_t *cv);

/* Signal one waiting thread */
void condvar_signal(condvar_t *cv);

/* Signal all waiting threads */
void condvar_broadcast(condvar_t *cv);

/*============================================================================
 * PER-CPU LOCKS
 *============================================================================*/

/* Per-CPU spinlock array */
typedef struct {
    spinlock_t locks[256];  /* Support up to 256 CPUs */
} per_cpu_spinlock_t;

/* Initialize per-CPU locks */
void per_cpu_spinlock_init(per_cpu_spinlock_t *pcpu);

/* Get current CPU ID */
int get_cpu_id(void);

/* Lock per-CPU lock for current CPU */
void per_cpu_spin_lock(per_cpu_spinlock_t *pcpu);

/* Unlock per-CPU lock for current CPU */
void per_cpu_spin_unlock(per_cpu_spinlock_t *pcpu);

/*============================================================================
 * SYNC STATISTICS
 *============================================================================*/

typedef struct {
    uint64_t spinlock_contentions;
    uint64_t mutex_blocks;
    uint64_t semaphore_waits;
    uint64_t deadlock_checks;
    uint64_t deadlocks_detected;
} sync_stats_t;

/* Get sync statistics */
sync_stats_t *sync_get_stats(void);

/* Print sync statistics */
void sync_print_stats(void);

/*============================================================================
 * LOCK DEBUGGING
 *============================================================================*/

#ifdef DEBUG
void spin_lock_debug(spinlock_t *lock, const char *file, int line);
void spin_unlock_debug(spinlock_t *lock, const char *file, int line);

#define SPIN_LOCK(lock) spin_lock_debug(lock, __FILE__, __LINE__)
#define SPIN_UNLOCK(lock) spin_unlock_debug(lock, __FILE__, __LINE__)
#else
#define SPIN_LOCK(lock) spin_lock(lock)
#define SPIN_UNLOCK(lock) spin_unlock(lock)
#endif

#endif /* FEATHEROS_SYNC_H */
