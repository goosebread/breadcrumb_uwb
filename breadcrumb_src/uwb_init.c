#include "uwb_bread.h"

volatile bool interrupt_flag;

//pretty sure this is the same as tx. need to clean up
static dwt_config_t config = {
    5,                /* Channel number. */
    DWT_PRF_64M,      /* Pulse repetition frequency. */
    DWT_PLEN_128,   //DWT_PLEN_128,     /* Preamble length. Used in TX only. */
    DWT_PAC8,         /* Preamble acquisition chunk size. Used in RX only. */
    11,               /* TX preamble code. Used in TX only. */
    11,               /* RX preamble code. Used in RX only. */
    0,                /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_6M8,       /* Data rate. */
    DWT_PHRMODE_STD, //  /* PHY header mode. */
    (129 + 8 - 8)     /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};

//could probably optimize to not rely on a dwt read
void changePreambleCode(uint8_t tx_code,uint8_t rx_code){
    dwt_forcetrxoff();
    uint32_t regval =  dwt_read32bitreg(CHAN_CTRL_ID)| // Use DW nsSFD
              (CHAN_CTRL_TX_PCOD_MASK & (tx_code << CHAN_CTRL_TX_PCOD_SHIFT)) | // TX Preamble Code
              (CHAN_CTRL_RX_PCOD_MASK & (rx_code << CHAN_CTRL_RX_PCOD_SHIFT)) ; // RX Preamble Code

    dwt_write32bitreg(CHAN_CTRL_ID,regval) ;
}

void breadcrumb_dwm_init(){

  /* Setup DW1000 IRQ pin */
  nrf_gpio_cfg_input(DW1000_IRQ, NRF_GPIO_PIN_NOPULL); 		//irq

  /* Reset DW1000 */
  reset_DW1000(); 

  /* Set SPI clock to 2MHz */
  port_set_dw1000_slowrate();			

  /* Init the DW1000 */
  if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR)
  {
    //Init of DW1000 Failed
    while (1)
    {};
  }

  // Set SPI to 8MHz clock
  port_set_dw1000_fastrate();  

  /* Configure DW1000. */
  dwt_configure(&config);

  /* Apply default antenna delay value. Defined in port platform.h */
  dwt_setrxantennadelay(RX_ANT_DLY);
  dwt_settxantennadelay(TX_ANT_DLY);

  /* Set preamble timeout for expected frames.  */
  //dwt_setpreambledetecttimeout(PRE_TIMEOUT);

  dwt_setrxtimeout(0);    // set to NO receive timeout for this simple example   
  interrupt_flag=false;
}
  //-------------dw1000  ini------end---------------------------