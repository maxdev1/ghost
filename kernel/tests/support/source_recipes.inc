const auto memory_user_cpp = source("kernel/src/kernel/memory/memory_user.cpp");
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
emit("pit_perform_sleep.inc", function(source("kernel/src/kernel/system/timing/pit.cpp"), "void pitPerformSleep("));
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
const auto tasks_callstructs_h = source("libapi/inc/ghost/tasks/callstructs.h");
const auto syscall_tasking_cpp = source("kernel/src/kernel/calls/syscall_tasking.cpp");
const auto clock_cpp = source("kernel/src/kernel/tasking/clock.cpp");
emit("time_syscalls.inc",
    between(clock_cpp, "static bool hypervisorClock;", "static bool clockReferenceAvailable(")
    + function(clock_cpp, "static bool clockReferenceAvailable(")
    + function(clock_cpp, "static uint64_t clockReadHypervisorCounter(")
    + function(clock_cpp, "static uint64_t clockReadVmwareNanos(")
    + function(clock_cpp, "bool clockReadCalibrationNanos(")
    + function(clock_cpp, "static uint64_t clockReadReferenceNanos(")
    + function(clock_cpp, "uint64_t clockGetMilliseconds(")
    + function(clock_cpp, "uint64_t clockGetNanoseconds(")
    + function(clock_cpp, "int64_t clockGetWallTime(")
    + function(syscall_tasking_cpp, "void syscallGetTime(")
    + function(syscall_tasking_cpp, "void syscallGetMilliseconds(")
    + function(syscall_tasking_cpp, "void syscallGetNanoseconds("));
emit("time_callstructs.inc", between(source("libapi/inc/ghost/tasks/callstructs.h"),
    "/**\n * @field millis local", "/**\n * @field forkedId"));
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

emit("memory_atomic.inc", between(source("libapi/inc/ghost/memory.h"), "#define G_MEMORY_ORDER_RELAXED", "__BEGIN_C"));
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
emit("processor_clock_rates.inc", between(processor_cpp, "bool processorHasHypervFeature(", "void processorGetVendor("));
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
emit("schedulerAccounting.inc", between(scheduler_round_robin_cpp, "static void schedulerAccountTime(", "void schedulerPrefer("));
emit("syscallKernQuery.inc", function(source("kernel/src/kernel/calls/syscall_kernquery.cpp"), "void syscallKernQuery("));
emit("schedulerGetNextTask.inc", function(scheduler_round_robin_cpp, "g_schedule_entry* schedulerGetNextTask("));
emit("tasking_local_layout.inc", between(tasking_hpp, "struct g_schedule_entry", "struct g_spawn_result"));
emit("taskingCleanupTasks.inc", function(source("kernel/src/kernel/tasking/cleanup.cpp"), "static void taskingCleanupTasks("));
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
emit("taskingDirectoryRegister.inc", function(tasking_directory_cpp, "bool taskingDirectoryRegister("));
emit("lower_heap_cpp.inc", lower_heap_cpp);
emit("heap_host_cpp.inc", std::string("\ng_allocator heapAllocator;\ng_mutex heapLock;\nbool heapInitialized = true;\ng_size heapAmountInUse;\nbool _heapExpand() { return false; }\n")
    + (function(heap_cpp, "void* heapAllocate(")) + std::string("\n")
    + (function(heap_cpp, "void heapFree(")) + std::string("\n")
    + (function(heap_cpp, "g_size heapGetUsedAmount(")));
emit("messageTopicsPost.inc", (function(message_topics_cpp, "g_message_send_status messageTopicsPost(")));
emit("pipeGetLength.inc", (function(pipes_cpp, "g_fs_length_status pipeGetLength(")));
emit("taskingMemoryExtendHeap.inc", (function(tasking_memory_cpp, "bool taskingMemoryExtendHeap(")));
emit("memoryReleasePages.inc", function(memory_cpp, "void memoryReleasePages("));
emit("tasking_stack.inc", (function(tasking_memory_cpp, "g_stack taskingMemoryCreateStack("))
    + (function(tasking_memory_cpp, "void taskingMemoryDestroyStack("))
    + (function(tasking_memory_cpp, "bool taskingMemoryHandleStackOverflow(")));
emit("tasking_fpu.inc", (function(tasking_memory_cpp, "void taskingMemoryInitializeUtility("))
    + (function(tasking_memory_cpp, "void taskingMemoryDestroyUtility(")));
emit("taskingRestoreState.inc", (function(tasking_cpp, "void taskingRestoreState(")));
emit("taskingPrepareWait.inc", (function(tasking_cpp, "void taskingPrepareWait(")));
emit("taskingWake.inc", (function(tasking_cpp, "void taskingWake(")));
emit("taskingWaitForExit.inc", (function(tasking_cpp, "bool taskingWaitForExit(")));
emit("create_task_callstruct.inc", callstruct(tasks_callstructs_h, "g_syscall_create_task", false));
emit("launch_mode_types.inc", between(tasks_types_h, "typedef uint8_t g_launch_mode;", "typedef uint8_t g_spawn_status;"));
emit("spawn_callstruct.inc", callstruct(tasks_callstructs_h, "g_syscall_spawn", false)
    + callstruct(tasks_callstructs_h, "g_syscall_get_launch_mode", false));
emit("syscallCreateTask_and_spawn.inc", (function(syscall_tasking_cpp, "void syscallCreateTask("))
    + function(syscall_tasking_cpp, "void syscallSpawn(")
    + function(syscall_tasking_cpp, "void syscallGetLaunchMode("));
emit("taskingCreateTask.inc", (function(tasking_cpp, "g_task* taskingCreateTask(")));
emit("tasking_tls.inc", (function(tasking_memory_cpp, "bool taskingMemoryInitializeTls("))
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
    + (function(filesystem_cpp, "static g_fs_read_status filesystemReadDescriptor("))
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
emit("task_layout.inc", "struct g_user_memory;\n" + between(task_hpp, "struct g_stack\n{", "/**\n * Task entry used"));

const auto delegate_hpp = source("kernel/src/kernel/filesystem/filesystem_taskdelegate.hpp");
const auto filesystem_hpp = source("kernel/src/kernel/filesystem/filesystem.hpp");
emit("filesystem_delegate_layout.inc", between(filesystem_hpp, "struct g_fs_node;", "struct g_filesystem_find_result"));
emit("filesystem_delegate_mutex.inc", between(mutex_hpp, "class g_mutex_guard", "#endif"));
emit("filesystem_delegate_hpp.inc", delegate_hpp);
emit("filesystem_delegate_cpp.inc", source("kernel/src/kernel/filesystem/filesystem_taskdelegate.cpp"));
emit("filesystem_delegate_memory.inc", source("kernel/src/kernel/filesystem/filesystem_taskdelegate_memory.cpp"));
emit("filesystem_delegate_syscalls.inc", source("kernel/src/kernel/calls/syscall_filesystem_delegate.cpp"));
emit("filesystem_delegate_nodes.inc", between(filesystem_cpp, "g_fs_node* filesystemGetMountFolder(", "g_fs_node* filesystemCreateNode("));
emit("filesystem_adopt_root.inc", function(filesystem_cpp, "void filesystemAddChild(")
    + function(filesystem_cpp, "void filesystemAdoptRoot("));
emit("filesystem_absolute_path.inc", function(filesystem_cpp, "int filesystemGetAbsolutePathLength(")
    + function(filesystem_cpp, "int filesystemGetAbsolutePath("));
emit("filesystem_find_existing_child.inc", function(filesystem_cpp, "bool filesystemFindExistingChild("));
const auto boot_ramdisk_cpp = source("kernel/src/kernel/filesystem/ramdisk.cpp");
emit("boot_modules.inc", function(boot_ramdisk_cpp, "g_ramdisk_entry* ramdiskCreateFile(")
    + function(boot_ramdisk_cpp, "g_ramdisk_entry* ramdiskFindChild(")
    + function(boot_ramdisk_cpp, "void ramdiskLoadBootModules("));

const auto syscall_filesystem_cpp = source("kernel/src/kernel/calls/syscall_filesystem.cpp");
emit("filesystem_delegate_io_syscalls.inc", function(syscall_filesystem_cpp, "void syscallFsRead(")
    + function(syscall_filesystem_cpp, "void syscallFsWrite("));

emit("memory_user_copy.inc", function(memory_user_cpp, "static bool memoryCopyUser(")
    + function(memory_user_cpp, "bool memoryCopyFromUser(")
    + function(memory_user_cpp, "bool memoryCopyToUser("));

emit("ramdisk_entry.inc", source("kernel/src/kernel/filesystem/ramdisk_entry.hpp"));
const auto ramdisk_delegate_cpp = source("kernel/src/kernel/filesystem/filesystem_ramdiskdelegate.cpp");
emit("ramdisk_namespace.inc", function(ramdisk_delegate_cpp, "g_fs_open_status filesystemRamdiskDelegateLookup(")
    + function(ramdisk_delegate_cpp, "g_fs_open_status filesystemRamdiskDelegateCreate("));
emit("filesystem_open_policy.inc", function(filesystem_cpp, "g_fs_open_status filesystemOpen("));
emit("ramdisk_io.inc", function(ramdisk_delegate_cpp, "g_fs_read_status filesystemRamdiskDelegateRead(")
    + function(ramdisk_delegate_cpp, "g_fs_write_status filesystemRamdiskDelegateWrite(")
    + function(ramdisk_delegate_cpp, "g_fs_open_status filesystemRamdiskDelegateTruncate("));

emit("filesystem_unlink_node.inc", function(source("kernel/src/kernel/filesystem/filesystem_mutation.cpp"), "void filesystemUnlinkNode("));

emit("filesystem_mutation_syscalls.inc", source("kernel/src/kernel/calls/syscall_filesystem_mutation.cpp"));
emit("filesystem_path_mutations.inc", between(source("kernel/src/kernel/filesystem/filesystem_mutation.cpp"),
    "g_fs_mutation_status filesystemMutationLookupStatus(", "g_fs_mutation_status filesystemMutateDescriptor("));

emit("mutex_acquire.inc", replace(function(source("kernel/src/kernel/system/mutex.cpp"), "void mutexAcquire("), "asm volatile(\"pause\")", "std::this_thread::yield()"));

emit("syscall_classification.inc", function(syscall_cpp, "bool syscallStartsTask(")
    + "\n" + function(syscall_cpp, "bool syscallMutatesState("));
emit("power_coordinator.inc", source("kernel/src/kernel/system/power/power.cpp"));
emit("power_syscall.inc", function(source("kernel/src/kernel/calls/syscall_system.cpp"), "void syscallSystemPower("));
emit("pci_driver.inc", source("kernel/src/kernel/system/pci_driver.cpp"));

emit("power_pci_driver.inc", between(source("applications/pcidriver/src/pcidriver.cpp"), "static std::vector", "int main()"));
emit("pci_resource_access.inc", function(source("applications/pcidriver/src/pcidriver.cpp"), "void pciDriverHandleEnableResourceAccess("));

emit("acpi_driver.inc", source("kernel/src/kernel/system/acpi_driver.cpp"));
emit("acpi_power_bridge.inc", replace(source("kernel/src/kernel/system/power/acpi_power.cpp"), "asm volatile(\"pause\")", "(void)0"));
emit("acpi_irq_layouts.inc", between(source("kernel/src/kernel/system/acpi/acpi.hpp"), "struct g_acpi_table_header", "/**\n * Generic address")
    + between(source("kernel/src/kernel/system/acpi/madt.hpp"), "struct g_madt_header", "/**\n * Reads the Multiple"));
emit("irq_backend_types.inc", source("kernel/src/kernel/system/interrupts/irq.hpp")
    + source("kernel/src/kernel/system/interrupts/irq_route.hpp"));
emit("irq_backend.inc", source("kernel/src/kernel/system/acpi/irq_routing.cpp")
    + function(source("kernel/src/kernel/system/interrupts/pic.cpp"), "bool picSupportsTrigger(")
    + function(source("kernel/src/kernel/system/interrupts/apic/lapic.cpp"), "bool lapicVectorIsBusy(")
    + function(source("kernel/src/kernel/system/interrupts/apic/ioapic.cpp"), "bool ioapicVectorIsBusyElsewhere(")
    + source("kernel/src/kernel/system/interrupts/irq_route.cpp")
    + source("kernel/src/kernel/system/interrupts/irq.cpp"));

emit("irq_syscall_types.inc", between(source("libapi/inc/ghost/system/callstructs.h"), "typedef uint8_t g_irq_operation;", "__END_C"));
emit("irq_syscalls.inc", function(source("kernel/src/kernel/calls/syscall_system.cpp"), "static g_irq_status syscallIrqOperation(")
    + function(source("kernel/src/kernel/calls/syscall_system.cpp"), "void syscallIrq("));
emit("acpi_platform_syscall.inc", function(source("kernel/src/kernel/calls/syscall_system.cpp"), "void syscallAcpiGetPlatformInfo(")
    + function(source("kernel/src/kernel/calls/syscall_system.cpp"), "void syscallAcpiMap("));
emit("tasking_wait_if.inc", between(tasking_hpp, "template<typename PrepareWait, typename AfterYield>", "/** Marks the task waiting, attaches"));

emit("ioapic_flags.inc", defines(source("kernel/src/kernel/system/interrupts/apic/ioapic.hpp"), "IOAPIC_REDTBL"));

emit("filesystem_rename.inc", fromSignature(source("kernel/src/kernel/filesystem/filesystem_mutation.cpp"), "static bool filesystemRenameIsFile("));

emit("filesystem_open_directory.inc", function(source("kernel/src/kernel/calls/syscall_filesystem.cpp"), "void syscallFsOpenDirectory("));

auto filesystem_links_cpp = source("kernel/src/kernel/filesystem/filesystem_links.cpp");
emit("filesystem_link_format.inc", between(filesystem_links_cpp, "bool filesystemIsLinkName(", "static bool filesystemIsDirectory("));
emit("filesystem_links.inc", function(source("kernel/src/kernel/filesystem/filesystem_mutation.cpp"), "g_fs_mutation_status filesystemMutationLookupStatus(")
    + fromSignature(filesystem_links_cpp, "static bool filesystemIsDirectory("));
emit("filesystem_link_stat.inc", function(filesystem_cpp, "g_fs_stat_status filesystemStatNode(")
    + function(filesystem_cpp, "g_fs_stat_status filesystemStat("));

emit("filesystem_link_syscalls.inc", source("kernel/src/kernel/calls/syscall_filesystem_links.cpp"));

emit("physicalResources.inc", function(source("kernel/src/kernel/memory/memory.cpp"), "static uint64_t memoryCountPhysicalBytes(")
    + function(source("kernel/src/kernel/memory/memory.cpp"), "void memoryGetPhysicalUsage("));


emit("memory_user_hpp.inc", source("kernel/src/kernel/memory/memory_user.hpp"));
emit("memory_user_cpp.inc", memory_user_cpp);
emit("syscall_registration.inc", source("kernel/src/kernel/calls/syscall.hpp"));
emit("syscall_adapter.inc", function(syscall_cpp, "static void _syscallInvoke("));
emit("syscall_messages.inc", source("kernel/src/kernel/calls/syscall_messaging.cpp"));

emit("interrupt_handler.inc", function(source("kernel/src/kernel/system/interrupts/interrupts.cpp"), "extern \"C\" volatile g_processor_state* _interruptHandler("));
emit("paging_shootdown_hpp.inc", source("kernel/src/kernel/memory/paging_shootdown.hpp"));
emit("timer_configuration.inc", source("kernel/src/kernel/system/configuration.hpp"));

emit("clock_hpp.inc", source("kernel/src/kernel/tasking/clock.hpp"));
emit("clock_cpp.inc", clock_cpp);
emit("scheduler_reschedule.inc", function(scheduler_round_robin_cpp, "void schedulerInitializeLocal(")
    + function(scheduler_round_robin_cpp, "void schedulerRequestReschedule(")
    + function(scheduler_round_robin_cpp, "void schedulerAcknowledgeReschedule("));
emit("schedulerPreferTask.inc", function(scheduler_round_robin_cpp, "void schedulerPreferTask("));
emit("taskingReleaseSpawn.inc", function(tasking_cpp, "void taskingReleaseSpawn("));
emit("taskingAssign.inc", function(tasking_cpp, "void taskingAssign("));
const auto power_platform_cpp = source("kernel/src/kernel/system/power/power_platform.cpp");
emit("powerStopProcessors.inc", function(power_platform_cpp, "static bool powerStopProcessors("));
emit("powerProcessorCheckpoint.inc", replace(function(power_platform_cpp, "void powerProcessorCheckpoint("), "asm volatile(\"pause\")", "std::this_thread::yield()"));
const auto lapic_cpp = source("kernel/src/kernel/system/interrupts/apic/lapic.cpp");
emit("lapic_timer_hpp.inc", source("kernel/src/kernel/system/interrupts/apic/lapic.hpp") + "\n"
    + defines(source("kernel/src/kernel/system/configuration.hpp"), "G_TIMER_FREQUENCY"));
emit("lapic_timer_state.inc", between(lapic_cpp, "enum class g_lapic_timer_mode", "// All APICs"));
emit("lapic_timer.inc", between(lapic_cpp, "struct g_lapic_calibration_sample", "void lapicStartTimer(")
    + function(lapic_cpp, "void lapicStartTimer(") + function(lapic_cpp, "void lapicConfigureTimer("));
emit("lapic_ipi.inc", function(lapic_cpp, "void lapicSendIpi(")
    + function(lapic_cpp, "void lapicSendIpiAllOthers("));
emit("lapic_hypervisor.inc", function(source("kernel/src/kernel/system/processor/processor.cpp"), "bool processorHasHypervFeature(")
    + between(lapic_cpp, "static bool hypervisorEoi;", "enum class g_lapic_timer_mode")
    + function(lapic_cpp, "void lapicSendEndOfInterrupt("));
emit("lapic_interrupt_guard.inc", between(source("kernel/src/kernel/system/interrupts/interrupts.hpp"),
    "class g_interrupt_guard", "/**\n * Installs all ISRs"));
emit("reschedule_vector.inc", defines(source("kernel/src/kernel/tasking/scheduler/scheduler.hpp"), "G_SCHEDULER_RESCHEDULE_VECTOR"));

emit("taskingReleaseTask.inc", function(tasking_cpp, "void taskingReleaseTask("));
