#include <ghost.h>
#include <ghost/system/acpi.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <unistd.h>

static uint8_t powerTestRegistered;
static uint8_t powerTestApprove;

static void powerTestRequire(bool condition, const char* what)
{
    if(condition) return;
    klog("power-test: FAIL %s", what);
    g_exit(1);
}

static g_acpi_info powerTestFirmwareInfo()
{
    auto service = g_task_get_by_name(G_ACPI_DRIVER_NAME);
    powerTestRequire(service != G_TID_NONE, "ACPI service");
    g_acpi_request request{G_ACPI_MAGIC, G_ACPI_VERSION, G_ACPI_GET_INFO, 0, G_TID_NONE, 0};
    auto transaction = g_get_message_tx_id();
    auto deadline = g_millis() + G_ACPI_TIMEOUT_MS;
    while(g_send_message_tm(service, &request, sizeof(request), transaction, G_MESSAGE_SEND_MODE_NON_BLOCKING) !=
        G_MESSAGE_SEND_STATUS_SUCCESSFUL)
    {
        powerTestRequire(g_millis() < deadline, "send firmware query");
        g_sleep(1);
    }
    uint8_t bytes[sizeof(g_message_header) + sizeof(g_acpi_response)];
    while(g_millis() < deadline)
    {
        powerTestRequire(g_task_get_by_name(G_ACPI_DRIVER_NAME) == service, "ACPI service remains available");
        if(g_receive_message_tm(bytes, sizeof(bytes), transaction, G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) !=
            G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
        {
            g_sleep(1);
            continue;
        }
        auto header = reinterpret_cast<g_message_header*>(bytes);
        if(header->sender != service || header->length != sizeof(g_acpi_response)) continue;
        g_acpi_response response;
        memcpy(&response, G_MESSAGE_CONTENT(bytes), sizeof(response));
        powerTestRequire(response.magic == G_ACPI_MAGIC && response.version == G_ACPI_VERSION &&
            response.command == G_ACPI_GET_INFO && response.token == 0 && response.status == G_ACPI_SUCCESSFUL,
            "firmware query response");
        return response.info;
    }
    powerTestRequire(false, "firmware query deadline");
    return {};
}

static void powerTestParticipant()
{
    powerTestRequire(g_power_register(G_POWER_APPLICATION) == G_POWER_SUCCESSFUL, "register application");
    __atomic_store_n(&powerTestRegistered, 1, __ATOMIC_RELEASE);
    uint8_t buffer[sizeof(g_message_header) + sizeof(g_power_event)];
    for(;;)
    {
        if(g_receive_message(buffer, sizeof(buffer)) != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) continue;
        auto header = reinterpret_cast<g_message_header*>(buffer);
        auto event = reinterpret_cast<g_power_event*>(G_MESSAGE_CONTENT(buffer));
        if(header->sender == G_TID_NONE && header->length == sizeof(*event) &&
            event->magic == G_POWER_EVENT_MAGIC && event->type == G_POWER_EVENT_PREPARE)
            powerTestRequire(g_power_acknowledge(event->request, __atomic_load_n(&powerTestApprove, __ATOMIC_ACQUIRE)) ==
                G_POWER_SUCCESSFUL, "acknowledge preparation");
    }
}

int main()
{
    auto firmware = powerTestFirmwareInfo();
    powerTestRequire(firmware.namespaceReady &&
        firmware.sleep[5].supported, "query userspace ACPI driver");
    uint64_t rsdp = 0; uint8_t ioapic = 0;
    powerTestRequire(g_acpi_get_platform_info(&rsdp, &ioapic) == G_ACPI_NOT_PERMITTED, "firmware bootstrap privilege");
    powerTestRequire(g_acpi_map(0x1000, 4096) == nullptr, "firmware mapping privilege");
    powerTestRequire(g_acpi_configure_irq(9, G_ACPI_PLATFORM_IRQ_INSTALL) == G_ACPI_NOT_PERMITTED, "SCI privilege");
    g_acpi_request spoof{G_ACPI_MAGIC, G_ACPI_VERSION, G_ACPI_PREPARE, 123, g_get_tid(), G_POWER_SHUTDOWN};
    auto tx = g_get_message_tx_id();
    auto service = g_task_get_by_name(G_ACPI_DRIVER_NAME);
    powerTestRequire(g_send_message_t(service, &spoof, sizeof(spoof), tx) == G_MESSAGE_SEND_STATUS_SUCCESSFUL, "send spoofed preparation");
    uint8_t reply[sizeof(g_message_header) + sizeof(g_acpi_response)];
    powerTestRequire(g_receive_message_t(reply, sizeof(reply), tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL &&
        reinterpret_cast<g_acpi_response*>(G_MESSAGE_CONTENT(reply))->status == G_ACPI_NOT_PERMITTED,
        "reject preparation outside kernel coordinator");
    klog("power-test: userspace ACPI and privilege checks PASS");
    powerTestRequire(g_system_power(99) == G_POWER_INVALID_ARGUMENT, "invalid action");
    powerTestRequire(g_power_register(G_POWER_STORAGE) == G_POWER_NOT_PERMITTED, "storage privilege");
    powerTestRequire(g_power_acknowledge(123, 1) == G_POWER_INVALID_ARGUMENT, "stale acknowledgement");
    powerTestRequire(g_kill(0) == G_KILL_STATUS_FAILED, "protect kernel task");
    powerTestRequire(g_create_task(reinterpret_cast<void*>(powerTestParticipant)) != G_TID_NONE, "preparation thread");
    while(!__atomic_load_n(&powerTestRegistered, __ATOMIC_ACQUIRE)) g_sleep(1);
    powerTestRequire(g_system_power(G_POWER_SHUTDOWN) == G_POWER_CANCELLED, "application veto");
    klog("power-test: veto and permission checks PASS");
    __atomic_store_n(&powerTestApprove, 1, __ATOMIC_RELEASE);

    std::string mount;
    auto deadline = g_millis() + 15000;
    while(mount.empty() && g_millis() < deadline)
    {
        auto directory = g_open_directory("/mount");
        if(directory)
        {
            while(auto entry = g_read_directory(directory))
                if(std::strncmp(entry->name, "fat", 3) == 0) mount = std::string("/mount/") + entry->name;
            g_close_directory(directory);
        }
        if(mount.empty()) g_sleep(10);
    }
    powerTestRequire(!mount.empty(), "FAT mount");
    auto path = mount + "/powerchk.txt";
    char previous[32]{};
    int fd = open(path.c_str(), O_RDONLY);
    bool rebooted = fd >= 0;
    if(rebooted)
    {
        powerTestRequire(read(fd, previous, sizeof(previous)) == 18, "read persisted data");
        close(fd);
        powerTestRequire(std::strcmp(previous, "power-test-reboot\n") == 0, "data survived reboot");
        klog("power-test: actual reboot and disk persistence PASS");
    }
    fd = open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0);
    powerTestRequire(fd >= 0, "create file after cancelled shutdown");
    const char* text = rebooted ? "power-test-shutdown\n" : "power-test-reboot\n";
    powerTestRequire(write(fd, text, std::strlen(text)) == static_cast<int>(std::strlen(text)), "write file");
    powerTestRequire(close(fd) == 0, "close file");
    klog(rebooted ? "power-test: requesting shutdown" : "power-test: requesting reboot");
    auto status = g_system_power(rebooted ? G_POWER_SHUTDOWN : G_POWER_REBOOT);
    klog("power-test: FAIL power operation returned %i", status);
    return 1;
}
