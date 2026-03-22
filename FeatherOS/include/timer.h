/* FeatherOS - Timer & Clock Subsystem
 * Sprint 14: Timer & Clock
 */

#ifndef FEATHEROS_TIMER_H
#define FEATHEROS_TIMER_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * TIME STRUCTURES
 *============================================================================*/

/* Timeval structure (POSIX compatible) */
typedef struct {
    int64_t tv_sec;   /* seconds */
    int64_t tv_usec;  /* microseconds */
} timeval_t;

/* Timespec structure (POSIX compatible) */
typedef struct {
    int64_t tv_sec;   /* seconds */
    int64_t tv_nsec;  /* nanoseconds */
} timespec_t;

/* Timezone structure */
typedef struct {
    int tz_minuteswest;  /* minutes west of Greenwich */
    int tz_dsttime;      /* type of DST correction */
} timezone_t;

/* System uptime info */
typedef struct {
    uint64_t uptime_ticks;      /* ticks since boot */
    uint64_t uptime_ms;         /* milliseconds since boot */
    uint64_t boot_time;         /* timestamp of boot */
} uptime_info_t;

/*============================================================================
 * TIMER FREQUENCIES
 *============================================================================*/

#define PIT_FREQUENCY        1193182UL    /* PIT base frequency */
#define PIT_CHANNEL0_RATE     1000         /* Channel 0 default: ~1000 Hz */
#define TSC_HZ               0           /* Will be detected */

/* HZ - system timer frequency */
#ifndef HZ
#define HZ 100
#endif

/*============================================================================
 * PIT (Programmable Interval Timer)
 *============================================================================*/

/* PIT I/O ports */
#define PIT_PORT_CHANNEL0    0x40
#define PIT_PORT_CHANNEL1    0x41
#define PIT_PORT_CHANNEL2    0x42
#define PIT_PORT_COMMAND     0x43

/* PIT command bits */
#define PIT_CMD_SELECT(x)    ((x) << 6)
#define PIT_CMD_LATCH        0x00
#define PIT_CMD_LOBYTE       0x10
#define PIT_CMD_HIBYTE       0x20
#define PIT_CMD_BOTH         0x30
#define PIT_CMD_MODE(x)      ((x) << 1)
#define PIT_CMD_BINARY       0x00
#define PIT_CMD_BCD          0x01

/* PIT modes */
#define PIT_MODE_TERMINATE   0
#define PIT_MODE_ONESHOT      1
#define PIT_MODE_RATEGEN      2
#define PIT_MODE_SQUARE       3
#define PIT_MODE_SWSTROBE    4
#define PIT_MODE_HWSTROBE    5
#define PIT_MODE_RATEGEN2    6
#define PIT_MODE_SQUARE2     7

/*============================================================================
 * TSC (Time Stamp Counter)
 *============================================================================*/

/* TSC calibration */
typedef struct {
    uint64_t tsc_hz;         /* TSC frequency in Hz */
    uint64_t tsc_per_ms;     /* TSC ticks per millisecond */
    uint64_t tsc_per_us;     /* TSC ticks per microsecond */
    bool initialized;
} tsc_info_t;

/* Read TSC (Time Stamp Counter) */
static inline uint64_t read_tsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/* Read TSC with serializing instructions */
static inline uint64_t read_tsc_serialized(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc; lfence" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/*============================================================================
 * HPET (High Precision Event Timer)
 *============================================================================*/

#define HPET_BASE            0xFED00000
#define HPET_CAPABILITY      0x00
#define HPET_CONFIG          0x10
#define HPET_INT_STATUS      0x20
#define HPET_MAIN_COUNTER    0xF0

#define HPET_TIMER_BASE(n)   (0x100 + (n) * 0x20)

/*============================================================================
 * TIMER WHEEL
 *============================================================================*/

/* Timer flags */
#define TIMER_FLAGS_ONESHOT  0x01
#define TIMER_FLAGS_PERIODIC 0x02
#define TIMER_FLAGS_EXPIRED  0x04

/* Timer structure */
typedef struct timer_list {
    struct timer_list *next;
    uint64_t expires;        /* expiry time in ticks */
    uint64_t period;         /* period for periodic timers */
    void (*function)(void *data);
    void *data;
    uint32_t flags;
} timer_list_t;

/* Timer wheel buckets */
#define TIMER_WHEEL_BITS     8
#define TIMER_WHEEL_SIZE     (1 << TIMER_WHEEL_BITS)
#define TIMER_WHEEL_MASK     (TIMER_WHEEL_SIZE - 1)
#define TIMER_WHEEL_LVL2     (1 << (TIMER_WHEEL_BITS * 2))
#define TIMER_WHEEL_LVL3     (1 << (TIMER_WHEEL_BITS * 3))

/*============================================================================
 * DELAY FUNCTIONS
 *============================================================================*/

/* Busy-wait delays */
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

/* Sleep functions */
void msleep(uint32_t ms);
void usleep(uint32_t us);

/*============================================================================
 * TIME FUNCTIONS
 *============================================================================*/

/* Get current time */
void do_gettimeofday(timeval_t *tv);
void do_settimeofday(const timeval_t *tv);

/* Get uptime */
uint64_t get_uptime_ticks(void);
uint64_t get_uptime_ms(void);

/* Get boot time (wall clock at boot) */
uint64_t get_boot_time(void);

/* TSC functions */
uint64_t get_tsc_hz(void);
uint64_t get_tsc_per_ms(void);
uint64_t get_tsc_per_us(void);

/* Timestamp conversion */
uint64_t tsc_to_ms(uint64_t tsc);
uint64_t tsc_to_us(uint64_t tsc);
uint64_t ms_to_tsc(uint64_t ms);
uint64_t us_to_tsc(uint64_t us);

/*============================================================================
 * PIT FUNCTIONS
 *============================================================================*/

/* Initialize PIT */
void pit_init(void);
void pit_set_frequency(uint32_t hz);
void pit_enable(void);
void pit_disable(void);

/* PIT calibration */
void pit_calibrate(void);

/*============================================================================
 * TIMER WHEEL FUNCTIONS
 *============================================================================*/

/* Initialize timer wheel */
void timer_wheel_init(void);

/* Add/remove timers */
void init_timer(timer_list_t *timer);
void add_timer(timer_list_t *timer);
void add_timer_on(timer_list_t *timer, int cpu);
int del_timer(timer_list_t *timer);

/* Timer tick (called from PIT handler) */
void timer_tick(void);

/* Process expired timers */
void run_timers(void);

/*============================================================================
 * SCHEDULER INTEGRATION
 *============================================================================*/

/* Scheduler tick (called from timer interrupt) */
void sched_tick(void);

/* Get current jiffies */
uint64_t get_jiffies(void);

/* Timer statistics */
typedef struct {
    uint64_t total_ticks;
    uint64_t timers_added;
    uint64_t timers_deleted;
    uint64_t timers_expired;
} timer_stats_t;

timer_stats_t *get_timer_stats(void);
void print_timer_stats(void);

#endif /* FEATHEROS_TIMER_H */
