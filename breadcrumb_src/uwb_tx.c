#include "uwb_bread.h"


//-----------------dw1000----------------------------
static uint8_t bread_write_buf[MAP_PACKET_LENGTH];

void readRTTBuf(){//TODO we should check if 
    int len = SEGGER_RTT_Read(0, bread_write_buf, MAP_PACKET_LENGTH);
}

void writeUWBFromRTT(){
  dwt_forcetrxoff();
  dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
  dwt_writetxdata(MAP_PACKET_LENGTH, bread_write_buf, 0); /* Zero offset in TX buffer. */
  dwt_writetxfctrl(MAP_PACKET_LENGTH, 0, 0); /* Zero offset in TX buffer, ranging. */
   // __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "write tx\n");

  /* Start transmission, indicating that a response is expected so that reception is enabled automatically after the frame is sent and the delay
  * set by dwt_setrxaftertxdelay() has elapsed. */
    dwt_starttx(DWT_START_TX_IMMEDIATE);
    
    SEGGER_RTT_printf(0, "tx sent");

    interrupt_flag=false;
    deca_sleep(10);//this slight pause of 10 ms might not be needed
}

void relayUWB(){
  dwt_forcetrxoff();
  dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
  dwt_writetxdata(MAP_PACKET_LENGTH, rx_data_buffer, 0); /* Zero offset in TX buffer. */
  dwt_writetxfctrl(MAP_PACKET_LENGTH, 0, 0); /* Zero offset in TX buffer, ranging. */

    dwt_starttx(DWT_START_TX_IMMEDIATE);
    
    //SEGGER_RTT_printf(0, "relay sent");

    interrupt_flag=false;
    deca_sleep(10);//this slight pause of 10 ms might not be needed
}