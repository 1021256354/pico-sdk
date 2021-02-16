/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico.h"
#include "pico/time.h"
#include "pico/bootrom.h"

// PICO_CONFIG: PICO_BOOTSEL_VIA_DOUBLE_RESET_TIMEOUT_MS, Window of opportunity for a second press of a reset button to enter BOOTSEL mode (milliseconds), type=int, default=200, group=pico_bootsel_via_double_reset
#ifndef PICO_BOOTSEL_VIA_DOUBLE_RESET_TIMEOUT_MS
#define PICO_BOOTSEL_VIA_DOUBLE_RESET_TIMEOUT_MS 200
#endif

// PICO_CONFIG: PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED, GPIO to use as bootloader activity LED when BOOTSEL mode is entered via reset double tap (or -1 for none), type=int, default=-1, group=pico_bootsel_via_double_reset
#ifndef PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED
#define PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED -1
#endif

// PICO_CONFIG: PICO_BOOTSEL_VIA_DOUBLE_RESET_INTERFACE_DISABLE_MASK, Optionally disable either the mass storage interface (bit 0) or the PICOBOOT interface (bit 1) when entering BOOTSEL mode via double reset, type=uint32_t, default=0, group=pico_bootsel_via_double_reset
#ifndef PICO_BOOTSEL_VIA_DOUBLE_RESET_INTERFACE_DISABLE_MASK
#define PICO_BOOTSEL_VIA_DOUBLE_RESET_INTERFACE_DISABLE_MASK 0u
#endif

/** \defgroup pico_bootsel_via_double_reset
 *
 * When the 'pico_bootsel_via_double_reset' library is linked, a function is
 * injected before main() which will detect when the system has been reset
 * twice in quick succession, and enter the USB ROM bootloader (BOOTSEL mode)
 * when this happens. This allows a double tap of a reset button on a
 * development board to be used to enter the ROM bootloader, provided this
 * library is always linked.
 */

// Doesn't make any sense for a RAM only binary
#if !PICO_NO_FLASH
static const uint32_t magic_token[] = {
        0xf01681de, 0xbd729b29, 0xd359be7a,
};

static uint32_t __uninitialized_ram(magic_location)[count_of(magic_token)];

/*! \brief Check for double reset and enter BOOTSEL mode if detected
 *  \ingroup pico_bootsel_via_double_reset
 *
 * This function is registered to run automatically before main(). The
 * algorithm is:
 *
 *   1. Check for magic token in memory; enter BOOTSEL mode if found.
 *   2. Initialise that memory with that magic token.
 *   3. Do nothing for a short while (few hundred ms).
 *   4. Clear the magic token.
 *   5. Continue with normal boot.
 *
 * Resetting the device twice quickly will interrupt step 3, leaving the token
 * in place so that the second boot will go to the bootloader.
 */
static void __attribute__((constructor)) boot_double_tap_check(void) {
    for (uint i = 0; i < count_of(magic_token); i++) {
        if (magic_location[i] != magic_token[i]) {
            // Arm, wait, then disarm and continue booting
            for (i = 0; i < count_of(magic_token); i++) {
                magic_location[i] = magic_token[i];
            }
            busy_wait_us(PICO_BOOTSEL_VIA_DOUBLE_RESET_TIMEOUT_MS * 1000);
            magic_location[0] = 0;
            return;
        }
    }
    // Detected a double reset, so enter USB bootloader
    magic_location[0] = 0;
    uint32_t led_mask = PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED >= 0 ?
        1u << PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED : 0u;
    reset_usb_boot(
        led_mask,
        PICO_BOOTSEL_VIA_DOUBLE_RESET_INTERFACE_DISABLE_MASK
    );
}

#endif
