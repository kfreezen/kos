#include <paging.h>
#include <print.h>

inline Pointer getPhysAddr(PageDirectory* dir, Pointer virt) { kprintf("warning:stub getPhysAddr()\n"); return NULL; }
inline int MapAllocatedPageTo(PageDirectory* dir, void* virtAddr) { return 0; }
inline int MapAllocatedPageBlockTo(PageDirectory* dir, void* virtAddr) { return 0; }
