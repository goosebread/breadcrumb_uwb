/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Model handler
 */

#ifndef UWB_BREAD_H__
#define UWB_BREAD_H__

#include <stdint.h>

#include "port_platform.h"
#include "deca_types.h"
#include "deca_param_types.h"
#include "deca_regs.h"
#include "deca_device_api.h"

#include "SEGGER_RTT.h"

//longest length available in UWB standard
#define MAP_PACKET_LENGTH 127

extern volatile bool interrupt_flag;
extern volatile uint8_t rx_data_buffer[MAP_PACKET_LENGTH];

//maybe also global buffer index for router queue buffer

/* Preamble timeout, in multiple of PAC size. See NOTE 3 below. */
#define PRE_TIMEOUT 1000

/* Delay between frames, in UWB microseconds. See NOTE 1 below. */
#define POLL_TX_TO_RESP_RX_DLY_UUS 100 

#define APP_NAME "SS TWR INIT v1.3"

/* Inter-ranging delay period, in milliseconds. */
#define RNG_DELAY_MS 500


/* Length of the common part of the message (up to and including the function code, see NOTE 1 below). */
#define ALL_MSG_COMMON_LEN 10
/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_SN_IDX 2
#define RESP_MSG_POLL_RX_TS_IDX 10
#define RESP_MSG_RESP_TX_TS_IDX 14
#define RESP_MSG_TS_LEN 4
#define DEST_ADDR_1 5
#define DEST_ADDR_2 6
#define SOURCE_ADDR_1 7
#define SOURCE_ADDR_2 8


/* Buffer to store received response message.
* Its size is adjusted to longest frame that this example code is supposed to handle. */
#define RX_BUF_LEN 20

/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) conversion factor.
* 1 uus = 512 / 499.2 s and 1 s = 499.2 * 128 dtu. */
#define UUS_TO_DWT_TIME 65536

// Not enough time to write the data so TX timeout extended for nRF operation.
// Might be able to get away with 800 uSec but would have to test
// See note 6 at the end of this file
#define POLL_RX_TO_RESP_TX_DLY_UUS  1100

/* This is the delay from the end of the frame transmission to the enable of the receiver, as programmed for the DW1000's wait for response feature. */
#define RESP_TX_TO_FINAL_RX_DLY_UUS 500

/* Speed of light in air, in metres per second. */
#define SPEED_OF_LIGHT 299702547

void readRTTBuf(void);
void writeUWBFromRTT(void);
void relayUWB(void);

void changePreambleCode(uint8_t tx_code,uint8_t rx_code);
void breadcrumb_dwm_init(void);

void rxUWB(void);

#endif /* UWB_BREAD_H__ */