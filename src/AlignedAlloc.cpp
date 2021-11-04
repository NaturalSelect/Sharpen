#include <sharpen/AlignedAlloc.hpp>

#include <sharpen/SystemMacro.hpp>

#ifdef SHARPEN_IS_WIN
#include <malloc.h>
#else
#include <stdlib.h>
#endif

void *sharpen::AlignedAlloc(sharpen::Size size,sharpen::Size alignment) noexcept
{
#ifdef SHARPEN_IS_WIN
    return _aligned_malloc(size,alignment);
#else
    void *p = nullptr;
    posix_memalign(&p,alignment,size);
    return p;
#endif
}

void sharpen::AlignedFree(void *memblock) noexcept
{
#ifdef SHARPEN_IS_WIN
    return _aligned_free(memblock);
#else
    return free(memblock);
#endif
}