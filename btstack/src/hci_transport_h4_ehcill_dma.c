/*
 * Copyright (C) 2009 by Matthias Ringwald
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
 *  hci_h4_transport_dma.c
 *
 *  HCI Transport implementation of H4 protocol with eHCILL support
 *  for IRQ-driven blockwise RX and TX, and IRQ callback on CTS toggle
 *
 *  Based on information found at http://e2e.ti.com/support/low_power_rf/f/660/t/134855.aspx
 *
 *  Created by Matthias Ringwald on 9/16/11.
 * 
 * Changed by Howard Su based on http://processors.wiki.ti.com/index.php/CC256x_eHCILL_Low_Power_Protocol
 * 
 */

#include "btstack-config.h"
//#define ENABLE_LOG_INFO
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "hci.h"
#include "hci_dump.h"
#include "hci_transport.h"
#include <btstack/run_loop.h>

#include <btstack/hal_uart_dma.h>

//#define DUMP

// eHCILL commands (+interal CTS signal)
#define EHCILL_GO_TO_SLEEP_IND 0x030
#define EHCILL_GO_TO_SLEEP_ACK 0x031
#define EHCILL_WAKE_UP_IND     0x032
#define EHCILL_WAKE_UP_ACK     0x033
#define EHCILL_CTS_SIGNAL      0x0EF

typedef enum {
    H4_W4_PACKET_TYPE = 1,
    H4_W4_EVENT_HEADER,
    H4_W4_ACL_HEADER,
    H4_W4_PAYLOAD,
    H4_PACKET_RECEIVED
} H4_STATE;

typedef enum {
    TX_IDLE = 1,
    TX_W4_WAKEUP,   // eHCILL only
    TX_W4_HEADER_SENT,
    TX_W4_PACKET_SENT,
    TX_W4_EHCILL_SENT,
} TX_STATE;

typedef enum {
    EHCILL_STATE_SLEEP = 1,
    EHCILL_STATE_W4_ACK, // wart for wakeup
    EHCILL_STATE_AWAKE
} EHCILL_STATE;

typedef struct hci_transport_h4 {
    hci_transport_t transport;
    data_source_t *ds;
} hci_transport_h4_t;

// prototypes
static int  h4_process(struct data_source *ds);
static void dummy_handler(uint8_t packet_type, uint8_t *packet, uint16_t size);
static void h4_block_received(void);
static void h4_block_sent(void);
static int  h4_open(void *transport_config);
static int  h4_close();
static void h4_register_packet_handler(void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size));
static const char * h4_get_transport_name();
static int  h4_set_baudrate(uint32_t baudrate);
static int  h4_can_send_packet_now(uint8_t packet_type);

static int  ehcill_send_packet(uint8_t packet_type, uint8_t *packet, int size);
static void ehcill_uart_dma_receive_block(uint8_t *buffer, uint16_t size);
static int  ehcill_sleep_mode_active(void);
static void ehcill_handle(uint8_t action);
static void ehcill_cts_irq_handler();

// eCHILL: state machine
static EHCILL_STATE ehcill_state;
static uint8_t *    ehcill_defer_rx_buffer;
static uint16_t     ehcill_defer_rx_size;
static uint8_t      ehcill_command_to_send;

// H4: packet reader state machine
static H4_STATE  h4_state;
static int       read_pos;
static int       bytes_to_read;
static uint8_t   hci_packet[HCI_PACKET_BUFFER_SIZE]; // bigger than largest packet
static  void (*packet_handler)(uint8_t packet_type, uint8_t *packet, uint16_t size) = dummy_handler;

static uint8_t justSent;

// H4: tx state
static TX_STATE  tx_state;
static uint8_t * tx_data;
static uint16_t  tx_len;
static uint8_t   tx_packet_type;

// data source used in run_loop
static data_source_t hci_transport_h4_dma_ds = {
    .process = h4_process
};

// hci_transport for use by hci
static const hci_transport_h4_t hci_transport_h4_ehcill_dma = {
    .ds                                      = &hci_transport_h4_dma_ds,
    .transport.open                          = h4_open,
    .transport.close                         = h4_close,
    .transport.send_packet                   = ehcill_send_packet,
    .transport.register_packet_handler       = h4_register_packet_handler,
    .transport.get_transport_name            = h4_get_transport_name,
    .transport.set_baudrate                  = h4_set_baudrate,
    .transport.can_send_packet_now           = h4_can_send_packet_now
};


static void dummy_handler(uint8_t packet_type, uint8_t *packet, uint16_t size){
}

static const char * h4_get_transport_name(){
    return "H4_EHCILL_DMA";
}

// get h4 singleton
hci_transport_t * hci_transport_h4_dma_instance() {
    return (hci_transport_t *) &hci_transport_h4_ehcill_dma;
}

static void h4_rx_init_sm(void){
    h4_state = H4_W4_PACKET_TYPE;
    ehcill_defer_rx_size = 0;
    read_pos = 0;
    bytes_to_read = 1;
    ehcill_uart_dma_receive_block(hci_packet, bytes_to_read);
}

static void h4_register_packet_handler(void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size)){
    packet_handler = handler;
}

static int h4_open(void *transport_config){

	// open uart
	hal_uart_dma_init();
    hal_uart_dma_set_block_received(h4_block_received);
    hal_uart_dma_set_block_sent(h4_block_sent);

	// set up data_source
    run_loop_add_data_source(&hci_transport_h4_dma_ds);

    // init state machiens
    ehcill_state = EHCILL_STATE_AWAKE;
    h4_rx_init_sm();
    tx_state = TX_IDLE;

    return 0;
}

static int h4_set_baudrate(uint32_t baudrate){
    log_info("h4_set_baudrate - set baud %lu\n", baudrate);
    return hal_uart_dma_set_baud(baudrate);
}

static int h4_close(){
    // first remove run loop handler
	run_loop_remove_data_source(&hci_transport_h4_dma_ds);

    // stop IRQ
    hal_uart_dma_set_csr_irq_handler(NULL);

    // close device
    hal_uart_dma_shutdown();
    return 0;
}

static void h4_block_received(void){

    read_pos += bytes_to_read;

    // act
    switch (h4_state) {
        case H4_W4_PACKET_TYPE:
            switch (hci_packet[0]) {
                case HCI_ACL_DATA_PACKET:
                    h4_state = H4_W4_ACL_HEADER;
                    bytes_to_read = HCI_ACL_HEADER_SIZE;
                    break;
                case HCI_EVENT_PACKET:
                    h4_state = H4_W4_EVENT_HEADER;
                    bytes_to_read = HCI_EVENT_HEADER_SIZE;
                    break;
                case EHCILL_GO_TO_SLEEP_IND:
                case EHCILL_WAKE_UP_IND:
                case EHCILL_WAKE_UP_ACK:
                case EHCILL_GO_TO_SLEEP_ACK:
                    ehcill_handle(hci_packet[0]);
                    read_pos = 0;
                    bytes_to_read = 1;
                    break;
                default:
                    log_error("h4_process: invalid packet type 0x%02x\n", hci_packet[0]);
                    read_pos = 0;
                    bytes_to_read = 1;
                    break;
            }
            break;

        case H4_W4_EVENT_HEADER:
            bytes_to_read = hci_packet[2];
            if (bytes_to_read) {
                h4_state = H4_W4_PAYLOAD;
                break;
            }
            h4_state = H4_PACKET_RECEIVED;
            break;

        case H4_W4_ACL_HEADER:
            bytes_to_read = READ_BT_16( hci_packet, 3);
            if (bytes_to_read) {
                h4_state = H4_W4_PAYLOAD;
                break;
            }
            h4_state = H4_PACKET_RECEIVED;
            break;

        case H4_W4_PAYLOAD:
            h4_state = H4_PACKET_RECEIVED;
            bytes_to_read = 0;

            // trigger run loop - necessary for use in low power modes
            embedded_trigger();
            break;

        default:
            bytes_to_read = 0;
            break;
    }


    // read next block
    if (bytes_to_read) {
        ehcill_uart_dma_receive_block(&hci_packet[read_pos], bytes_to_read);
    }
}

static int h4_can_send_packet_now(uint8_t packet_type){
    return tx_state == TX_IDLE;

}

static void h4_block_sent(void){
    switch (tx_state){
        case TX_W4_HEADER_SENT:
            tx_state = TX_W4_PACKET_SENT;
            // h4 packet type + actual packet
            hal_uart_dma_send_block(tx_data, tx_len);
            break;
        case TX_W4_PACKET_SENT:
            // send pending ehcill command
            if (ehcill_command_to_send){
                tx_state = TX_W4_EHCILL_SENT;
                hal_uart_dma_send_block(&ehcill_command_to_send, 1);
                break;
            }
            tx_state = TX_IDLE;
            justSent = 1;
            // trigger run loop
            embedded_trigger();
            break;
        case TX_W4_EHCILL_SENT:
            if (ehcill_command_to_send == EHCILL_GO_TO_SLEEP_ACK) {
                // UART not needed after EHCILL_GO_TO_SLEEP_ACK was sent
                hal_uart_dma_set_sleep(1);
            }
            ehcill_command_to_send = 0;
            tx_state = TX_IDLE;
            justSent = 1;
            // trigger run loop
            embedded_trigger();
            break;
        default:
            break;
    }
}

#ifdef DUMP
static void dump(uint8_t *data, uint16_t len){
    int i;
    if (len > 5) len = 8;
    for (i=0; i<len;i++){
        printf("%02X ", ((uint8_t *)data)[i]);
    }
    printf("\n\r");
}
#endif

static int h4_process(struct data_source *ds) {

    // notify about packet sent
    if (justSent){
        // reset state
        justSent = 0;
        uint8_t event[] = {DAEMON_EVENT_HCI_PACKET_SENT, 0};
        packet_handler(HCI_EVENT_PACKET, &event[0], sizeof(event));
    }

    if (h4_state != H4_PACKET_RECEIVED) return 0;

    // log packet
#ifdef DUMP
    printf("RX: ");
    dump(hci_packet, read_pos);
#endif

    packet_handler(hci_packet[0], &hci_packet[1], read_pos-1);

    h4_rx_init_sm();

    return 0;
}

//////////////////////////


int  ehcill_sleep_mode_active(void){
    return ehcill_state != EHCILL_STATE_AWAKE;
}

static void ehcill_cts_irq_handler(){
    ehcill_handle(EHCILL_CTS_SIGNAL);
}

static void ehcill_schedule_ecill_command(uint8_t command){
    // reply with HCILL_SLEEP_ACK
    ehcill_command_to_send = command;
    switch (tx_state){
        case TX_IDLE:
            tx_state = TX_W4_EHCILL_SENT;
            switch(command)
            {
                case EHCILL_WAKE_UP_ACK:
                    log_info("TX: EHCILL_WAKE_UP_ACK\n");
                    break;
                case EHCILL_GO_TO_SLEEP_ACK:
                    log_info("TX: EHCILL_GO_TO_SLEEP_ACK\n");
                    break;
                case EHCILL_GO_TO_SLEEP_IND:
                    log_info("TX: EHCILL_GO_TO_SLEEP_ACK\n");
                    break;
                case EHCILL_WAKE_UP_IND:
                    log_info("TX: EHCILL_WAKE_UP_IND\n");
                    break;
            }
            
            hal_uart_dma_send_block(&ehcill_command_to_send, 1);
            break;
        default:
            break;
    }
}

static void ehcill_handle(uint8_t action){
    int size;

    // printf("ehcill_handle: %x, state %u, defer_rx %u \n\r", action, ehcill_state, ehcill_defer_rx_size);
    switch(ehcill_state){
        case EHCILL_STATE_AWAKE:
            switch(action){
                case EHCILL_GO_TO_SLEEP_IND:

                    // 1. set RTS high - already done by BT RX ISR
                    // 2. enable CTS
					hal_uart_dma_set_csr_irq_handler(ehcill_cts_irq_handler);

                    ehcill_state = EHCILL_STATE_SLEEP;
                    log_info("RX: EHCILL_GO_TO_SLEEP_IND\n");
                    ehcill_schedule_ecill_command(EHCILL_GO_TO_SLEEP_ACK);
                    break;

                case EHCILL_WAKE_UP_IND:
                    // we can accept a WAKEUP_IND even we alrady in wake up state?
                    log_info("RX: EHCILL_WAKE_UP_IND 1 TX_STATE = %d\n", tx_state);
                    ehcill_schedule_ecill_command(EHCILL_WAKE_UP_ACK);
                    break;

                default:
                    log_error("1: unknow action: %d\n", action);
                    break;
            }
            break;

        case EHCILL_STATE_SLEEP:
            switch(action){
                case EHCILL_CTS_SIGNAL:

                    // re-activate rx (also clears RTS)
					hal_uart_dma_set_csr_irq_handler(NULL);

					if (!ehcill_defer_rx_size) break;

                    // UART needed again
					hal_uart_dma_set_sleep(0);

                    log_info ("RX: EHCILL_CTS_SIGNAL\n");
                    size = ehcill_defer_rx_size;
                    ehcill_defer_rx_size = 0;
                    hal_uart_dma_receive_block(ehcill_defer_rx_buffer, size);
                    break;

                case EHCILL_WAKE_UP_IND:

                    ehcill_state = EHCILL_STATE_AWAKE;
                    log_info("RX: EHCILL_WAKE_UP_IND 2\n");
                    ehcill_schedule_ecill_command(EHCILL_WAKE_UP_ACK);
                    break;

                default:
                    log_error("2: unknow action: %d\n", action);
                    break;
            }
            break;

        case EHCILL_STATE_W4_ACK:
            switch(action){
                case EHCILL_WAKE_UP_IND:
                case EHCILL_WAKE_UP_ACK:
                    log_info("RX: EHCILL_WAKE_UP_IND or ACK\n");

                    tx_state = TX_W4_HEADER_SENT;
                    hal_uart_dma_send_block(&tx_packet_type, 1);
                    ehcill_state = EHCILL_STATE_AWAKE;

                    break;
                case EHCILL_GO_TO_SLEEP_IND:
                    // ignore this
                    // keep wait for wakeup ack
                    break;                
                default:
                    log_error("3: unknow action: %d\n", action);
                    break;
            }
            break;
    }
}

static int ehcill_send_packet(uint8_t packet_type, uint8_t *packet, int size){

    // write in progress
    if (tx_state != TX_IDLE) {
        log_error("h4_send_packet with tx_state = %u, type %u, data %02x %02x %02x\n", tx_state, packet_type, packet[0], packet[1], packet[2]);
        return -1;
    }

#ifdef DUMP
    printf("TX: %02x ", packet_type);
    dump(packet, size);
#endif

    tx_packet_type = packet_type;
    tx_data = packet;
    tx_len  = size;

    if (!ehcill_sleep_mode_active()){
        tx_state = TX_W4_HEADER_SENT;
        hal_uart_dma_send_block(&tx_packet_type, 1);
        return 0;
    }

    // disable CTS
    hal_uart_dma_set_csr_irq_handler(NULL);

    // UART needed again
    hal_uart_dma_set_sleep(0);

    // update state
    tx_state     = TX_W4_WAKEUP;
    ehcill_state = EHCILL_STATE_W4_ACK;

    // wake up
    log_info("RX: SLEEP\n");

    if (!ehcill_defer_rx_size){
        log_error("ERROR: NO RX REQUEST PENDING\n");
        return 0;
    }

    // receive request, clears RTS
    int rx_size = ehcill_defer_rx_size;
    ehcill_defer_rx_size = 0;
    hal_uart_dma_receive_block(ehcill_defer_rx_buffer, rx_size);

    log_info("TX: EHCILL_WAKE_UP_IND\n");
    ehcill_command_to_send = EHCILL_WAKE_UP_IND;
    hal_uart_dma_send_block(&ehcill_command_to_send, 1);

    return 0;
}

void ehcill_uart_dma_receive_block(uint8_t *buffer, uint16_t size){
    if (!ehcill_sleep_mode_active()){
        ehcill_defer_rx_size = 0;
        hal_uart_dma_receive_block(buffer, size);
        return;
    }

    // store receive request for later
    ehcill_defer_rx_buffer = buffer;
    ehcill_defer_rx_size   = size;
}
