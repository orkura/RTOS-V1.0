/*
 * Route the standard C allocation API to RT-Thread's system heap.
 *
 * The adapter supplies max_align_t alignment even when RT_ALIGN_SIZE is
 * smaller.  Each returned pointer has a private header immediately before it;
 * therefore pointers allocated here must be released through free(), not
 * rt_free().  Conversely, pointers returned by rt_malloc() must still be
 * released through rt_free().
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <rtthread.h>

#ifndef RT_USING_HEAP
#error "The libc heap adapter requires RT_USING_HEAP"
#endif

typedef struct
{
    void   *raw_pointer;
    size_t  requested_size;
} libc_heap_header_t;

#define LIBC_HEAP_ALIGNMENT ((size_t)_Alignof(max_align_t))

_Static_assert(sizeof(rt_size_t) >= sizeof(size_t),
               "rt_size_t cannot represent every libc allocation size");

static void *libc_heap_allocate(size_t size)
{
    const size_t overhead = sizeof(libc_heap_header_t) +
                            LIBC_HEAP_ALIGNMENT - 1U;
    libc_heap_header_t *header;
    uintptr_t candidate;
    uintptr_t aligned;
    size_t remainder;
    void *raw_pointer;

    if (size > SIZE_MAX - overhead)
    {
        return NULL;
    }

    raw_pointer = rt_malloc((rt_size_t)(size + overhead));
    if (raw_pointer == RT_NULL)
    {
        return NULL;
    }

    candidate = (uintptr_t)raw_pointer + sizeof(libc_heap_header_t);
    remainder = (size_t)(candidate % LIBC_HEAP_ALIGNMENT);
    aligned = remainder == 0U
            ? candidate
            : candidate + LIBC_HEAP_ALIGNMENT - remainder;

    header = (libc_heap_header_t *)(aligned - sizeof(libc_heap_header_t));
    header->raw_pointer = raw_pointer;
    header->requested_size = size;

    return (void *)aligned;
}

static void libc_heap_release(void *pointer)
{
    libc_heap_header_t *header;

    if (pointer == NULL)
    {
        return;
    }

    header = (libc_heap_header_t *)((uintptr_t)pointer -
                                    sizeof(libc_heap_header_t));
    rt_free(header->raw_pointer);
}

void *malloc(size_t size)
{
    return libc_heap_allocate(size);
}

void free(void *pointer)
{
    libc_heap_release(pointer);
}

void *calloc(size_t count, size_t size)
{
    size_t total_size;
    void *pointer;

    if ((size != 0U) && (count > SIZE_MAX / size))
    {
        return NULL;
    }

    total_size = count * size;
    pointer = libc_heap_allocate(total_size);
    if (pointer != NULL)
    {
        rt_memset(pointer, 0, (rt_ubase_t)total_size);
    }

    return pointer;
}

void *realloc(void *pointer, size_t size)
{
    libc_heap_header_t *header;
    size_t copy_size;
    void *new_pointer;

    if (pointer == NULL)
    {
        return libc_heap_allocate(size);
    }

    if (size == 0U)
    {
        libc_heap_release(pointer);
        return NULL;
    }

    header = (libc_heap_header_t *)((uintptr_t)pointer -
                                    sizeof(libc_heap_header_t));
    if (size <= header->requested_size)
    {
        header->requested_size = size;
        return pointer;
    }

    new_pointer = libc_heap_allocate(size);
    if (new_pointer == NULL)
    {
        return NULL;
    }

    copy_size = header->requested_size;
    rt_memcpy(new_pointer, pointer, (rt_ubase_t)copy_size);
    libc_heap_release(pointer);

    return new_pointer;
}
