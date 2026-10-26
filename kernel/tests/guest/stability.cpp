// Run in a two-core Ghost guest; see ../README.md. Not installed by normal builds.
#include <ghost.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <initializer_list>

static void require(bool condition, const char* what)
{
    if(condition) return;
    char message[160]; snprintf(message, sizeof(message), "stability FAIL: %s", what);
    g_log(message); g_exit(1);
}

static unsigned phase, acknowledged;
static uintptr_t sharedAddress;
static void tlbReader()
{
    for(unsigned i = 1; i <= 200; ++i)
    {
        while(__atomic_load_n(&phase, __ATOMIC_ACQUIRE) != i) g_yield();
        auto page = (volatile unsigned*) __atomic_load_n(&sharedAddress, __ATOMIC_ACQUIRE);
        require(page[0] == i, "remote TLB reused an old mapping");
        __atomic_store_n(&acknowledged, i, __ATOMIC_RELEASE);
    }
}
static void tlbController()
{
    auto reader = g_create_task_a((void*) tlbReader, 1);
    require(reader != G_TID_NONE, "TLB reader creation");
    uintptr_t previous = 0;
    void* retained = nullptr;
    for(unsigned i = 1; i <= 200; ++i)
    {
        auto page = (unsigned*) g_alloc_mem(4096);
        require(page != nullptr, "TLB allocation");
        if(previous) require((uintptr_t) page == previous, "fixture must reuse virtual address");
        page[0] = i;
        __atomic_store_n(&sharedAddress, (uintptr_t) page, __ATOMIC_RELEASE);
        __atomic_store_n(&phase, i, __ATOMIC_RELEASE);
        while(__atomic_load_n(&acknowledged, __ATOMIC_ACQUIRE) != i) g_yield();
        if(retained) g_unmap(retained);
        // Keep the old physical frame alive through a second mapping, so the
        // next allocation reuses the VA with a different frame. A stale TLB
        // entry must see the old pattern, rather than accidentally passing.
        retained = g_share_mem(page, 4096, g_get_pid());
        require(retained != nullptr, "retain old physical frame");
        previous = (uintptr_t) page;
        g_unmap(page);
    }
    g_unmap(retained); g_join(reader); g_log("stability PASS: cross-core mapping reuse (200)");
}

struct Io { g_fd fd; bool write; unsigned started, finished; };
static void blockedIo(void* data)
{
    auto io = (Io*) data; char byte = 0;
    __atomic_store_n(&io->started, 1, __ATOMIC_RELEASE);
    if(io->write)
    {
        g_fs_write_status status;
        require(g_write_s(io->fd, &byte, 1, &status) == -1 && status == G_FS_WRITE_INVALID_FD,
                "close must cancel a blocked writer");
    }
    else
    {
        g_fs_read_status status;
        require(g_read_s(io->fd, &byte, 1, &status) == -1 && status == G_FS_READ_INVALID_FD,
                "close must cancel a blocked reader");
    }
    __atomic_store_n(&io->finished, 1, __ATOMIC_RELEASE);
}
static void descriptorChecks()
{
    static char buffer[16384];
    for(int cycle = 0; cycle < 30; ++cycle)
        for(bool write : {false, true})
        {
            g_fd writer, reader;
            require(g_pipe(&writer, &reader) == G_FS_PIPE_SUCCESSFUL, "pipe creation");
            if(write) require(g_write(writer, buffer, sizeof(buffer)) == sizeof(buffer), "fill pipe");
            Io io{write ? writer : reader, write, 0, 0};
            auto worker = g_create_task_da((void*) blockedIo, &io, 1);
            require(worker != G_TID_NONE, "I/O worker creation");
            while(!__atomic_load_n(&io.started, __ATOMIC_ACQUIRE)) g_yield();
            g_sleep(20);
            require(g_close(io.fd) == G_FS_CLOSE_SUCCESSFUL, "concurrent close");
            for(int ms = 0; !__atomic_load_n(&io.finished, __ATOMIC_ACQUIRE) && ms < 2000; ms += 10)
                g_sleep(10);
            require(__atomic_load_n(&io.finished, __ATOMIC_ACQUIRE), "closed I/O did not wake");
            g_join(worker); g_close(write ? reader : writer);
        }
    g_log("stability PASS: close of blocked read/write (60)");
}
static void cwdWriter()
{
    for(int i = 0; i < 300; ++i)
        require(g_set_working_directory(i & 1 ? "/system" : "/") == G_SET_WORKING_DIRECTORY_SUCCESSFUL,
                "set working directory");
}
static void cwdChecks()
{
    require(g_set_working_directory("/") == G_SET_WORKING_DIRECTORY_SUCCESSFUL, "initial working directory");
    auto writer = g_create_task_a((void*) cwdWriter, 1);
    require(writer != G_TID_NONE, "CWD worker creation");
    char path[4096];
    for(int i = 0; i < 300; ++i)
    {
        require(g_get_working_directory_l(path, sizeof(path)) == G_GET_WORKING_DIRECTORY_SUCCESSFUL,
                "get working directory");
        if(strcmp(path, "/") != 0 && strcmp(path, "/system") != 0) g_log(path);
        require(strcmp(path, "/") == 0 || strcmp(path, "/system") == 0, "torn working directory");
    }
    g_join(writer); g_log("stability PASS: concurrent working directory");
}
static void memoryChecks()
{
    constexpr unsigned chunk = 4 * 1024 * 1024;
    void* blocks[128]; unsigned count = 0;
    while(count < 128)
    {
        auto block = g_alloc_mem(chunk);
        if(!block) break;
        blocks[count++] = block;
        auto bytes = (volatile uint8_t*) block;
        bytes[0] = 0x39; bytes[chunk - 1] = 0x93;
    }
    // Use -m 384: the physical allocator must fail before the 512 MiB VA pool fills.
    require(count > 0 && count < 128, "fixture did not reach physical exhaustion");
    g_log("stability PASS: allocation returned failure under physical pressure");
    for(unsigned i = 0; i < count; ++i) g_unmap(blocks[i]);
    auto recovered = g_alloc_mem(chunk);
    require(recovered != nullptr, "allocation did not recover"); g_unmap(recovered);
    g_log("stability PASS: physical exhaustion cleanup and recovery");
}
int main(int argc, char** argv)
{
    if(argc > 1 && strcmp(argv[1], "fault") == 0)
    {
        g_log("stability START: invalid frame-pointer fault");
        asm volatile("mov $-1, %%rbp; ud2" ::: "memory");
        __builtin_unreachable();
    }
    if(argc > 1 && strcmp(argv[1], "memory") == 0) memoryChecks();
    else
    {
        auto controller = g_create_task_a((void*) tlbController, 0);
        require(controller != G_TID_NONE, "TLB controller creation");
        g_join(controller); descriptorChecks(); cwdChecks();
    }
    g_log("stability PASS: complete");
    puts("stability checks passed");
    return 0;
}
