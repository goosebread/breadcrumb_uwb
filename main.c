/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**@file
 * @defgroup nrf_dev_simple_timer_example_main.c
 * @{
 * @ingroup nrf_dev_simple_timer_example
 * @brief Timer example application main file.
 *
 * This file contains the source code for a sample application using timer library.
 * For a more detailed description of the functionality, see the SDK documentation.
 */

#include <stdio.h>
#include "boards.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "flash_util.h"

#include "rtt_input.h"
#include "SEGGER_RTT.h"

#include "app_scheduler.h"
#include "uwb_bread.h"

#define RTT_INPUT_POLL_PERIOD_MS 20

#define SCHED_MAX_EVENT_DATA_SIZE   sizeof(int)
#define SCHED_QUEUE_SIZE            10

//inline void led_write(uint32_t led_idx, bool val);
void case1fun(void);

/**@brief Function for the Power Management. useful if we are interested in low energy
 */
static void power_manage(void)
{
    // Wait for event.
    __WFE();
    // Clear Event Register.
    __SEV();
    __WFE();
}

static inline void led_write(uint32_t led_idx, bool val){
    if(val){bsp_board_led_on(led_idx);}
    else{bsp_board_led_off(led_idx);}
}

//WARNING we will need to change how they are stored if preamble channel needs
//more than a 2 bits
void storeChannelSettings(){
    uint32_t val2 = pcitx<<2;
    uint32_t val = 0xFFFFFFF0+(pcitx<<2)+pcirx;
    writeWordFlash(val);
}

void loadChannelSettings(){
    uint32_t val = readWordFlash();
    pcitx = (val&0x0000000C)>>2;
    pcirx = (val&0x00000003);
    led_write(BSP_BOARD_LED_3,pcitx & 0x2);
    led_write(BSP_BOARD_LED_2,pcitx & 0x1);
    led_write(BSP_BOARD_LED_1,pcirx & 0x2);
    led_write(BSP_BOARD_LED_0,pcirx & 0x1);
    changePreambleCode(preambleCodeList[pcitx], preambleCodeList[pcirx]);
}

void case1fun(){
            pcitx = (pcitx+1)% sizeof(preambleCodeList);
            uint8_t np = preambleCodeList[pcitx];
            changePreambleCode(np, preambleCodeList[pcirx]);
            led_write(BSP_BOARD_LED_3,pcitx & 0x2);
            led_write(BSP_BOARD_LED_2,pcitx & 0x1);
            SEGGER_RTT_printf(0, "new tx preamble %d \n", np);
            storeChannelSettings();
            interrupt_flag = false;
}
void case3fun(){
            pcirx = (pcirx+1)% sizeof(preambleCodeList);
            uint8_t np = preambleCodeList[pcirx];
            changePreambleCode(preambleCodeList[pcitx],np);
            led_write(BSP_BOARD_LED_1,pcirx & 0x2);
            led_write(BSP_BOARD_LED_0,pcirx & 0x1);
            SEGGER_RTT_printf(0, "new rx preamble %d \n", np);
            storeChannelSettings();
            interrupt_flag = false;
}


static void button_event_handler(uint32_t button_number)
{
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Button %u pressed\n", button_number);
    switch (button_number)
    {
        case 1:
        {
            //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "User action \n");
            app_sched_event_put(NULL, 0, case1fun);
            interrupt_flag = true;

            break;
        }
        
        case 2:
        {
            //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "button 2 trigger message send \n");
            readRTTBuf();
            app_sched_event_put(NULL, 0, writeUWBFromRTT);
            interrupt_flag = true;
            break;
        }
        case 3:
        {
            //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Run RYE UWB \n");
            app_sched_event_put(NULL, 0, case3fun);
            interrupt_flag = true;
            break;
        }

        case 4:
        {   
            SEGGER_RTT_printf(0, "Run Ranging\n");
            app_sched_event_put(NULL, 0, doRanging);
            interrupt_flag = true;
            break;
        }

        default:
            SEGGER_RTT_printf(0, "Invalid Key\n");
            break;
    }
}

//external rtt input. These events can just get scheduled accordingly
static void app_rtt_input_handler(int key)
{
    if (key >= '1' && key <= '4')
    {
        uint32_t button_number = key - '0';
        SEGGER_RTT_printf(0, "Key %d\n", button_number);
        button_event_handler(button_number);
    }
    else
    {
        SEGGER_RTT_printf(0, "Invalid Key\n");
    }
}

static void initialize(void)
{
    SEGGER_RTT_printf(0, "Breadcrumb :)\n");
    //__LOG_INIT(LOG_SRC_APP | LOG_SRC_FRIEND, LOG_LEVEL_DBG1, LOG_CALLBACK_DEFAULT);
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- BLE Mesh Light Switch Server Demo -----\n");

    //use app scheduler to get around nested interrupt problem
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);

    //hal_leds_init();
    bsp_board_init(BSP_INIT_LEDS);

    breadcrumb_dwm_init();
    loadChannelSettings();
}

static void start(void)
{  
    //listen to rtt connection
    rtt_input_enable(app_rtt_input_handler, RTT_INPUT_POLL_PERIOD_MS);
}

int main(void) 
{
    initialize();
    start();
    for (;;)
    {
        //power_manage();
        app_sched_execute();//do scheduled tasks
        rxUWB(); //something in here messes up actual transmission
    }
}


