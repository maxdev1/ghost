#ifndef GHOST_KERNEL_PAGING_SHOOTDOWN_HPP
#define GHOST_KERNEL_PAGING_SHOOTDOWN_HPP

#include <ghost/memory/types.h>

// Called after every boot CPU has installed its IDT, TSS and local APIC.
void pagingInitializeSmp();
// Completes invalidation on every relevant CPU before an old frame can be reused.
void pagingSynchronizePage(g_virtual_address address);
// NMI entry only: no allocation, locks, logging, task/GS state or scheduling.
bool pagingHandleShootdownNmi();

#endif
