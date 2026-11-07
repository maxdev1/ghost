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

#include "libps2/ps2.hpp"
#include "ps2_intern.hpp"

#include <ghost.h>
#include <stdio.h>

static constexpr uint8_t PS2_OUTPUT_FULL = 0x01;
static constexpr uint8_t PS2_INPUT_FULL = 0x02;
static constexpr uint8_t PS2_AUXILIARY = 0x20;
static constexpr uint8_t PS2_ERROR = 0xC0;
static constexpr uint64_t PS2_TIMEOUT_MS = 200;

static uint8_t mousePacketNumber = 0;
static uint8_t mousePacketBuffer[4];
static uint8_t mouseDeviceId = 0;
static uint64_t lastPacketTime = 0;
static bool mouseRecoveryNeeded = false;
static bool mouseResyncNeeded = false;
static uint64_t nextMouseRecovery = 0;
struct g_ps2_irq_receiver
{
	uint32_t line;
	g_irq_handle handle;
	bool attempted;
};

static g_ps2_irq_receiver keyReceiver{1, G_IRQ_HANDLE_NONE, false};
static g_ps2_irq_receiver mouseReceiver{12, G_IRQ_HANDLE_NONE, false};
static bool receiversReady = false;
static g_user_mutex packetLock = g_mutex_initialize();
static g_user_mutex eventSignal = g_mutex_initialize();
static g_tid dispatchTask = G_TID_NONE;

struct ps2_pending_event
{
	bool mouse;
	bool buttonEdge;
	int32_t x;
	int32_t y;
	uint8_t flags;
	uint8_t value; // Keyboard byte or signed wheel delta.
	uint64_t millis; // Packet intake time, before consumer I/O can block.
};

static constexpr size_t PS2_EVENT_CAPACITY = 512;
static ps2_pending_event pendingEvents[PS2_EVENT_CAPACITY];
static size_t eventHead = 0;
static size_t eventCount = 0;
static uint8_t lastMouseButtons = 0;
static bool mouseStateLost = false;
static uint32_t eventOverruns = 0;

static bool ps2QueueEvent(ps2_pending_event event)
{
	if(event.mouse)
	{
		const uint8_t buttons = event.flags & 7;
		event.buttonEdge = buttons != lastMouseButtons;
		lastMouseButtons = buttons;
		// Coalesce only under pressure. Button/scroll positions and keyboard
		// ordering remain distinct; wide accumulators retain fast motion.
		if(eventCount >= PS2_EVENT_CAPACITY / 2 && !event.buttonEdge && event.value == 0)
		{
			auto& tail = pendingEvents[(eventHead + eventCount - 1) % PS2_EVENT_CAPACITY];
			const int64_t x = (int64_t) tail.x + event.x;
			const int64_t y = (int64_t) tail.y + event.y;
			if(tail.mouse && !tail.buttonEdge && tail.value == 0 &&
			   (tail.flags & 7) == buttons && x >= INT32_MIN && x <= INT32_MAX &&
			   y >= INT32_MIN && y <= INT32_MAX)
			{
				tail.x = x;
				tail.y = y;
				tail.millis = event.millis;
				return true;
			}
		}
	}

	if(eventCount == PS2_EVENT_CAPACITY)
	{
		++eventOverruns;
		mouseStateLost |= event.mouse;
		return false;
	}

	pendingEvents[(eventHead + eventCount++) % PS2_EVENT_CAPACITY] = event;
	return true;
}

static void ps2QueueMouse(int16_t x, int16_t y, uint8_t flags, int8_t scroll, uint64_t millis)
{
	ps2QueueEvent({true, false, x, y, flags, (uint8_t) scroll, millis});
}

static void ps2QueueKeyboard(uint8_t value)
{
	ps2QueueEvent({false, false, 0, 0, 0, value, 0});
}

static void (*registeredMouseCallback)(int16_t, int16_t, uint8_t, int8_t, uint64_t);
static void (*registeredKeyboardCallback)(uint8_t);

static void ps2HandlePacket();
static void ps2DispatchPending();

bool ps2WaitForBuffer(ps2_buffer_t buffer)
{
	const uint8_t mask = buffer == PS2_OUT ? PS2_INPUT_FULL : PS2_OUTPUT_FULL;
	const uint8_t expected = buffer == PS2_OUT ? 0 : PS2_OUTPUT_FULL;
	const uint64_t start = g_millis();
	for(;;)
	{
		if((g_io_port_read_byte(G_PS2_STATUS_PORT) & mask) == expected)
			return true;
		if(g_millis() - start >= PS2_TIMEOUT_MS)
			return false;

		g_sleep(1);
	}
}

static bool ps2ControllerCommand(uint8_t command)
{
	if(!ps2WaitForBuffer(PS2_OUT))
	{
		klog("PS/2 controller: timed out before command %x", command);
		return false;
	}

	g_io_port_write_byte(G_PS2_STATUS_PORT, command);
	return true;
}

static bool ps2WriteData(uint8_t value)
{
	if(!ps2WaitForBuffer(PS2_OUT))
	{
		klog("PS/2 controller: timed out before data %x", value);
		return false;
	}

	g_io_port_write_byte(G_PS2_DATA_PORT, value);
	return true;
}

// The keyboard and mouse share port 0x60. Preserve keyboard input while waiting
// for mouse replies, and never read data unless the controller says it is ready.
static bool ps2ReadByte(bool auxiliary, uint8_t& value, uint64_t timeout = PS2_TIMEOUT_MS)
{
	const uint64_t start = g_millis();
	while(g_millis() - start < timeout)
	{
		const uint8_t status = g_io_port_read_byte(G_PS2_STATUS_PORT);
		if(!(status & PS2_OUTPUT_FULL))
		{
			g_sleep(1);
			continue;
		}

		const uint8_t data = g_io_port_read_byte(G_PS2_DATA_PORT);
		const bool fromMouse = status & PS2_AUXILIARY;
		if(status & PS2_ERROR)
		{
			if(fromMouse == auxiliary)
				return false;
			continue;
		}

		if(fromMouse == auxiliary)
		{
			value = data;
			return true;
		}

		if(!fromMouse && registeredKeyboardCallback)
			ps2QueueKeyboard(data);
	}

	return false;
}

int ps2WriteToMouse(uint8_t value)
{
	for(int attempt = 0; attempt < 3; ++attempt)
	{
		if(!ps2ControllerCommand(0xD4) || !ps2WriteData(value))
			return 1;

		const uint64_t start = g_millis();
		for(;;)
		{
			const uint64_t elapsed = g_millis() - start;
			uint8_t reply;
			if(elapsed >= PS2_TIMEOUT_MS || !ps2ReadByte(true, reply, PS2_TIMEOUT_MS - elapsed))
			{
				klog("PS/2 mouse: no ACK for byte %x, controller status %x", value,
				     g_io_port_read_byte(G_PS2_STATUS_PORT));
				return 1;
			}

			if(reply == 0xFA)
				return 0;
			if(reply == 0xFE) // RESEND applies to this byte, including command parameters.
				break;
			if(reply == 0xFC || reply == 0xFD)
			{
				klog("PS/2 mouse: byte %x rejected with reply %x", value, reply);
				return 1;
			}
			// Reports already in flight may precede the ACK to disable reporting.
		}
	}

	klog("PS/2 mouse: resend limit reached for byte %x", value);
	return 1;
}

/**
 * Encodes a Synaptics argument through four standard resolution commands.
 * Reporting must be disabled and the shared controller lock held by the caller.
 */
static bool ps2SynapticsArgument(uint8_t argument)
{
	if(ps2WriteToMouse(0xE6))
		return false;

	for(int shift = 6; shift >= 0; shift -= 2)
		if(ps2WriteToMouse(0xE8) || ps2WriteToMouse((argument >> shift) & 3))
			return false;

	return true;
}

/**
 * Identifies Synaptics touchpads without enabling their absolute packet format.
 * Other mice return ordinary status; unsuccessful probes fall back to defaults.
 */
static bool ps2SynapticsIdentify(uint8_t* identity)
{
	if(!ps2SynapticsArgument(0) || ps2WriteToMouse(0xE9))
		return false;

	for(unsigned i = 0; i < 3; ++i)
	{
		if(!ps2ReadByte(true, identity[i]))
		{
			klog("PS/2 mouse: identify response incomplete at byte %i", i);
			return false;
		}
	}

	klog("PS/2 mouse: identify response %x %x %x", identity[0], identity[1], identity[2]);
	return identity[1] == 0x47;
}

static bool ps2InitializeController()
{
	// Quiesce both ports before reading the controller configuration so scan
	// codes and firmware's pending mouse reports cannot masquerade as its reply.
	if(!ps2ControllerCommand(0xAD) || !ps2ControllerCommand(0xA7))
		return false;

	const uint64_t start = g_millis();
	while(g_io_port_read_byte(G_PS2_STATUS_PORT) & PS2_OUTPUT_FULL)
	{
		if(g_millis() - start >= PS2_TIMEOUT_MS)
		{
			klog("PS/2 controller: output buffer did not drain");
			return false;
		}
		g_io_port_read_byte(G_PS2_DATA_PORT);
	}

	uint8_t configuration;
	if(!ps2ControllerCommand(0x20) || !ps2ReadByte(false, configuration))
	{
		klog("PS/2 controller: could not read configuration");
		return false;
	}

	klog("PS/2 controller: configuration %x", configuration);
	// Preserve translation and other firmware settings; enable both IRQs.
	if(!ps2ControllerCommand(0x60) || !ps2WriteData(configuration | 0x03))
		return false;

	// Enable-port commands do not produce replies.
	return ps2ControllerCommand(0xAE) && ps2ControllerCommand(0xA8);
}

ps2_status_t ps2InitializeMouse()
{
	mousePacketNumber = 0;
	mouseDeviceId = 0;
	lastPacketTime = 0;

	if(ps2WriteToMouse(0xF5) || ps2WriteToMouse(0xF6))
		return G_PS2_STATUS_FAILED_INITIALIZE;

	uint8_t identity[3];
	const bool synaptics = ps2SynapticsIdentify(identity);
	if(synaptics)
	{
		/**
		 * Set Defaults clears absolute mode but preserves sleep and gesture bits.
		 * Write the whole mode byte to wake the pad and enable firmware tapping.
		 */
		if(!ps2SynapticsArgument(0) || ps2WriteToMouse(0xF3) || ps2WriteToMouse(20))
		{
			klog("PS/2 Synaptics touchpad: failed to set relative mode");
			return G_PS2_STATUS_FAILED_INITIALIZE;
		}

		klog("PS/2 Synaptics touchpad: firmware %i.%i, relative mode enabled", identity[2] & 15, identity[0]);
	}
	else
	{
		/**
		 * Restore resolution after probing before negotiating wheel packets.
		 * Synaptics pads keep their standard three-byte relative packet format.
		 */
		if(ps2WriteToMouse(0xF6) ||
		   ps2WriteToMouse(0xF3) || ps2WriteToMouse(200) ||
		   ps2WriteToMouse(0xF3) || ps2WriteToMouse(100) ||
		   ps2WriteToMouse(0xF3) || ps2WriteToMouse(80))
			return G_PS2_STATUS_FAILED_INITIALIZE;
	}

	if(ps2WriteToMouse(0xF2))
		return G_PS2_STATUS_FAILED_INITIALIZE;

	uint8_t id;
	if(!ps2ReadByte(true, id))
	{
		klog("PS/2 mouse: missing device id");
		return G_PS2_STATUS_FAILED_INITIALIZE;
	}

	if((synaptics && id != 0) || (id != 0 && id != 3 && id != 4))
	{
		klog("PS/2 mouse: unsupported device id %i", id);
		return G_PS2_STATUS_FAILED_INITIALIZE;
	}

	mouseDeviceId = id;

	// The wheel-detection sequence leaves the rate at 80 Hz. Restore a normal
	// report rate to reduce accumulated motion per packet.
	if(ps2WriteToMouse(0xF3) || ps2WriteToMouse(100) || ps2WriteToMouse(0xF4))
		return G_PS2_STATUS_FAILED_INITIALIZE;

	klog("PS/2 mouse: device id %i, %i-byte packets", id, id == 0 ? 3 : 4);
	return G_PS2_STATUS_SUCCESS;
}

ps2_status_t ps2Initialize(void (*mouseCallback)(int16_t, int16_t, uint8_t, int8_t, uint64_t),
                           void (*keyboardCallback)(uint8_t))
{
	g_mutex_acquire(packetLock);
	registeredMouseCallback = mouseCallback;
	registeredKeyboardCallback = keyboardCallback;
	receiversReady = false;
	mouseRecoveryNeeded = mouseResyncNeeded = false;
	nextMouseRecovery = 0;
	eventHead = eventCount = 0;
	lastMouseButtons = 0;
	mouseStateLost = false;
	eventOverruns = 0;
	g_mutex_try_acquire(eventSignal);

	keyReceiver.handle = mouseReceiver.handle = G_IRQ_HANDLE_NONE;
	keyReceiver.attempted = mouseReceiver.attempted = false;
	g_tid keyTask = G_TID_NONE, mouseTask = G_TID_NONE;
	dispatchTask = G_TID_NONE;

	ps2_status_t status = ps2InitializeController() ? ps2InitializeMouse() : G_PS2_STATUS_FAILED_INITIALIZE;
	if(status == G_PS2_STATUS_SUCCESS)
	{
		keyTask = g_create_task_das((void*) &ps2AwaitKeyIrq, nullptr, 0, nullptr);
		mouseTask = g_create_task_das((void*) &ps2AwaitMouseIrq, nullptr, 0, nullptr);
		dispatchTask = g_create_task_das((void*) &ps2DispatchEvents, nullptr, 0, nullptr);
		if(keyTask == G_TID_NONE || mouseTask == G_TID_NONE || dispatchTask == G_TID_NONE)
			status = G_PS2_STATUS_FAILED_INITIALIZE;
	}

	if(status == G_PS2_STATUS_SUCCESS)
	{
		/**
		 * Both waiters bind masked on core zero. Keep packetLock until both
		 * routes are armed so neither worker can consume input on partial startup.
		 */
		uint64_t started = g_millis();
		while(!memoryAtomicLoad(&keyReceiver.attempted, G_MEMORY_ORDER_ACQUIRE) ||
		      !memoryAtomicLoad(&mouseReceiver.attempted, G_MEMORY_ORDER_ACQUIRE))
		{
			if(g_millis() - started >= PS2_TIMEOUT_MS)
			{
				status = G_PS2_STATUS_FAILED_INITIALIZE;
				break;
			}
			g_sleep(1);
		}
		if(status == G_PS2_STATUS_SUCCESS &&
		   (!keyReceiver.handle || !mouseReceiver.handle ||
		    g_irq_complete(keyReceiver.handle) != G_IRQ_SUCCESSFUL ||
		    g_irq_complete(mouseReceiver.handle) != G_IRQ_SUCCESSFUL))
			status = G_PS2_STATUS_FAILED_INITIALIZE;
	}

	receiversReady = status == G_PS2_STATUS_SUCCESS;
	if(!receiversReady)
	{
		ps2ControllerCommand(0xAD);
		ps2ControllerCommand(0xA7);
		klog("PS/2 input initialization failed");
	}

	const bool pending = receiversReady && eventCount;
	g_mutex_release(packetLock);
	if(pending)
	{
		g_mutex_release(eventSignal);
		if(dispatchTask != G_TID_NONE)
			g_yield_t(dispatchTask);
	}

	if(status != G_PS2_STATUS_SUCCESS)
	{
		if(keyTask != G_TID_NONE)
			g_join(keyTask);
		if(mouseTask != G_TID_NONE)
			g_join(mouseTask);
		if(dispatchTask != G_TID_NONE)
			g_join(dispatchTask);
	}

	return status;
}

static bool ps2ReceiverReady()
{
	g_mutex_acquire(packetLock);
	const bool ready = receiversReady;
	g_mutex_release(packetLock);
	return ready;
}

static void ps2AwaitIrq(g_ps2_irq_receiver* receiver, const char* name)
{
	g_task_register_name(name);
	g_irq_status bound = g_irq_bind(G_IRQ_SOURCE_ISA, receiver->line, &receiver->handle);
	memoryAtomicStore(&receiver->attempted, true, G_MEMORY_ORDER_RELEASE);
	if(bound != G_IRQ_SUCCESSFUL)
		return;

	if(ps2ReceiverReady())
	{
		for(;;)
		{
			auto status = g_irq_wait(receiver->handle, 50);
			if(status != G_IRQ_DELIVERED && status != G_IRQ_TIMED_OUT)
				break;
			ps2CheckForData();
			if(status == G_IRQ_DELIVERED && g_irq_complete(receiver->handle) != G_IRQ_SUCCESSFUL)
				break;
		}
	}
	g_irq_release(receiver->handle);
}

void ps2AwaitKeyIrq()
{
	ps2AwaitIrq(&keyReceiver, "libps2/await-key");
}

void ps2AwaitMouseIrq()
{
	ps2AwaitIrq(&mouseReceiver, "libps2/await-mouse");
}

// One dispatcher preserves event order and may block on consumer I/O. IRQ
// receivers never call consumers and can keep draining while it is blocked.
static void ps2DispatchPending()
{
	for(;;)
	{
		g_mutex_acquire(packetLock);
		const uint32_t overruns = eventOverruns;
		eventOverruns = 0;
		ps2_pending_event event{};
		bool available = eventCount != 0;
		if(available)
		{
			auto& head = pendingEvents[eventHead];
			event = head;
			if(event.mouse)
			{
				event.x = head.x < INT16_MIN ? INT16_MIN : head.x > INT16_MAX ? INT16_MAX : head.x;
				event.y = head.y < INT16_MIN ? INT16_MIN : head.y > INT16_MAX ? INT16_MAX : head.y;
				head.x -= event.x;
				head.y -= event.y;
			}

			if(!event.mouse || (head.x == 0 && head.y == 0))
			{
				eventHead = (eventHead + 1) % PS2_EVENT_CAPACITY;
				--eventCount;
			}
		}
		else if(mouseStateLost)
		{
			// Finite queues cannot retain unlimited button/scroll/key events.
			// Report overload and restore the final physical button state.
			event = {true, false, 0, 0, lastMouseButtons, 0, g_millis()};
			mouseStateLost = false;
			available = true;
		}

		g_mutex_release(packetLock);
		if(overruns)
			klog("PS/2 input queue overloaded: %i events lost", overruns);
		if(!available)
			return;
		if(event.mouse)
		{
			if(registeredMouseCallback)
				registeredMouseCallback(event.x, event.y, event.flags, (int8_t) event.value, event.millis);
		}
		else if(registeredKeyboardCallback)
			registeredKeyboardCallback(event.value);
	}
}

void ps2DispatchEvents()
{
	if(!ps2ReceiverReady())
		return;

	g_task_register_name("libps2/dispatch");
	for(;;)
	{
		// Used as a binary event: producers release it; the consumer acquires
		// it and leaves it locked. A wake before the wait is retained.
		g_mutex_acquire(eventSignal);
		ps2DispatchPending();
	}
}

static bool ps2ResetAnnouncement()
{
	return mousePacketNumber == 2 && mousePacketBuffer[0] == 0xAA &&
	       mousePacketBuffer[1] == 0 && g_millis() - lastPacketTime > 5;
}

static bool ps2ResynchronizeMouse()
{
	// Disable reports, discard in-flight bytes up to its ACK, then obtain one
	// explicitly framed packet. Do not guess a new offset from payload bits.
	if(ps2WriteToMouse(0xF5) || ps2WriteToMouse(0xEB))
		return false;

	const unsigned length = mouseDeviceId == 0 ? 3 : 4;
	for(unsigned i = 0; i < length; ++i)
		if(!ps2ReadByte(true, mousePacketBuffer[i]))
			return false;
	if(!(mousePacketBuffer[0] & 8) || ps2WriteToMouse(0xF4))
		return false;

	mousePacketNumber = 0;
	ps2HandlePacket();
	return true;
}

void ps2CheckForData()
{
	g_mutex_acquire(packetLock);
	uint8_t status;
	while(((status = g_io_port_read_byte(G_PS2_STATUS_PORT)) & PS2_OUTPUT_FULL) != 0)
	{
		const uint8_t value = g_io_port_read_byte(G_PS2_DATA_PORT);
		if(status & PS2_ERROR)
		{
			if(status & PS2_AUXILIARY)
			{
				mousePacketNumber = 0;
				mouseResyncNeeded = true;
			}
			continue;
		}

		if(!(status & PS2_AUXILIARY))
			ps2QueueKeyboard(value);
		else if(!mouseRecoveryNeeded && !mouseResyncNeeded)
			ps2HandleMouseData(value);
	}

	// A reset ends after AA/00. Drain any already-buffered continuation first:
	// AA/00 followed by Y/wheel bytes is a valid right-button overflow packet.
	if(ps2ResetAnnouncement())
		mouseRecoveryNeeded = true;
	if((mouseRecoveryNeeded || mouseResyncNeeded) && g_millis() >= nextMouseRecovery)
	{
		const bool reset = mouseRecoveryNeeded;
		if(reset)
			ps2QueueMouse(0, 0, 0, 0, g_millis());
		const bool recovered = reset ?
			ps2InitializeMouse() == G_PS2_STATUS_SUCCESS : ps2ResynchronizeMouse();
		mousePacketNumber = 0;
		mouseRecoveryNeeded = !recovered;
		mouseResyncNeeded = false;
		if(!recovered)
		{
			if(!reset)
				ps2QueueMouse(0, 0, 0, 0, g_millis());
			nextMouseRecovery = g_millis() + 1000;
			klog("PS/2 mouse recovery failed; retrying in one second");
		}
	}

	const bool pending = eventCount || eventOverruns || mouseStateLost;
	g_mutex_release(packetLock);
	if(pending)
	{
		g_mutex_release(eventSignal);
		if(dispatchTask != G_TID_NONE)
			g_yield_t(dispatchTask);
	}
}

void ps2HandleMouseData(uint8_t value)
{
	// These timestamps measure when the task drains bytes, not their arrival
	// at the controller. Preemption between reads is not evidence of a lost
	// byte; retain the partial packet until framing or hardware errors say so.
	lastPacketTime = g_millis();

	if(mousePacketNumber == 0)
	{
		if(value & 0x08)
			mousePacketBuffer[mousePacketNumber++] = value;
		else
			mouseResyncNeeded = true;
		return;
	}

	mousePacketBuffer[mousePacketNumber++] = value;
	if(mousePacketNumber == (mouseDeviceId == 0 ? 3 : 4))
	{
		mousePacketNumber = 0;
		// After a byte slip, a header can land in the wheel field and delta
		// zeroes can retain a sign bit. This ambiguous shape needs an explicit
		// boundary check. An ordinary stale-sign zero without wheel data is
		// handled as zero motion below, without resetting the device.
		if(mouseDeviceId && mousePacketBuffer[1] == 0 && mousePacketBuffer[2] == 0 &&
		   (mousePacketBuffer[0] & 0x30) && (mousePacketBuffer[3] & 0xC8) == 0x08)
			mouseResyncNeeded = true;
		else
			ps2HandlePacket();
	}
}

static int16_t ps2DecodeMotion(uint8_t value, bool negative, bool overflow)
{
	if(overflow)
		return negative ? -255 : 255;
	// Some real mice and firmware emulators leave the sign bit set for zero.
	return value ? (int16_t) value - (negative ? 256 : 0) : 0;
}

static void ps2HandlePacket()
{
	const uint8_t flags = mousePacketBuffer[0];
	const int16_t x = ps2DecodeMotion(mousePacketBuffer[1], flags & 0x10, flags & 0x40);
	const int16_t y = ps2DecodeMotion(mousePacketBuffer[2], flags & 0x20, flags & 0x80);
	int8_t scroll = mouseDeviceId == 0 ? 0 : (int8_t) mousePacketBuffer[3];
	if(mouseDeviceId == 4)
		scroll = (mousePacketBuffer[3] & 0x0F) - ((mousePacketBuffer[3] & 0x08) ? 16 : 0);
	ps2QueueMouse(x, -y, flags, scroll, lastPacketTime);
}
