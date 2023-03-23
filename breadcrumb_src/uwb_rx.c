#include "uwb_bread.h"

//main loop for listening to UWB radio and rx response

volatile uint8_t rx_data_buffer[MAP_PACKET_LENGTH];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

void rxUWB(){

  // Activate reception immediately. 
  dwt_rxenable(DWT_START_RX_IMMEDIATE);

  // Poll for reception of a frame or error/timeout. See NOTE 5 below. 
  while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
  {
    if(interrupt_flag){
      return;
    }
  };

    #if 0	  // Include to determine the type of timeout if required.
    int temp = 0;
    // (frame wait timeout and preamble detect timeout)
    if(status_reg & SYS_STATUS_RXRFTO )
    temp =1;
    else if(status_reg & SYS_STATUS_RXPTO )
    temp =2;
    #endif

  if (status_reg & SYS_STATUS_RXFCG)
  {

    uint32 frame_len;

    /* Clear good RX frame event in the DW1000 status register. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);

    /* A frame has been received, read it into the local buffer. */
    frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
    
    if (frame_len <= RX_BUFFER_LEN)
    {
      dwt_readrxdata(rx_data_buffer, frame_len, 0);
    }

    for(int i=0;i<frame_len;i++){//hope this works and doesn't take forever
        SEGGER_RTT_printf(0, "%x, ",rx_data_buffer[i]);
    }
    SEGGER_RTT_printf(0, "\n");

    relayUWB();
  }
    else
  {
    /* Clear RX error/timeout events in the DW1000 status register. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);

    /* Reset RX to properly reinitialise LDE operation. */
    dwt_rxreset();
  }

}
