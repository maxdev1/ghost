#include <gtest/gtest.h>
#include "disk.hpp"
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

class PowerQemuProcess
{
    pid_t process = -1;
public:
    int status = -1;
    ~PowerQemuProcess()
    {
        if(process <= 0) return;
        kill(process, SIGTERM);
        while(waitpid(process, nullptr, 0) < 0 && errno == EINTR) {}
    }
    bool start(const std::vector<std::string>& command, const fs::path& log)
    {
        int fd = open(log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if(fd < 0) return false;
        std::vector<char*> arguments;
        for(const auto& argument : command) arguments.push_back(const_cast<char*>(argument.c_str()));
        arguments.push_back(nullptr);
        process = fork();
        if(process == 0)
        {
            dup2(fd, STDOUT_FILENO); dup2(fd, STDERR_FILENO); close(fd);
            execvp(arguments[0], arguments.data()); _exit(127);
        }
        close(fd);
        return process > 0;
    }
    bool running()
    {
        if(process <= 0) return false;
        int result = waitpid(process, &status, WNOHANG);
        if(result == 0 || (result < 0 && errno == EINTR)) return true;
        process = -1; return false;
    }
};

static std::string powerRead(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

TEST(PowerQemu, RebootsThenPowersOffAndPreservesDiskWrites)
{
    const fs::path root = HOST_REPOSITORY;
    const auto iso = root / "kernel/tests/bin/guest-power/power-test.iso";
    ASSERT_TRUE(fs::is_regular_file(iso)) << "Build kernel/tests/guest/power/build.sh in the toolchain container.";
    const char* configured = std::getenv("POWER_TEST_CASE");
    for(const auto& name : {"pc-1", "pc-2", "q35-1", "q35-2", "pc-nohpet-2"})
    {
        if(configured && std::string(configured) != name) continue;
        SCOPED_TRACE(name);
        char temporary[] = "/tmp/ghost-power-qemu-XXXXXX";
        auto created = mkdtemp(temporary); ASSERT_NE(nullptr, created);
        fs::path directory = created;
        auto volume = powerTestDiskImage();
        {
            std::ofstream disk(directory / "disk.raw", std::ios::binary);
            ASSERT_TRUE(disk.write(reinterpret_cast<const char*>(volume.data()), volume.size()));
        }
        std::string variant = name;
        std::vector<std::string> command{"qemu-system-x86_64", "-cdrom", iso.string(), "-m", "1024", "-smp",
            variant.back() == '2' ? "2" : "1", "-machine", variant.find("q35") == 0 ? "q35" :
            variant.find("nohpet") != std::string::npos ? "pc,hpet=off" : "pc", "-display", "none", "-nic", "none", "-boot", "order=d",
            "-serial", "file:" + (directory / "serial.log").string(), "-device", "ich9-ahci,id=powerAhci",
            "-drive", "if=none,id=powerdisk,format=raw,file=" + (directory / "disk.raw").string(),
            "-device", "ide-hd,drive=powerdisk,bus=powerAhci.0"};
        PowerQemuProcess vm;
        ASSERT_TRUE(vm.start(command, directory / "qemu.log"));
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
        while(vm.running() && std::chrono::steady_clock::now() < deadline)
        {
            auto serial = powerRead(directory / "serial.log");
            ASSERT_EQ(std::string::npos, serial.find("power-test: FAIL")) << serial;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        ASSERT_FALSE(vm.running()) << "Guest timeout; logs at " << directory;
        ASSERT_TRUE(WIFEXITED(vm.status)) << powerRead(directory / "qemu.log");
        EXPECT_EQ(0, WEXITSTATUS(vm.status));
        auto serial = powerRead(directory / "serial.log");
        for(const char* marker : {"veto and permission checks PASS", "actual reboot and disk persistence PASS",
            "committing reboot", "committing shutdown"}) EXPECT_NE(std::string::npos, serial.find(marker)) << serial;
        for(const char* fault : {"unhandled exception", "page fault", "unrecoverable error", "long lock", "power-test: FAIL"})
            EXPECT_EQ(std::string::npos, serial.find(fault)) << serial;
        EXPECT_NE(std::string::npos, powerRead(directory / "disk.raw").find("power-test-shutdown\n"));
        std::cout << name << " PASS; logs at " << directory << std::endl;
    }
}
