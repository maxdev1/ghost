/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <gtest/gtest.h>
#include "qemu.hpp"
#include "disk.hpp"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

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
        g_qemu_test_process vm;

        ASSERT_TRUE(vm.start(command, directory / "qemu.log"));

        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
        while(vm.running() && std::chrono::steady_clock::now() < deadline)
        {
            auto serial = powerRead(directory / "serial.log");

            ASSERT_EQ(std::string::npos, serial.find("power-test: FAIL")) << serial;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        ASSERT_FALSE(vm.running()) << "Guest timeout; logs at " << directory;
        auto status = vm.waitStatus();
        ASSERT_NE(-1, status) << powerRead(directory / "qemu.log");
        ASSERT_TRUE(WIFEXITED(status)) << powerRead(directory / "qemu.log");
        EXPECT_EQ(0, WEXITSTATUS(status));

        auto serial = powerRead(directory / "serial.log");
        for(const char* marker : {"veto and permission checks PASS", "actual reboot and disk persistence PASS",
            "committing reboot", "committing shutdown"}) EXPECT_NE(std::string::npos, serial.find(marker)) << serial;
        for(const char* fault : {"unhandled exception", "page fault", "unrecoverable error", "long lock", "power-test: FAIL"})
            EXPECT_EQ(std::string::npos, serial.find(fault)) << serial;
        EXPECT_NE(std::string::npos, powerRead(directory / "disk.raw").find("power-test-shutdown\n"));

        std::cout << name << " PASS; logs at " << directory << std::endl;
    }
}
