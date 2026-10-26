// Host views of production source. Hardware substitutions are explicit below.
const auto mutex_hpp = source("kernel/src/kernel/system/mutex.hpp");
const auto constants_hpp = source("kernel/src/kernel/memory/constants.hpp");
const auto memory_cpp = source("kernel/src/kernel/memory/memory.cpp");
const auto memory_callstructs_h = source("libapi/inc/ghost/memory/callstructs.h");
const auto syscall_memory_cpp = source("kernel/src/kernel/calls/syscall_memory.cpp");
const auto allocator_hpp = source("kernel/src/kernel/memory/allocator.hpp");
const auto math_cpp = source("kernel/src/kernel/utils/math.cpp");
const auto allocator_cpp = source("kernel/src/kernel/memory/allocator.cpp");
const auto paging_hpp = source("kernel/src/kernel/memory/paging.hpp");
const auto paging_cpp = source("kernel/src/kernel/memory/paging.cpp");
const auto filesystem_cpp = source("kernel/src/kernel/filesystem/filesystem.cpp");
const auto page_reference_tracker_hpp = source("kernel/src/kernel/memory/page_reference_tracker.hpp");
const auto page_reference_tracker_cpp = source("kernel/src/kernel/memory/page_reference_tracker.cpp");
const auto bitmap_page_allocator_hpp = source("kernel/src/kernel/memory/bitmap_page_allocator.hpp");
const auto bitmap_page_allocator_cpp = source("kernel/src/kernel/memory/bitmap_page_allocator.cpp");
const auto processor_hpp = source("kernel/src/kernel/system/processor/processor.hpp");
const auto processor_cpp = source("kernel/src/kernel/system/processor/processor.cpp");
const auto system_cpp = source("kernel/src/kernel/system/system.cpp");
const auto hashmap_hpp = source("kernel/src/kernel/utils/hashmap.hpp");
const auto hashmap_string_hpp = source("kernel/src/kernel/utils/hashmap_string.hpp");
const auto hashmap_string_cpp = source("kernel/src/kernel/utils/hashmap_string.cpp");
const auto elf_object_hpp = source("kernel/src/kernel/tasking/elf/elf_object.hpp");
const auto elf_object_cpp = source("kernel/src/kernel/tasking/elf/elf_object.cpp");
const auto tasking_memory_cpp = source("kernel/src/kernel/tasking/tasking_memory.cpp");
const auto spinlock_hpp = source("kernel/src/kernel/system/spinlock.hpp");
const auto wait_queue_hpp = source("kernel/src/kernel/utils/wait_queue.hpp");
const auto wait_queue_cpp = source("kernel/src/kernel/utils/wait_queue.cpp");
const auto tasking_hpp = source("kernel/src/kernel/tasking/tasking.hpp");
const auto tasking_cpp = source("kernel/src/kernel/tasking/tasking.cpp");
const auto filesystem_process_cpp = source("kernel/src/kernel/filesystem/filesystem_process.cpp");
const auto task_hpp = source("kernel/src/kernel/tasking/task.hpp");
const auto user_mutex_hpp = source("kernel/src/kernel/tasking/user_mutex.hpp");
const auto user_mutex_cpp = source("kernel/src/kernel/tasking/user_mutex.cpp");
const auto scheduler_round_robin_cpp = source("kernel/src/kernel/tasking/scheduler/scheduler_round_robin.cpp");
const auto syscall_cpp = source("kernel/src/kernel/calls/syscall.cpp");
const auto types_h = source("libapi/inc/ghost/messages/types.h");
const auto message_queues_hpp = source("kernel/src/kernel/ipc/message_queues.hpp");
const auto message_queues_cpp = source("kernel/src/kernel/ipc/message_queues.cpp");
const auto tasking_directory_cpp = source("kernel/src/kernel/tasking/tasking_directory.cpp");
const auto lower_heap_cpp = source("kernel/src/kernel/memory/lower_heap.cpp");
const auto heap_cpp = source("kernel/src/kernel/memory/heap.cpp");
const auto message_topics_cpp = source("kernel/src/kernel/ipc/message_topics.cpp");
const auto pipes_cpp = source("kernel/src/kernel/ipc/pipes.cpp");
const auto terminal_cpp = source("applications/terminal/src/terminal.cpp");
const auto tasks_callstructs_h = source("libapi/inc/ghost/tasks/callstructs.h");
const auto syscall_tasking_cpp = source("kernel/src/kernel/calls/syscall_tasking.cpp");
const auto exceptions_cpp = source("kernel/src/kernel/system/interrupts/exceptions.cpp");
const auto elf64_h = source("libapi/inc/ghost/elf64.h");
const auto tasks_types_h = source("libapi/inc/ghost/tasks/types.h");
const auto elf_loader_cpp = source("kernel/src/kernel/tasking/elf/elf_loader.cpp");
const auto elf_tls_cpp = source("kernel/src/kernel/tasking/elf/elf_tls.cpp");
const auto filesystem_types_h = source("libapi/inc/ghost/filesystem/types.h");
const auto pipes_hpp = source("kernel/src/kernel/ipc/pipes.hpp");
const auto filesystem_process_hpp = source("kernel/src/kernel/filesystem/filesystem_process.hpp");
const auto paging_shootdown_cpp = source("kernel/src/kernel/memory/paging_shootdown.cpp");
const auto address_range_pool_hpp = source("kernel/src/kernel/memory/address_range_pool.hpp");
const auto address_range_pool_cpp = source("kernel/src/kernel/memory/address_range_pool.cpp");

emit("mutex_layout.inc", (before(mutex_hpp, "/**")) + std::string("\n#endif\n"));
emit("direct_map_offset.inc", (defines(constants_hpp, "G_MEM_HIGHER_HALF_DIRECT_MAP_OFFSET")));
emit("physical_to_virtual.inc", (defines(constants_hpp, "G_MEM_PHYS_TO_VIRT")));
emit("physical_range.inc", (between(memory_cpp, "bool memoryPhysicalIsRangeAddressable(", "g_physical_address memoryPhysicalAllocate(")));
emit("map_mmio_callstruct.inc", callstruct(memory_callstructs_h, "g_syscall_map_mmio", true));
emit("syscallMapMmioArea.inc", (fromSignature(syscall_memory_cpp, "void syscallMapMmioArea(")));
emit("allocator_hpp.inc", allocator_hpp);
emit("math_cpp.inc", math_cpp);
emit("allocator_cpp.inc", allocator_cpp);
emit("paging_host_hpp.inc", (replace(paging_hpp, "__asm__ __volatile__(\"invlpg (%0)\" : : \"r\"(addr) : \"memory\");", "(void)addr;")));
emit("paging_host_cpp.inc", (replaceFunction((replaceFunction(paging_cpp, "void pagingSwitchToSpace(", "void pagingSwitchToSpace(g_physical_address dir) { currentSpace = dir; }")), "g_physical_address pagingGetCurrentSpace(", "g_physical_address pagingGetCurrentSpace() { return currentSpace; }")));
emit("memoryOnDemandHandlePageFault.inc", (between(memory_cpp, "bool memoryOnDemandHandlePageFault(", "void* memorySetBytes(")));
emit("filesystemReadToMemory.inc", (between(filesystem_cpp, "bool filesystemReadToMemory(", "bool filesystemGetFileName(")));
emit("page_reference_tracker_hpp.inc", page_reference_tracker_hpp);
emit("page_reference_tracker_cpp.inc", page_reference_tracker_cpp);
emit("bitmap_page_allocator_hpp.inc", bitmap_page_allocator_hpp);
emit("bitmap_page_allocator_cpp.inc", bitmap_page_allocator_cpp);
emit("processor_layout.inc", before(slice(processor_hpp, find(processor_hpp, "struct g_processor")), "/**"));
emit("processor_lookup_and_add.inc", (between(processor_cpp, "g_processor* _processorGetCurrent()", "void processorInitializeBsp()")) + (between(processor_cpp, "void processorAdd(", "uint16_t processorGetNumberOfProcessors()")));
emit("system_host_cpp.inc", (replace((replace(system_cpp, "asm volatile(\"pause\");", "std::this_thread::yield();")), "asm(\"pause\");", "std::this_thread::yield();")));
emit("hashmap_hpp.inc", hashmap_hpp);
emit("hashmap_string_hpp.inc", hashmap_string_hpp);
emit("hashmap_string_cpp.inc", hashmap_string_cpp);
emit("elf_object_layout.inc", (between(elf_object_hpp, "struct g_elf_object;", "struct g_elf_object_load_result")));
emit("elfObjectDestroy.inc", (function(elf_object_cpp, "void elfObjectDestroy(")));
emit("tasking_page_space_teardown.inc", (between(tasking_memory_cpp, "static void _taskingMemoryDestroyTable(", "bool taskingMemoryInitializeTls(")));
emit("spinlock_host_hpp.inc", (replace(spinlock_hpp, "asm volatile(\"pause\");", "std::this_thread::yield();")));
emit("wait_queue_hpp.inc", wait_queue_hpp);
emit("wait_queue_cpp.inc", wait_queue_cpp);
emit("task_reference_hpp.inc", (between(tasking_hpp, "g_task* taskingAcquireTask(", "/** Retires a dead task")));
emit("task_reference_cpp.inc", (between(tasking_cpp, "g_task* taskingAcquireTask(", "void taskingRequestTermination(")));
emit("descriptor_map_teardown.inc", (function(filesystem_process_cpp, "void filesystemProcessRemove("))
    + (function(filesystem_process_cpp, "void filesystemProcessRemoveDescriptor(")));
emit("process_layout.inc", slice(task_hpp, find(task_hpp, "struct g_process\n{"), rfind(task_hpp, "#endif")));
emit("taskingDestroyProcess.inc", (function(tasking_cpp, "void taskingDestroyProcess(")));
emit("taskingProcessRemoveFromTaskList.inc", (function(tasking_cpp, "void taskingProcessRemoveFromTaskList(")));
emit("taskingDestroyTask.inc", (function(tasking_cpp, "void taskingDestroyTask(")));
emit("user_mutex_layout.inc", (between(user_mutex_hpp, "struct g_user_mutex_entry", "/**")));
emit("user_mutex_cpp.inc", user_mutex_cpp);
emit("syscall_share_and_unmap.inc", (between(syscall_memory_cpp, "void syscallUnmap(", "void syscallMapMmioArea(")));
emit("schedulerSchedule.inc", (function(scheduler_round_robin_cpp, "void schedulerSchedule(")));
emit("taskingCancelPendingSpawns.inc", (function(tasking_cpp, "static void taskingCancelPendingSpawns(")));
emit("taskingCompleteSpawn.inc", (function(tasking_cpp, "static void taskingCompleteSpawn(")));
emit("elfObjectValidateLoadRange.inc", (function(elf_object_cpp, "static bool _elfObjectValidateLoadRange(")));
emit("elfObjectLoadLoadSegment.inc", (function(elf_object_cpp, "g_spawn_status elfObjectLoadLoadSegment(")));
emit("syscall.inc", (function(syscall_cpp, "void syscall(")));
emit("message_types.inc", (between(types_h, "typedef int32_t g_message_transaction;", "__END_C")));
emit("message_queue_layout.inc", (between(message_queues_hpp, "struct g_message_queue\n", "/**\n * Initializes")));
emit("message_queues_cpp.inc", message_queues_cpp);
emit("memorySizeToPageCount.inc", (function(memory_cpp, "bool memorySizeToPageCount(")));
emit("syscallAllocateMemory.inc", (function(syscall_memory_cpp, "void syscallAllocateMemory(")));
emit("taskingDirectoryCopyIdentifier.inc", (function(tasking_directory_cpp, "void taskingDirectoryCopyIdentifier(")));
emit("lower_heap_cpp.inc", lower_heap_cpp);
emit("heap_host_cpp.inc", std::string("\ng_allocator heapAllocator;\ng_mutex heapLock;\nbool heapInitialized = true;\ng_size heapAmountInUse;\nbool _heapExpand() { return false; }\n")
    + (function(heap_cpp, "void* heapAllocate(")) + std::string("\n")
    + (function(heap_cpp, "void heapFree(")) + std::string("\n")
    + (function(heap_cpp, "g_size heapGetUsedAmount(")));
emit("messageTopicsPost.inc", (function(message_topics_cpp, "g_message_send_status messageTopicsPost(")));
emit("pipeGetLength.inc", (function(pipes_cpp, "g_fs_length_status pipeGetLength(")));
const auto terminalStart = find(terminal_cpp, "{", find(terminal_cpp, "int main(")) + 1;
emit("terminal_exit_initialization.inc", slice(terminal_cpp, terminalStart, find(terminal_cpp, "terminalInitializeScreen(", terminalStart)));
emit("terminalTerminationRoutine.inc", (function(terminal_cpp, "void terminalTerminationRoutine(")));
emit("taskingMemoryExtendHeap.inc", (function(tasking_memory_cpp, "bool taskingMemoryExtendHeap(")));
emit("tasking_stack.inc", (function(tasking_memory_cpp, "g_stack taskingMemoryCreateStack("))
    + (function(tasking_memory_cpp, "bool taskingMemoryHandleStackOverflow(")));
emit("tasking_fpu.inc", (function(tasking_memory_cpp, "void taskingMemoryInitializeUtility("))
    + (function(tasking_memory_cpp, "void taskingMemoryDestroyUtility(")));
emit("taskingRestoreState.inc", (function(tasking_cpp, "void taskingRestoreState(")));
emit("taskingPrepareWait.inc", (function(tasking_cpp, "void taskingPrepareWait(")));
emit("taskingWake.inc", (function(tasking_cpp, "void taskingWake(")));
emit("taskingWaitForExit.inc", (function(tasking_cpp, "bool taskingWaitForExit(")));
emit("create_task_callstruct.inc", callstruct(tasks_callstructs_h, "g_syscall_create_task", false));
emit("spawn_callstruct.inc", callstruct(tasks_callstructs_h, "g_syscall_spawn", false));
emit("syscallCreateTask_and_spawn.inc", (function(syscall_tasking_cpp, "void syscallCreateTask("))
    + (function(syscall_tasking_cpp, "void syscallSpawn(")));
emit("taskingCreateTask.inc", (function(tasking_cpp, "g_task* taskingCreateTask(")));
emit("tasking_tls.inc", (function(tasking_memory_cpp, "bool taskingMemoryInitializeTls("))
    + (function(tasking_memory_cpp, "static void _taskingMemoryReleasePage("))
    + (function(tasking_memory_cpp, "void taskingMemoryDestroyTls(")));
emit("exceptionsHandlePageFault.inc", (function(exceptions_cpp, "bool exceptionsHandlePageFault(")));
emit("elf64_types.inc", (replace((replace(elf64_h, "__BEGIN_C", "")), "__END_C", "")));
emit("spawn_validation.inc", defines(tasks_types_h, "G_SPAWN_VALIDATION"));
emit("elfValidateHeader.inc", (function(elf_loader_cpp, "g_spawn_validation_details elfValidateHeader(")));
emit("elf_load_segment.inc", (function(elf_object_cpp, "static bool _elfObjectValidateLoadRange("))
    + (function(elf_object_cpp, "g_spawn_status elfObjectLoadLoadSegment(")));
emit("elfTlsCreateMasterImage.inc", (function(elf_tls_cpp, "bool elfTlsCreateMasterImage(")));
emit("elfUserProcessCreateInfo.inc", (function(elf_loader_cpp, "g_virtual_address elfUserProcessCreateInfo(")));
emit("elfObjectSymbolHash.inc", (function(elf_object_cpp, "static uint32_t _elfObjectSymbolHash(")));
emit("elfObjectFindSymbol.inc", (function(elf_object_cpp, "static Elf64_Sym* _elfObjectFindSymbol(")));
emit("elfObjectApplyRelocations.inc", (function(elf_object_cpp, "void elfObjectApplyRelocations(")));
emit("pipe_atomic_capacity.inc", (defines(filesystem_types_h, "G_PIPE_DEFAULT_CAPACITY")));
emit("pipe_stream_capacity.inc", (defines(pipes_hpp, "G_PIPE_STREAM_CAPACITY")));
emit("pipe_layout.inc", between(pipes_hpp, "struct g_pipeline", "/**"));
emit("pipeCanWrite.inc", (function(pipes_cpp, "static bool pipeCanWrite(")));
emit("pipeCreate.inc", (function(pipes_cpp, "g_fs_pipe_status pipeCreate(")));
emit("pipeRead.inc", (function(pipes_cpp, "g_fs_read_status pipeRead(")));
emit("pipeWrite.inc", (function(pipes_cpp, "g_fs_write_status pipeWrite(")));
emit("pipeWaitForRead.inc", (function(pipes_cpp, "bool pipeWaitForRead(")));
emit("pipeWaitForWrite.inc", (function(pipes_cpp, "bool pipeWaitForWrite(")));
emit("filesystem_process_hpp.inc", filesystem_process_hpp);
emit("filesystem_process_cpp.inc", (before(filesystem_process_cpp, "g_fs_clonefd_status filesystemProcessCloneDescriptor(")));
emit("filesystem_task_io.inc", between(mutex_hpp, "class g_mutex_guard", "#endif")
    + (function(filesystem_cpp, "static g_fs_write_status filesystemWriteDescriptor("))
    + (function(filesystem_cpp, "g_fs_read_status filesystemRead(g_task*"))
    + (function(filesystem_cpp, "g_fs_write_status filesystemWrite(g_task*")));
emit("spinlock_yield_hpp.inc", (replace(spinlock_hpp, "asm volatile(\"pause\")", "std::this_thread::yield()")));
emit("paging_shootdown_host_cpp.inc", (replace(paging_shootdown_cpp, "asm volatile(\"pause\")", "std::this_thread::yield()")));
emit("syscall_working_directory.inc", (function(syscall_tasking_cpp, "void syscallGetWorkingDirectory("))
    + (function(syscall_tasking_cpp, "void syscallSetWorkingDirectory(")));
emit("elfTlsLoadData.inc", (function(elf_tls_cpp, "g_spawn_status elfTlsLoadData(")));
emit("exception_stack_trace.inc", (function(exceptions_cpp, "static bool exceptionsReadUser("))
    + (function(exceptions_cpp, "void exceptionsPrintStackTrace(")));
emit("address_range_pool_hpp.inc", address_range_pool_hpp);
emit("address_range_free.inc", (function(address_range_pool_cpp, "bool addressRangePoolClaimForFree("))
    + (function(address_range_pool_cpp, "int32_t addressRangePoolFree(")));

// Alignment checks use actual task/API layouts, including trailing packing attributes.
emit("task_api_types.inc", replace(replace(tasks_types_h, "__BEGIN_C", ""), "__END_C", ""));
emit("task_layout.inc", between(task_hpp, "struct g_stack\n{", "/**\n * Task entry used"));
