#include "support/fixture.hpp"

namespace
{
void processorInitializeBsp() {}
void acpiInitialize(uintptr_t) {}
void apicDetect() {}
void hpetInitialize() {}
bool processorListAvailable() { return true; }
void gdtInitialize() {}
void gdtInitializeLocal() {}
void interruptsInitializeBsp() {}
void interruptsInitializeAp() {}
void syscallRegisterAll() {}
void processorFinalizeSetup() {}
int processorGetNumberOfProcessors() { return 16; }
uint32_t processorGetCurrentId() { return 0; }
uintptr_t pagingGetCurrentSpace() { return 4096; }
void smpInitialize(uintptr_t);

#include "production/system_host_cpp.inc"

// smpInitialize must see the AP count before it starts the processors.
void smpInitialize(uintptr_t) { EXPECT_EQ(15, applicationCoresWaiting); }
TEST(BootBarrier, PublishesAllApplicationCoreInitializationBeforeReady) {
    for(int round = 0; round < 40; ++round) {
        systemReady = false;
        systemInitializeBsp(0);
        int initialized[15] = {};
        std::vector<std::thread> threads;
        for(int core = 0; core < 15; ++core) {
            threads.emplace_back([&, core] {
                initialized[core] = core + 1;
                systemMarkApplicationCoreReady();
                systemWaitForReady();
                for(int i = 0; i < 15; ++i) EXPECT_EQ(i + 1, initialized[i]);
            });
        }
        systemWaitForApplicationCores();
        for(int i = 0; i < 15; ++i) EXPECT_EQ(i + 1, initialized[i]);
        systemMarkReady();
        for(auto& thread : threads) thread.join();
        EXPECT_TRUE(systemIsReady());
    }
}
}
