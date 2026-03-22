/* FeatherOS - Timer Implementation
 * Sprint 14: Timer & Clock
 */

#include <timer.h>
#include <kernel.h>
#include <sync.h>

/* External assembly functions */
extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);

/*============================================================================
 * SYSTEM TIME
 *============================================================================*/

static volatile uint64_t jiffies = 0;        /* ticks since boot */
static volatile uint64_t boot_time = 0;      /* timestamp when booted */
static uptime_info_t uptime_info = {0};

/* Timer statistics */
static timer_stats_t timer_stats = {0};

/* TSC information */
static tsc_info_t tsc_info = {0};

/* Timer wheel */
static timer_list_t *timer_wheel[TIMER_WHEEL_SIZE];
static timer_list_t *timer_wheel_lvl2[TIMER_WHEEL_SIZE];
static timer_list_t *timer_wheel_lvl3[TIMER_WHEEL_SIZE];
static spinlock_t timer_lock = SPINLOCK_UNLOCKED;

/*============================================================================
 * DELAY FUNCTIONS
 *============================================================================*/

void delay_us(uint32_t us) {
    uint64_t start = read_tsc();
    uint64_t target = start + (us * tsc_info.tsc_per_us);
    
    while (read_tsc() < target) {
        __asm__ volatile("pause");
    }
}

void delay_ms(uint32_t ms) {
    delay_us(ms * 1000);
}

/*============================================================================
 * SLEEP FUNCTIONS
 *============================================================================*/

void msleep(uint32_t ms) {
    /* Simple busy wait for now */
    delay_ms(ms);
}

void usleep(uint32_t us) {
    /* Simple busy wait for now */
    delay_us(us);
}

/*============================================================================
 * TSC FUNCTIONS
 *============================================================================*/

static void calibrate_tsc(void) {
    /* Simple calibration - assume 3GHz TSC if not calibrated */
    uint64_t start_tsc = read_tsc();
    uint64_t start_ms = get_uptime_ms();
    
    /* Wait 100ms */
    delay_ms(100);
    
    uint64_t end_tsc = read_tsc();
    uint64_t end_ms = get_uptime_ms();
    
    if (end_ms > start_ms) {
        uint64_t tsc_diff = end_tsc - start_tsc;
        uint64_t ms_diff = end_ms - start_ms;
        
        tsc_info.tsc_hz = (tsc_diff * 1000) / ms_diff;
        tsc_info.tsc_per_ms = tsc_diff / ms_diff;
        tsc_info.tsc_per_us = tsc_info.tsc_per_ms / 1000;
    } else {
        /* Fallback - assume 3 GHz */
        tsc_info.tsc_hz = 3000000000ULL;
        tsc_info.tsc_per_ms = 3000000ULL;
        tsc_info.tsc_per_us = 3000ULL;
    }
    
    tsc_info.initialized = true;
    
    console_print("TSC calibrated: %lu MHz\n", tsc_info.tsc_hz / 1000000);
}

uint64_t get_tsc_hz(void) {
    if (!tsc_info.initialized) {
        calibrate_tsc();
    }
    return tsc_info.tsc_hz;
}

uint64_t get_tsc_per_ms(void) {
    if (!tsc_info.initialized) {
        calibrate_tsc();
    }
    return tsc_info.tsc_per_ms;
}

uint64_t get_tsc_per_us(void) {
    if (!tsc_info.initialized) {
        calibrate_tsc();
    }
    return tsc_info.tsc_per_us;
}

uint64_t tsc_to_ms(uint64_t tsc) {
    if (!tsc_info.initialized) {
        calibrate_tsc();
    }
    return tsc / tsc_info.tsc_per_ms;
}

uint64_t tsc_to_us(uint64_t tsc) {
    if (!tsc_info.initialized) {
        calibrate_tsc();
    }
    return tsc / tsc_info.tsc_per_us;
}

uint64_t ms_to_tsc(uint64_t ms) {
    if (!tsc_info.initialized) {
        calibrate_tsc();
    }
    return ms * tsc_info.tsc_per_ms;
}

uint64_t us_to_tsc(uint64_t us) {
    if (!tsc_info.initialized) {
        calibrate_tsc();
    }
    return us * tsc_info.tsc_per_us;
}

/*============================================================================
 * PIT FUNCTIONS
 *============================================================================*/

void pit_set_divisor(uint16_t divisor) {
    /* PIT divisor: 0 means 65536 */
    outb(PIT_PORT_COMMAND, 0x36);  /* Channel 0, lobyte/hibyte, mode 3 (square) */
    outb(PIT_PORT_CHANNEL0, divisor & 0xFF);
    outb(PIT_PORT_CHANNEL0, (divisor >> 8) & 0xFF);
}

void pit_set_frequency(uint32_t hz) {
    uint16_t divisor = PIT_FREQUENCY / hz;
    pit_set_divisor(divisor);
}

void pit_enable(void) {
    /* Already enabled via frequency setting */
}

void pit_disable(void) {
    /* Disable by setting divisor to 0 */
    pit_set_divisor(0);
}

void pit_driver_init(void) {
    /* Set initial frequency */
    pit_set_frequency(HZ);
    
    /* Set boot time (simple - just jiffies based) */
    boot_time = 0;  /* Would be set from RTC in real system */
    
    console_print("PIT driver initialized at %d Hz\n", HZ);
}

void pit_calibrate(void) {
    /* Calibrate TSC based on PIT */
    calibrate_tsc();
}

/*============================================================================
 * TIMER WHEEL FUNCTIONS
 *============================================================================*/

void timer_wheel_init(void) {
    for (int i = 0; i < TIMER_WHEEL_SIZE; i++) {
        timer_wheel[i] = NULL;
        timer_wheel_lvl2[i] = NULL;
        timer_wheel_lvl3[i] = NULL;
    }
    
    console_print("Timer wheel initialized\n");
}

void init_timer(timer_list_t *timer) {
    timer->next = NULL;
    timer->expires = 0;
    timer->period = 0;
    timer->function = NULL;
    timer->data = NULL;
    timer->flags = 0;
}

static void add_timer_bucket(timer_list_t *timer) {
    uint64_t expires = timer->expires;
    uint64_t idx = expires & TIMER_WHEEL_MASK;
    
    if (expires < TIMER_WHEEL_SIZE) {
        timer->next = timer_wheel[idx];
        timer_wheel[idx] = timer;
    } else if (expires < TIMER_WHEEL_LVL2) {
        timer->next = timer_wheel_lvl2[idx];
        timer_wheel_lvl2[idx] = timer;
    } else {
        timer->next = timer_wheel_lvl3[idx];
        timer_wheel_lvl3[idx] = timer;
    }
}

void add_timer(timer_list_t *timer) {
    spin_lock(&timer_lock);
    add_timer_bucket(timer);
    timer_stats.timers_added++;
    spin_unlock(&timer_lock);
}

void add_timer_on(timer_list_t *timer, int cpu) {
    (void)cpu;  /* Per-CPU not implemented yet */
    add_timer(timer);
}

int del_timer(timer_list_t *timer) {
    timer_list_t **bucket;
    
    spin_lock(&timer_lock);
    
    uint64_t expires = timer->expires;
    uint64_t idx = expires & TIMER_WHEEL_MASK;
    
    if (expires < TIMER_WHEEL_SIZE) {
        bucket = &timer_wheel[idx];
    } else if (expires < TIMER_WHEEL_LVL2) {
        bucket = &timer_wheel_lvl2[idx];
    } else {
        bucket = &timer_wheel_lvl3[idx];
    }
    
    while (*bucket) {
        if (*bucket == timer) {
            *bucket = timer->next;
            timer->flags |= TIMER_FLAGS_EXPIRED;
            timer_stats.timers_deleted++;
            spin_unlock(&timer_lock);
            return 1;
        }
        bucket = &(*bucket)->next;
    }
    
    spin_unlock(&timer_lock);
    return 0;
}

static void run_timer_list(timer_list_t **head) {
    timer_list_t *timer = *head;
    *head = NULL;
    
    while (timer) {
        timer_list_t *next = timer->next;
        
        /* Execute timer function */
        if (timer->function) {
            timer->function(timer->data);
        }
        timer_stats.timers_expired++;
        
        /* Handle periodic timers */
        if (timer->period > 0 && !(timer->flags & TIMER_FLAGS_EXPIRED)) {
            timer->expires += timer->period;
            add_timer_bucket(timer);
        }
        
        timer = next;
    }
}

void timer_tick(void) {
    spin_lock(&timer_lock);
    
    jiffies++;
    timer_stats.total_ticks++;
    uptime_info.uptime_ticks = jiffies;
    uptime_info.uptime_ms = (jiffies * 1000) / HZ;
    
    uint64_t idx = jiffies & TIMER_WHEEL_MASK;
    
    /* Run expired timers */
    if (timer_wheel[idx]) {
        run_timer_list(&timer_wheel[idx]);
    }
    
    /* Cascade timers from lvl2 */
    if ((jiffies & (TIMER_WHEEL_SIZE - 1)) == 0) {
        uint64_t lvl2_idx = (jiffies >> TIMER_WHEEL_BITS) & TIMER_WHEEL_MASK;
        if (timer_wheel_lvl2[lvl2_idx]) {
            /* Move to lower level */
            timer_list_t *t = timer_wheel_lvl2[lvl2_idx];
            timer_wheel_lvl2[lvl2_idx] = NULL;
            while (t) {
                timer_list_t *next = t->next;
                add_timer_bucket(t);
                t = next;
            }
        }
    }
    
    spin_unlock(&timer_lock);
}

void run_timers(void) {
    /* Called from idle or schedule */
    uint64_t idx = jiffies & TIMER_WHEEL_MASK;
    
    spin_lock(&timer_lock);
    if (timer_wheel[idx]) {
        run_timer_list(&timer_wheel[idx]);
    }
    spin_unlock(&timer_lock);
}

/*============================================================================
 * SCHEDULER INTEGRATION
 *============================================================================*/

/* Timer tick for scheduler - called from timer interrupt */
void timer_tick_hook(void) {
    timer_tick();
}

uint64_t get_jiffies(void) {
    return jiffies;
}

/*============================================================================
 * TIME FUNCTIONS
 *============================================================================*/

void do_gettimeofday(timeval_t *tv) {
    if (!tv) return;
    
    uint64_t now = get_uptime_ms();
    tv->tv_sec = now / 1000;
    tv->tv_usec = (now % 1000) * 1000;
}

void do_settimeofday(const timeval_t *tv) {
    if (!tv) return;
    
    /* Would adjust system time - simplified */
    boot_time = 0;
}

/* Get uptime */
uint64_t get_uptime_ticks(void) {
    return jiffies;
}

uint64_t get_uptime_ms(void) {
    return (jiffies * 1000) / HZ;
}

uint64_t get_boot_time(void) {
    return boot_time;
}

/*============================================================================
 * STATISTICS
 *============================================================================*/

timer_stats_t *get_timer_stats(void) {
    return &timer_stats;
}

void print_timer_stats(void) {
    console_print("\n=== Timer Statistics ===\n");
    console_print("Total ticks: %lu\n", timer_stats.total_ticks);
    console_print("Timers added: %lu\n", timer_stats.timers_added);
    console_print("Timers deleted: %lu\n", timer_stats.timers_deleted);
    console_print("Timers expired: %lu\n", timer_stats.timers_expired);
    console_print("Uptime: %lu ms\n", get_uptime_ms());
    console_print("HZ: %d\n", HZ);
    
    if (tsc_info.initialized) {
        console_print("TSC: %lu MHz\n", tsc_info.tsc_hz / 1000000);
    }
}

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

void timer_subsystem_init(void) {
    /* Initialize timer wheel */
    timer_wheel_init();
    
    /* Initialize PIT */
    pit_driver_init();
    
    /* Calibrate TSC */
    calibrate_tsc();
    
    console_print("Timer subsystem initialized\n");
}
