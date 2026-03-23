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
typedef int32_t i32;

#define UNUSED __attribute__((unused))
#define PAGE_SIZE     (1<<12) // base page, 4KiB
#define HP_PAGE_SIZE  (1<<21) // huge page, 2MiB
#define STRIDE        128     // bytes
#define N_TRAIN       5       // rounds of training the prefetcher

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

static u64 time_maccess(u8* p)
{
    u64 volatile UNUSED w = timer_warmup();
    u64 t1 = timer_start();
    maccess(p);
    u64 t2 = timer_stop();
    return t2-t1;
}

i32 main()
{
    //u8* volatile k_pages = mmap_private_init(NULL, PAGE_SIZE*2000, 'A'); // tlb eviction set
    //u8* bp = mmap_private_init(NULL, PAGE_SIZE*2, 'A');
    //clflushopt_4k_page(bp+4096); // second page cache ev
    _mfence();
    // ^ evict the prev pages TLB?
    u8* hp = mmap_huge_private_init(NULL, HP_PAGE_SIZE, 'A');
    if (!hp) {
        printf("ERROR: No HP\n");
        return -1;
    }

    //printf("address of bp: %p\n", bp);
    //clflushopt_4k_page(bp); // bring
    //clflushopt_4k_page(bp+4096);
    clflushopt_2m_page(hp);
    _mfence();

    for (u16 i = 0; i < N_TRAIN; i++) // train the stride prefetecher
        maccess(hp + STRIDE * i);

    _mfence();

    u64 t = time_maccess(hp+STRIDE*(N_TRAIN+1));
    printf("cycles: %lu\n", t);
}
