/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __KERNEL_HPET__
#define __KERNEL_HPET__

#include "kernel/system/acpi/acpi.hpp"
#include <ghost/stdint.h>

/**
 * High-precision event timer
 */
struct g_acpi_hpet
{
    g_acpi_table_header header;
    uint8_t hardwareRevId;
    uint8_t comparatorCount : 5;
    uint8_t counterSize : 1;
    uint8_t resreved : 1;
    uint8_t legacyReplacement : 1;
    uint16_t pciVendorId;
    g_acpi_gas baseAddress;
    uint8_t hpetNumber;
    uint16_t minTick;
    uint8_t pageProtection;
} __attribute__((packed));

// General Capabilities and Configuration Registers
#define HPET_GEN_CAP_REG        0x00
#define HPET_GEN_CONFIG_REG     0x10

// Main Counter
#define HPET_MAIN_COUNTER_REG   0xF0

// Timer Registers
#define HPET_TIMER0_CONFIG_REG  0x100
#define HPET_TIMER1_CONFIG_REG  0x110
#define HPET_TIMER2_CONFIG_REG  0x120
#define HPET_TIMER3_CONFIG_REG  0x130

// Interrupt Status and Enable Registers
#define HPET_IRQ_STATUS_REG     0x80
#define HPET_IRQ_ENABLE_REG     0x90

// Tick Period
#define HPET_MIN_TICK_REG       0x3C

#define HPET_DEFAULT_FREQUENCY  14318180

/**
 * Attempts to detect and initialize the High-Precision Event Timer (HPET).
 */
void hpetInitialize();

/**
 * @return whether the HPET is available
 */
bool hpetIsAvailable();

/**
 * @return the nano time
 */
uint64_t hpetGetNanos();

#endif
