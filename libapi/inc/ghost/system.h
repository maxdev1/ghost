/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef GHOST_API_SYSTEM
#define GHOST_API_SYSTEM

#include "common.h"
#include "stdint.h"
#include "filesystem/types.h"
#include "system/types.h"
#include "system/power.h"
#include "memory/types.h"
#include "system/acpi.h"
#include "system/irq.h"

__BEGIN_C

// not implemented warning
#define __G_NOT_IMPLEMENTED_WARN(name)		g_log("'" #name "' is not implemented");
#define __G_NOT_IMPLEMENTED(name)		    __G_NOT_IMPLEMENTED_WARN(name) g_exit(0);

/**
 * Requests a legacy Virtual 8086 BIOS interrupt call.
 *
 * The x86-64 kernel returns G_VM86_CALL_STATUS_NOT_SUPPORTED for permitted calls.
 * Application-level callers receive G_VM86_CALL_STATUS_FAILED_NOT_PERMITTED.
 *
 * @param[in] interrupt BIOS interrupt number.
 * @param[in] in Non-null input register set.
 * @param[out] out Non-null register output, written only if the call succeeds.
 * @return A g_vm86_call_status code when the call returns.
 * @security-level DRIVER
 */
g_vm86_call_status g_call_vm86(uint32_t interrupt, g_vm86_registers* in, g_vm86_registers* out);

/**
 * Writes a message to the kernel log, prefixed with the caller's identity.
 *
 * @param[in] message Non-null, null-terminated text to log.
 * @security-level APPLICATION
 */
void g_log(const char* message);

/**
 * Opens a read descriptor for subsequent kernel log output.
 *
 * @return A descriptor to close with g_close(), or G_FD_NONE on failure.
 * @security-level APPLICATION
 */
g_fd g_open_log_pipe();

/**
 * Enables or disables kernel log rendering on the video output.
 *
 * @param[in] enabled Nonzero to enable video logging, or zero to disable it.
 * @security-level APPLICATION
 */
void g_set_video_log(uint8_t enabled);

/**
 * Invokes the kernel's diagnostic test entry point.
 *
 * @param[in] test Diagnostic value passed to the kernel.
 * @return The supplied test value in the current kernel.
 * @security-level APPLICATION
 */
uint32_t g_test(uint32_t test);

/**
 * Reads 8 bits directly from an x86 I/O port.
 *
 * @param[in] port I/O port number.
 * @return The 8-bit value read from the port.
 * @security-level DRIVER
 */
uint8_t g_io_port_read_byte(uint16_t port);

/**
 * Reads 16 bits directly from an x86 I/O port.
 *
 * @param[in] port I/O port number.
 * @return The 16-bit value read from the port.
 * @security-level DRIVER
 */
uint16_t g_io_port_read_word(uint16_t port);

/**
 * Reads 32 bits directly from an x86 I/O port.
 *
 * @param[in] port I/O port number.
 * @return The 32-bit value read from the port.
 * @security-level DRIVER
 */
uint32_t g_io_port_read_dword(uint16_t port);

/**
 * Writes 8 bits directly to an x86 I/O port.
 *
 * @param[in] port I/O port number.
 * @param[in] data The 8-bit value to write.
 * @security-level DRIVER
 */
void g_io_port_write_byte(uint16_t port, uint8_t data);

/**
 * Writes 16 bits directly to an x86 I/O port.
 *
 * @param[in] port I/O port number.
 * @param[in] data The 16-bit value to write.
 * @security-level DRIVER
 */
void g_io_port_write_word(uint16_t port, uint16_t data);

/**
 * Writes 32 bits directly to an x86 I/O port.
 *
 * @param[in] port I/O port number.
 * @param[in] data The 32-bit value to write.
 * @security-level DRIVER
 */
void g_io_port_write_dword(uint16_t port, uint32_t data);

/**
 * Retrieves the boot framebuffer's physical address and layout.
 *
 * Requires an available boot framebuffer. There is no status result; outputs must not be used
 * after an unprivileged call.
 *
 * @param[out] outFramebuffer Non-null output for the physical framebuffer base; map it before
 *                            access.
 * @param[out] outWidth Non-null output for the width in pixels.
 * @param[out] outHeight Non-null output for the height in pixels.
 * @param[out] outBpp Non-null output for the bits per pixel.
 * @param[out] outPitch Non-null output for the byte stride between rows.
 * @security-level DRIVER
 */
void g_get_efi_framebuffer(g_address* outFramebuffer, uint16_t* outWidth, uint16_t* outHeight, uint16_t* outBpp, uint32_t* outPitch);

/**
 * Retrieves firmware-table and interrupt-controller information.
 *
 * @param[out] rsdp Receives the physical RSDP address on success; may be null.
 * @param[out] ioapic Receives nonzero if IOAPIC routing is available; may be null.
 * @return A g_acpi_status code; G_ACPI_NOT_SUPPORTED indicates that no RSDP is available.
 * @security-level DRIVER
 */
g_acpi_status g_acpi_get_platform_info(uint64_t* rsdp, uint8_t* ioapic);

/**
 * Maps a physical ACPI table or register region without CPU caching.
 *
 * The mapping covers whole pages. Release it with g_unmap() using the returned address rounded
 * down to its page boundary.
 *
 * @param[in] address Physical byte address; page alignment is not required.
 * @param[in] size Positive length in bytes.
 * @return The virtual address of the requested byte, or null on failure.
 * @security-level DRIVER
 */
void* g_acpi_map(uint64_t address, uint32_t size);

/**
 * Claims an exclusive interrupt route for the calling driver thread. The
 * route starts masked. Configure the device, then call g_irq_complete to
 * enable delivery before the first g_irq_wait. A PCI driver supplies an idle
 * route and verifies delivery before relying on interrupts.
 *
 * Returns G_IRQ_SUCCESSFUL and a nonzero handle on success. A failed call
 * clears the output handle.
 * @security-level DRIVER
 */
g_irq_status g_irq_bind(g_irq_source source, uint32_t line, g_irq_handle* handle);

/**
 * Waits on the binding from the task that claimed it. A zero timeout waits
 * indefinitely. Multiple interrupts can produce one pending notification, so
 * check the device for all outstanding work. Returns G_IRQ_DELIVERED when a
 * notification is consumed, G_IRQ_TIMED_OUT when the deadline expires, or
 * G_IRQ_RELEASED when the binding has been released.
 * A timeout preserves the route's arming state and any pending notification.
 * @security-level DRIVER
 */
g_irq_status g_irq_wait(g_irq_handle handle, uint32_t timeoutMs);

/**
 * Arms the binding initially or after servicing the device. For a level
 * interrupt, clear the device's cause before completing. Edge-triggered
 * interrupts remain enabled during delivery. Completing preserves unread level
 * notifications and edges already pending in the kernel. A task in the owning
 * process may complete the binding.
 * @security-level DRIVER
 */
g_irq_status g_irq_complete(g_irq_handle handle);

/**
 * Pauses delivery at the interrupt controller while keeping the binding and
 * any notification already pending. Use g_irq_complete to resume delivery.
 * Masking alone does not wake a blocked waiter. A thread in the owning
 * process may mask the binding.
 * @security-level DRIVER
 */
g_irq_status g_irq_mask(g_irq_handle handle);

/**
 * Invalidates the handle and wakes its waiter. The kernel restores the prior
 * route after any interrupt already in flight has drained. Rebinding that
 * source can return G_IRQ_BUSY in the meantime. Disable interrupts at the
 * device before release. A task in the owning process may release the binding.
 * @security-level DRIVER
 */
g_irq_status g_irq_release(g_irq_handle handle);

/**
 * Requests an orderly shutdown or reboot.
 *
 * The kernel coordinates registered participants and storage flushing. Vetoes, timeouts and I/O
 * failures cancel the request; there is no force option.
 *
 * @param[in] action G_POWER_SHUTDOWN or G_POWER_REBOOT.
 * @return A g_power_status describing failure or cancellation; successful power-off or reset does
 *         not return.
 * @security-level APPLICATION
 */
g_power_status g_system_power(g_power_action action);

/**
 * Registers the calling thread as a power-preparation participant.
 *
 * Receive g_power_event messages from sender G_TID_NONE and acknowledge preparation within 30
 * seconds. Re-registering updates this thread's role. Storage producers and platform participants
 * acknowledge without blocking and restore service and acknowledge again on cancellation.
 *
 * @param[in] role One of G_POWER_APPLICATION, G_POWER_STORAGE, G_POWER_PLATFORM or
 *                 G_POWER_STORAGE_PRODUCER.
 * @return A g_power_status code; G_POWER_BUSY also covers registration during an active request.
 * @security-level APPLICATION for G_POWER_APPLICATION; DRIVER for all other roles.
 */
g_power_status g_power_register(g_power_role role);

/**
 * Acknowledges the current power event for the registered calling thread.
 *
 * A successful readiness acknowledgement by G_POWER_STORAGE blocks until cancellation, so storage
 * service resumes when the call returns. Other roles return without waiting.
 *
 * @param[in] request Request generation from the received g_power_event.
 * @param[in] ready Nonzero to report readiness or completed cancellation; zero to veto
 *                  preparation.
 * @return A g_power_status code; stale requests and unregistered or duplicate acknowledgements
 *         fail.
 * @security-level APPLICATION
 */
g_power_status g_power_acknowledge(uint32_t request, uint8_t ready);

/**
 * Returns a human-readable description of a power status.
 *
 * @param[in] status Power-operation status to describe.
 * @return A static, null-terminated string; do not modify or free it. Unknown codes use a generic
 *         invalid-request message.
 * @security-level APPLICATION
 */
const char* g_power_status_string(g_power_status status);

__END_C

#endif
