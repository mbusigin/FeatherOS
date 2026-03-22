/* FeatherOS - Synchronization Primitives Implementation
 * Sprint 12: Synchronization Primitives
 */

#include <sync.h>
#include <kernel.h>

/*============================================================================
 * STATISTICS
 *============================================================================*/

static sync_stats_t sync_stats = {0};

/*============================================================================
 * ATOMIC 32-BIT OPERATIONS
 *============================================================================*/

int32_t atomic_read(atomic32_t *v) {
    return v->counter;
}

void atomic_set(atomic32_t *v, int32_t val) {
    v->counter = val;
}

void atomic_add(atomic32_t *v, int32_t val) {
    __asm__ __volatile__(
        "lock addl %1, %0"
        : "+m"(v->counter)
        : "ir"(val)
        : "memory"
    );
}

void atomic_sub(atomic32_t *v, int32_t val) {
    __asm__ __volatile__(
        "lock subl %1, %0"
        : "+m"(v->counter)
        : "ir"(val)
        : "memory"
    );
}

void atomic_inc(atomic32_t *v) {
    __asm__ __volatile__(
        "lock incl %0"
        : "+m"(v->counter)
        :: "memory"
    );
}

void atomic_dec(atomic32_t *v) {
    __asm__ __volatile__(
        "lock decl %0"
        : "+m"(v->counter)
        :: "memory"
    );
}

int32_t atomic_add_return(atomic32_t *v, int32_t val) {
    int32_t result;
    __asm__ __volatile__(
        "lock xaddl %0, %1"
        : "=r"(result), "+m"(v->counter)
        : "0"(val)
        : "memory"
    );
    return result + val;
}

int32_t atomic_sub_return(atomic32_t *v, int32_t val) {
    return atomic_add_return(v, -val);
}

bool atomic_compare_and_swap(atomic32_t *v, int32_t old_val, int32_t new_val) {
    int32_t prev;
    __asm__ __volatile__(
        "lock cmpxchgl %2, %1"
        : "=a"(prev), "+m"(v->counter)
        : "r"(new_val), "0"(old_val)
        : "memory"
    );
    return prev == old_val;
}

/*============================================================================
 * ATOMIC 64-BIT OPERATIONS
 *============================================================================*/

int64_t atomic64_read(atomic64_t *v) {
    return v->counter;
}

void atomic64_set(atomic64_t *v, int64_t val) {
    v->counter = val;
}

void atomic64_add(atomic64_t *v, int64_t val) {
    __asm__ __volatile__(
        "lock addq %1, %0"
        : "+m"(v->counter)
        : "er"(val)
        : "memory"
    );
}

void atomic64_sub(atomic64_t *v, int64_t val) {
    __asm__ __volatile__(
        "lock subq %1, %0"
        : "+m"(v->counter)
        : "er"(val)
        : "memory"
    );
}

void atomic64_inc(atomic64_t *v) {
    __asm__ __volatile__(
        "lock incq %0"
        : "+m"(v->counter)
        :: "memory"
    );
}

void atomic64_dec(atomic64_t *v) {
    __asm__ __volatile__(
        "lock decq %0"
        : "+m"(v->counter)
        :: "memory"
    );
}

int64_t atomic64_add_return(atomic64_t *v, int64_t val) {
    int64_t result;
    __asm__ __volatile__(
        "lock xaddq %0, %1"
        : "=r"(result), "+m"(v->counter)
        : "0"(val)
        : "memory"
    );
    return result + val;
}

bool atomic64_compare_and_swap(atomic64_t *v, int64_t old_val, int64_t new_val) {
    int64_t prev;
    __asm__ __volatile__(
        "lock cmpxchgq %2, %1"
        : "=a"(prev), "+m"(v->counter)
        : "r"(new_val), "0"(old_val)
        : "memory"
    );
    return prev == old_val;
}

/*============================================================================
 * SPINLOCK
 *============================================================================*/

void spin_lock_init(spinlock_t *lock) {
    lock->lock = 0;
}

void spin_lock(spinlock_t *lock) {
    while (1) {
        /* Try to acquire lock */
        if (__sync_bool_compare_and_swap(&lock->lock, 0, 1)) {
            barrier();
            return;
        }
        /* Contention detected - retry */
        sync_stats.spinlock_contentions++;
        /* Hint to processor to allow other threads to run */
        __asm__ __volatile__("pause" ::: "memory");
    }
}

void spin_unlock(spinlock_t *lock) {
    barrier();
    lock->lock = 0;
}

bool spin_trylock(spinlock_t *lock) {
    if (__sync_bool_compare_and_swap(&lock->lock, 0, 1)) {
        barrier();
        return true;
    }
    return false;
}

bool spin_is_locked(spinlock_t *lock) {
    return lock->lock != 0;
}

/* With IRQ save/restore (for interrupt context) */
static uint64_t irq_flags_save;

void spin_lock_irq(spinlock_t *lock) {
    __asm__ __volatile__(
        "pushfq; cli; popq %0"
        : "=r"(irq_flags_save)
        :: "memory"
    );
    spin_lock(lock);
}

void spin_unlock_irq(spinlock_t *lock) {
    spin_unlock(lock);
    __asm__ __volatile__(
        "pushq %0; popfq"
        :: "r"(irq_flags_save)
        : "memory"
    );
}

/*============================================================================
 * READ-WRITE LOCK
 *============================================================================*/

void rwlock_init(rwlock_t *lock) {
    spin_lock_init(&lock->lock);
    lock->readers = 0;
    lock->writers = 0;
}

void read_lock(rwlock_t *lock) {
    spin_lock(&lock->lock);
    while (lock->writers) {
        spin_unlock(&lock->lock);
        /* Wait for writers to release */
        __asm__ __volatile__("pause" ::: "memory");
        spin_lock(&lock->lock);
    }
    lock->readers++;
    spin_unlock(&lock->lock);
}

void read_unlock(rwlock_t *lock) {
    spin_lock(&lock->lock);
    lock->readers--;
    spin_unlock(&lock->lock);
}

bool read_trylock(rwlock_t *lock) {
    spin_lock(&lock->lock);
    if (lock->writers) {
        spin_unlock(&lock->lock);
        return false;
    }
    lock->readers++;
    spin_unlock(&lock->lock);
    return true;
}

void write_lock(rwlock_t *lock) {
    spin_lock(&lock->lock);
    while (lock->readers || lock->writers) {
        spin_unlock(&lock->lock);
        __asm__ __volatile__("pause" ::: "memory");
        spin_lock(&lock->lock);
    }
    lock->writers = 1;
    spin_unlock(&lock->lock);
}

void write_unlock(rwlock_t *lock) {
    spin_lock(&lock->lock);
    lock->writers = 0;
    spin_unlock(&lock->lock);
}

bool write_trylock(rwlock_t *lock) {
    spin_lock(&lock->lock);
    if (lock->readers || lock->writers) {
        spin_unlock(&lock->lock);
        return false;
    }
    lock->writers = 1;
    spin_unlock(&lock->lock);
    return true;
}

/*============================================================================
 * MUTEX
 *============================================================================*/

void mutex_init(mutex_t *mutex) {
    mutex->locked = 0;
    mutex->owner = NULL;
    mutex->count = 0;
}

void mutex_lock(mutex_t *mutex) {
    /* Try fast path */
    if (atomic_compare_and_swap((atomic32_t*)mutex, 0, 1)) {
        mutex->owner = (void*)1;  /* Simple marker */
        return;
    }
    
    /* Slow path - wait */
    sync_stats.mutex_blocks++;
    
    while (!atomic_compare_and_swap((atomic32_t*)mutex, 0, 1)) {
        /* Wait for unlock */
        __asm__ __volatile__("pause" ::: "memory");
    }
    mutex->owner = (void*)1;
}

void mutex_unlock(mutex_t *mutex) {
    mutex->owner = NULL;
    atomic_set((atomic32_t*)mutex, 0);
}

bool mutex_trylock(mutex_t *mutex) {
    return atomic_compare_and_swap((atomic32_t*)mutex, 0, 1);
}

bool mutex_is_locked(mutex_t *mutex) {
    return atomic_read((atomic32_t*)mutex) != 0;
}

/*============================================================================
 * SEMAPHORE
 *============================================================================*/

void sem_init(semaphore_t *sem, int32_t count, int32_t max_count) {
    atomic_set(&sem->count, count);
    sem->max_count = max_count;
}

void sem_post(semaphore_t *sem) {
    atomic_inc(&sem->count);
}

void sem_wait(semaphore_t *sem) {
    sync_stats.semaphore_waits++;
    
    while (1) {
        int32_t old = atomic_read(&sem->count);
        if (old <= 0) {
            /* Wait */
            __asm__ __volatile__("pause" ::: "memory");
            continue;
        }
        if (atomic_compare_and_swap(&sem->count, old, old - 1)) {
            return;
        }
    }
}

bool sem_trywait(semaphore_t *sem) {
    int32_t old = atomic_read(&sem->count);
    while (old > 0) {
        if (atomic_compare_and_swap(&sem->count, old, old - 1)) {
            return true;
        }
        old = atomic_read(&sem->count);
    }
    return false;
}

int32_t sem_getcount(semaphore_t *sem) {
    return atomic_read(&sem->count);
}

/*============================================================================
 * CONDITION VARIABLE (simple spin-based)
 *============================================================================*/

void condvar_init(condvar_t *cv) {
    cv->signaled = 0;
    spin_lock_init(&cv->lock);
}

void condvar_wait_spin(condvar_t *cv) {
    spin_lock(&cv->lock);
    while (!cv->signaled) {
        spin_unlock(&cv->lock);
        __asm__ __volatile__("pause" ::: "memory");
        spin_lock(&cv->lock);
    }
    cv->signaled = 0;
    spin_unlock(&cv->lock);
}

void condvar_signal(condvar_t *cv) {
    spin_lock(&cv->lock);
    cv->signaled = 1;
    spin_unlock(&cv->lock);
}

void condvar_broadcast(condvar_t *cv) {
    spin_lock(&cv->lock);
    cv->signaled = 1;
    spin_unlock(&cv->lock);
}

/*============================================================================
 * PER-CPU LOCKS
 *============================================================================*/

void per_cpu_spinlock_init(per_cpu_spinlock_t *pcpu) {
    for (int i = 0; i < 256; i++) {
        spin_lock_init(&pcpu->locks[i]);
    }
}

int get_cpu_id(void) {
    /* Simplified - assume single CPU (CPU 0) */
    return 0;
}

void per_cpu_spin_lock(per_cpu_spinlock_t *pcpu) {
    int cpu = get_cpu_id();
    spin_lock(&pcpu->locks[cpu]);
}

void per_cpu_spin_unlock(per_cpu_spinlock_t *pcpu) {
    int cpu = get_cpu_id();
    spin_unlock(&pcpu->locks[cpu]);
}

/*============================================================================
 * STATISTICS
 *============================================================================*/

sync_stats_t *sync_get_stats(void) {
    return &sync_stats;
}

void sync_print_stats(void) {
    console_print("Synchronization Statistics:\n");
    console_print("  Spinlock contentions: %lu\n", sync_stats.spinlock_contentions);
    console_print("  Mutex blocks: %lu\n", sync_stats.mutex_blocks);
    console_print("  Semaphore waits: %lu\n", sync_stats.semaphore_waits);
    console_print("  Deadlock checks: %lu\n", sync_stats.deadlock_checks);
    console_print("  Deadlocks detected: %lu\n", sync_stats.deadlocks_detected);
    console_print("\n");
}
