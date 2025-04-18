/*
 * Copyright (C) 2011 by Matthias Ringwald
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MATTHIAS RINGWALD AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 *  bt_control_cc256x.c
 *
 *  Adapter to use cc256x-based chipsets with BTstack
 *
 *  Handles init script (a.k.a. Service Patch)
 *  Allows for non-standard UART baud rate
 *  Allows to configure transmit power
 *  Allows to activate eHCILL deep sleep mode
 *
 *  Issues with mspgcc LTS:
 *  - 20 bit support is not there yet -> .text cannot get bigger than 48 kb
 *  - arrays cannot have more than 32k entries
 *
 *  workarounds:
 *  - store init script in .fartext and use assembly code to read from there
 *  - split into two arrays
 *
 */

#include "bt_control_cc256x.h"

#include <stddef.h>   /* NULL */
#include <stdio.h>
#include <string.h>   /* memcpy */

#if defined(__GNUC__) && (__MSP430X__ > 0)
#include "hal_compat.h"
#define __data20
#endif

#if defined(__GNUC__) && defined(EFM32GG395F1024)
#define __data20
#endif

#include "bt_control.h"


// actual init script provided by seperate .c file
extern const uint8_t  __data20 cc256x_init_script[];
extern const uint32_t cc256x_init_script_size;

//
static uint32_t init_script_offset  = 0;
static int16_t  init_power_in_dB    = 0; // 13 dBm
static int      init_ehcill_enabled = 1;


static int bt_control_cc256x_on(void *config){
	init_script_offset = 0;
	return 0;
}

static void bt_control_cc256x_hw_error(void)
{
    printf("hardware error\n");
    return;
}

// UART Baud Rate control from: http://e2e.ti.com/support/low_power_rf/f/660/p/134850/484763.aspx
static int cc256x_baudrate_cmd(void * config, uint32_t baudrate, uint8_t *hci_cmd_buffer){
    hci_cmd_buffer[0] = 0x36;
    hci_cmd_buffer[1] = 0xFF;
    hci_cmd_buffer[2] = 0x04;
    hci_cmd_buffer[3] =  baudrate        & 0xff;
    hci_cmd_buffer[4] = (baudrate >>  8) & 0xff;
    hci_cmd_buffer[5] = (baudrate >> 16) & 0xff;
    hci_cmd_buffer[6] = 0;
    return 0;
}


// Output Power control from: http://e2e.ti.com/support/low_power_rf/f/660/p/134853/484767.aspx
#define NUM_POWER_LEVELS 16
#define DB_MIN_LEVEL -35
#define DB_PER_LEVEL 5
#define DB_DYNAMIC_RANGE 30

static int get_max_power_for_modulation_type(int type){
    // a) limit max output power
    int power_db;
    switch (type){
        case 0:     // GFSK
            power_db = 12;
            break;
        default:    // EDRx
            power_db = 10;
            break;
    }
    if (power_db > init_power_in_dB) {
        power_db = init_power_in_dB;
    }
    return power_db;
}

static int get_highest_level_for_given_power(int power_db, int recommended_db){
    int i = NUM_POWER_LEVELS-1;
    while (i) {
        if (power_db < recommended_db) {
            return i;
        }
        power_db -= DB_PER_LEVEL;
        i--;
    }
    return 0;
}

static void update_set_power_vector(uint8_t *hci_cmd_buffer){
    int i;
    int power_db = get_max_power_for_modulation_type(hci_cmd_buffer[3]);
    int dynamic_range = 0;
    // f) don't touch level 0
    for ( i = (NUM_POWER_LEVELS-1) ; i >= 1 ; i--){
        hci_cmd_buffer[4+i] = 2 * power_db;

        if (dynamic_range + DB_PER_LEVEL > DB_DYNAMIC_RANGE) continue;  // e)

        power_db      -= DB_PER_LEVEL;   // d)
        dynamic_range += DB_PER_LEVEL;

        if (power_db > DB_MIN_LEVEL) continue;

        power_db = DB_MIN_LEVEL;    // b)
    }
}

static void update_set_class2_single_power(uint8_t * hci_cmd_buffer){
    int i = 0;
    for (i=0;i<3;i++){
        hci_cmd_buffer[3+i] = get_highest_level_for_given_power(get_max_power_for_modulation_type(i), 4);
    }
}

// eHCILL activate from http://e2e.ti.com/support/low_power_rf/f/660/p/134855/484776.aspx
static void update_sleep_mode_configurations(uint8_t * hci_cmd_buffer){
    if (init_ehcill_enabled) {
        hci_cmd_buffer[4] = 1;
    } else {
        hci_cmd_buffer[4] = 0;
    }
}

static void bt_control_cc256x_update_command(uint8_t *hci_cmd_buffer){

    uint16_t opcode = hci_cmd_buffer[0] | (hci_cmd_buffer[1] << 8);

    switch (opcode){
    case 0xFD87:
            update_set_class2_single_power(hci_cmd_buffer);
            break;
        case 0xFD82:
            update_set_power_vector(hci_cmd_buffer);
            break;
        case 0xFD0C:
            update_sleep_mode_configurations(hci_cmd_buffer);
            break;
        default:
            break;
    }
}

static int bt_control_cc256x_next_cmd(void *config, uint8_t *hci_cmd_buffer){

    if (init_script_offset >= cc256x_init_script_size) {
        return 0;
    }

    init_script_offset++;   // extracted init script has 0x01 cmd packet type, but BTstack expects them without

#if defined(__GNUC__) && (__MSP430X__ > 0)

    // workaround: use FlashReadBlock with 32-bit integer and assume init script starts at 0x10000
    uint32_t init_script_addr = 0x10000;
    FlashReadBlock(&hci_cmd_buffer[0], init_script_addr + init_script_offset, 3);  // cmd header
    init_script_offset += 3;
    int payload_len = hci_cmd_buffer[2];
    FlashReadBlock(&hci_cmd_buffer[3], init_script_addr + init_script_offset, payload_len);  // cmd payload

#else
    // use memcpy with pointer
    uint8_t * init_script_ptr = (uint8_t*) &cc256x_init_script[0];
    memcpy(&hci_cmd_buffer[0], init_script_ptr + init_script_offset, 3);  // cmd header
    init_script_offset += 3;
    int payload_len = hci_cmd_buffer[2];
    memcpy(&hci_cmd_buffer[3], init_script_ptr + init_script_offset, payload_len);  // cmd payload
#endif

    // support for cc256x power commands and ehcill
    bt_control_cc256x_update_command(hci_cmd_buffer);

    init_script_offset += payload_len;

    return 1;
}

// MARK: const structs

static const bt_control_t bt_control_cc256x = {
	.on = bt_control_cc256x_on,
    .next_cmd = bt_control_cc256x_next_cmd,
    .baudrate_cmd = cc256x_baudrate_cmd,
    .hw_error = bt_control_cc256x_hw_error
};

static const hci_uart_config_t hci_uart_config_cc256x = {
    .baudrate_init = 115200,
    .baudrate_main = 0
};

// MARK: public API

void bt_control_cc256x_enable_ehcill(int on){
    init_ehcill_enabled = on;
}

int bt_control_cc256x_ehcill_enabled(void){
    return init_ehcill_enabled;
}
void bt_control_cc256x_set_power(int16_t power_in_dB){
    init_power_in_dB = power_in_dB;
}

bt_control_t *bt_control_cc256x_instance(void){
    return (bt_control_t*) &bt_control_cc256x;
}

hci_uart_config_t *hci_uart_config_cc256x_instance(void){
    return (hci_uart_config_t*) &hci_uart_config_cc256x;
}
