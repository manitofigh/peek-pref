// [     4KiB     ]
// ^   ^   ^   *
// [           2MiB            ]
// ^ ^ ^ ^ ^ ^ ^ ^] *
// next page prefetecher? will that end up pulling in lines from the next page?
// how many lines does the prefetecher fetch? how can we even test that ? 
// ^^ not to only access the next possible line, but like e.g. 4 lines in advance
// ^  ^  ^  ^           *|         |      \

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/cdefs.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>

// custom types
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#define PAGE_SIZE     1<<12 // base page, 4KiB
#define HP_PAGE_SIZE  1<<21 // huge page, 2MiB

#define ALWAYS_INLINE inline __always_inline
// https://github.com/google/highwayhash/blob/master/highwayhash/tsc_timer.h
#define timer_start timer_start_google
#define timer_stop timer_stop_google
#define maccess(P)                                                            \
    do {                                                                       \
        typeof(*(P)) _NO_USE;                                                  \
        __asm__ __volatile__("mov (%1), %0\n"                                  \
                             : "=r"(_NO_USE)                                   \
                             : "r"((P))                                        \
                             : "memory");                                      \
    } while (0)

static ALWAYS_INLINE u64 timer_start_google(void)
{
    u64 t;
    __asm__ __volatile__("mfence\n\t"
                         "lfence\n\t"
                         "rdtsc\n\t"
                         "shl $32, %%rdx\n\t"
                         "or %%rdx, %0\n\t"
                         "lfence"
                         : "=a"(t)
                         :
                         // "memory" avoids reordering. rdx = TSC >> 32.
                         // "cc" = flags modified by SHL.
                         : "rdx", "memory", "cc");
    return t;
}

static ALWAYS_INLINE u64 timer_stop_google(void)
{
    u64 t;
    __asm__ __volatile__("rdtscp\n\t"
                         "shl $32, %%rdx\n\t"
                         "or %%rdx, %0\n\t"
                         "lfence"
                         : "=a"(t)
                         :
                         // "memory" avoids reordering.
                         // rcx = TSC_AUX. rdx = TSC >> 32.
                         // "cc" = flags modified by SHL.
                         : "rcx", "rdx", "memory", "cc");
    return t;
}

static ALWAYS_INLINE u64 timer_warmup(void)
{
    return timer_start_google();
}

static ALWAYS_INLINE void _clflushopt(const void *p)
{
    __asm__ __volatile__("clflushopt 0(%0)" : : "r"(p) : "memory");
}

// you would need to calculate how many cache lines are to be evicted (assuming 64B per cl)
static inline void clflushopt_n_lines(const void *p, const u32 n_lines)
{
    for (u32 i = 0; i < n_lines; i++)
        _clflushopt(p + i * 64);
}

static inline void clflushopt_4k_page(const void *p)
{ 
    for (u32 i = 0; i < 64; i++) // 4KiB / 64 bytes
        _clflushopt(p + i * 64);
}

// 2MiB huge page
static inline void clflushopt_2m_page(const void *p)
{
    for (u32 i = 0; i < 32768 ; i++) // 2MiB / 64 bytes
        _clflushopt(p + i * 64);
}

static ALWAYS_INLINE void _lfence(void)
{
    __asm__ __volatile__("lfence" ::: "memory");
}

static ALWAYS_INLINE void _mfence(void)
{
    __asm__ __volatile__("mfence" ::: "memory");
}

static ALWAYS_INLINE u8 *mmap_private(void *addr, size_t size) {
    u8 *ptr = mmap(addr, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
        return NULL;
    return ptr;
}

static ALWAYS_INLINE u8 *mmap_private_init(void *addr, size_t size, u8 init) {
    u8 *ptr = mmap_private(addr, size);
    if (ptr)
        memset(ptr, init, size);
    return ptr;
}

static ALWAYS_INLINE u8 *mmap_huge_private(void *addr, size_t size)
{
    u8 *ptr = mmap((void *)addr, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (ptr == MAP_FAILED)
        return NULL;
    return ptr;
}

static ALWAYS_INLINE u8 *mmap_huge_private_init(void *addr, size_t size, u8 i)
{
    u8 *ptr = mmap_huge_private(addr, size);
    if (ptr)
        memset(ptr, i, size);
    return ptr;
}

i32 main()
{
    //printf("Hello, world!\n");
    //u8* bp = mmap_private_init(NULL, PAGE_SIZE*2, 'A');
    u8* hp = mmap_huge_private_init(NULL, HP_PAGE_SIZE, 'A');
    //printf("address of bp: %p\n", bp);
    //clflushopt_4k_page(bp);
    //clflushopt_4k_page(bp+4096);
    clflushopt_2m_page(hp);

    __asm__ __volatile__("" ::: "memory"); // barrier
    _mfence();
    for (u16 i = 0; i < 8; i++) {
        //maccess(bp + 512 * i);
        maccess(hp + 512 * i);
        //printf("address: %p\n", bp + 512 * i);
    }
    _mfence(); 
    //printf("hoping to have %p prefeteched!\n", bp + 512 * 7);
    u64 volatile w = timer_warmup();
    u64 t1 = timer_start();
    //maccess(bp+512*8);
    maccess(hp+512*8);
    u64 t2 = timer_stop();
    printf("%lu\n", t2-t1);
}

// behaviors observations
//
