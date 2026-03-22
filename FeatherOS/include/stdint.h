/* FeatherOS - Standard Library Headers - Fixed */
#ifndef FEATHEROS_STDINT_H
#define FEATHEROS_STDINT_H

/* Fixed-size integer types */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long      uint64_t;

typedef signed char        int8_t;
typedef signed short     int16_t;
typedef signed int       int32_t;
typedef signed long       int64_t;

/* Size types */
typedef unsigned long      size_t;
typedef long               ssize_t;
typedef unsigned long      uintptr_t;
typedef long              intptr_t;

/* Boolean type */
typedef _Bool             bool;
#ifndef true
#define true              1
#endif
#ifndef false
#define false             0
#endif

/* NULL pointer */
#ifndef NULL
#define NULL               ((void*)0)
#endif

/* Limits */
#define SIZE_MAX           UINT64_MAX

#endif /* FEATHEROS_STDINT_H */
