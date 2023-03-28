#include "uwb_bread.h"

//ranging code here





//-----------------dw1000----------------------------


//start of ss_init_main.c

/* Frames used in the ranging process. See NOTE 1,2 below. */
static uint8 tx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'P', 'U', 'R', 'Y', 0xE0, 0, 0};
static uint8 rx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'R', 'Y', 'P', 'U', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static uint8 rx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'P', 'U', 'R', 'Y', 0xE0, 0, 0};
static uint8 tx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'R', 'Y', 'P', 'U', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Frame sequence number, incremented after each transmission to a specific node */
static uint8 frame_seq_nb = 0;
static uint8 frame_seq_nb_1 = 0;
static uint8 frame_seq_nb_2 = 0;
static uint8 frame_seq_nb_3 = 0;
static uint8 frame_seq_nb_4 = 0;
static uint8 frame_seq_nb_5 = 0;
static uint8 frame_seq_nb_6 = 0;
static uint8 frame_seq_nb_7 = 0;
static uint8 frame_seq_nb_8 = 0;
static uint8 frame_seq_nb_9 = 0;
static uint8 frame_seq_nb_10 = 0;
static uint8 frame_seq_nb_11 = 0;
static uint8 frame_seq_nb_12 = 0;

static uint8 rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;


/* Hold copies of computed time of flight and distance here for reference so that it can be examined at a debug breakpoint. */
static double tof;
static double distance;
static double distance_uncal;
static double dist_scale = 0.951273;
static double dist_offset = 0.34282;

/* Declaration of static functions. */
static void resp_msg_get_ts(uint8_t *ts_field, uint32_t *ts);
int target_node_select(int target_node);
int frame_incr(int target_node);
void print_distance(int target_node);

/*Transactions Counters */
static volatile int tx_count = 0 ; // Successful transmit counter
static volatile int rx_count = 0 ; // Successful receive counter 
static int target_node = 0; //Target node selection index


void rangeRequest(void);

static void resp_msg_get_ts(uint8_t *ts_field, uint32_t *ts);
//void ss_initiator_task_function (void * pvParameter);
int target_node_select (int target_node);
int frame_incr(int target_node);
void print_distance(int target_node);


/* Timestamps of frames transmission/reception.
* As they are 40-bit wide, we need to define a 64-bit int type to handle them. */
typedef unsigned long long uint64;
static uint64 poll_rx_ts;
static uint64 resp_tx_ts;
static uint8 rx_frame_seq_nb = 0;
static void resp_msg_set_ts(uint8 *ts_field, const uint64 ts);
static uint64 get_rx_timestamp_u64();
/*! ------------------------------------------------------------------------------------------------------------------
* @fn main()
*
* @brief Application entry point.
*
* @param  none
*
* @return none
*/

//in progress
void handleRxRanging(){
    SEGGER_RTT_printf(0, "ranging request received \n");
    changePreambleCode(preambleCodeList[pcirx], preambleCodeList[pcirx]);

    /* Check that the frame is a poll sent by "SS TWR initiator" example.
     * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
    rx_data_buffer[ALL_MSG_SN_IDX] = 0;
    if (memcmp(rx_data_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN) == 0) {
        // start  here

        uint32 resp_tx_time;
        int ret;

        /* Retrieve poll reception timestamp. */
        poll_rx_ts = get_rx_timestamp_u64();

        /* Compute final message transmission time. See NOTE 7 below. */
        resp_tx_time = (poll_rx_ts + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
        dwt_setdelayedtrxtime(resp_tx_time);

        /* Response TX timestamp is the transmission time we programmed plus the antenna delay. */
        resp_tx_ts = (((uint64)(resp_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

        /* Write all timestamps in the final message. See NOTE 8 below. */
        resp_msg_set_ts(&tx_resp_msg[RESP_MSG_POLL_RX_TS_IDX], poll_rx_ts);
        resp_msg_set_ts(&tx_resp_msg[RESP_MSG_RESP_TX_TS_IDX], resp_tx_ts);

        /* Write and send the response message. See NOTE 9 below. */
        tx_resp_msg[ALL_MSG_SN_IDX] = rx_frame_seq_nb;
        dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0); /* Zero offset in TX buffer. See Note 5 below.*/
        dwt_writetxfctrl(sizeof(tx_resp_msg), 0, 1);          /* Zero offset in TX buffer, ranging. */

        //TODO fiddle around with these settings until its fixed. imm works but then the timing is off
        //ret = dwt_starttx(DWT_START_TX_DELAYED);
        ret = dwt_starttx(DWT_START_TX_IMMEDIATE);

        /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. */
        if (ret == DWT_SUCCESS) {
            /* Poll DW1000 until TX frame sent event set. See NOTE 5 below. */
            while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS)) {
            };

            /* Clear TXFRS event. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

            /* Increment frame sequence number after transmission of the poll message (modulo 256). */
            rx_frame_seq_nb++;

        } else {
            /* If we end up in here then we have not succeded in transmitting the packet we sent up.
            POLL_RX_TO_RESP_TX_DLY_UUS is a critical value for porting to different processors.
            For slower platforms where the SPI is at a slower speed or the processor is operating at a lower
            frequency (Comparing to STM32F, SPI of 18MHz and Processor internal 72MHz)this value needs to be increased.
            Knowing the exact time when the responder is going to send its response is vital for time of flight
            calculation. The specification of the time of respnse must allow the processor enough time to do its
            calculations and put the packet in the Tx buffer. So more time is required for a slower system(processor).
            */

            /* Reset RX to properly reinitialise LDE operation. */
            dwt_rxreset();
        }
    }
    changePreambleCode(preambleCodeList[pcitx], preambleCodeList[pcirx]);
}

void doRanging(void){
    changePreambleCode(preambleCodeList[pcitx], preambleCodeList[pcitx]);
    dwt_setrxtimeout(65000);
    for(int i=0; i<3; i++){
        rangeRequest();
    }
    changePreambleCode(preambleCodeList[pcitx], preambleCodeList[pcirx]);
    dwt_setrxtimeout(0);
    interrupt_flag=false;
}

void rangeRequest(void)
{
  dwt_forcetrxoff();

  target_node++;

  if(target_node >= 4)
  {
    target_node = 1;
  }
  SEGGER_RTT_printf(0, "node %d\n",target_node);

  /*Calling Function to select which node the Initiator will poll for a ranging response*/
  target_node_select(target_node); 

  /* Write frame data to DW1000 and prepare transmission. See NOTE 3 below. */
 // tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
  dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
  dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0); /* Zero offset in TX buffer. */
  dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1); /* Zero offset in TX buffer, ranging. */
   // __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "write tx\n");

  /* Start transmission, indicating that a response is expected so that reception is enabled automatically after the frame is sent and the delay
  * set by dwt_setrxaftertxdelay() has elapsed. */
  dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

  tx_count++;
  //printf("Transmission # : %d\r\n",tx_count);
  SEGGER_RTT_printf(0, "tx started, transmission # %d\n",tx_count);

  /* We assume that the transmission is achieved correctly, poll for reception of a frame or error/timeout. See NOTE 4 below. */
  while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
  {};

    #if 0  // include if required to help debug timeouts.
    int temp = 0;		
    if(status_reg & SYS_STATUS_RXFCG )
    temp =1;
    else if(status_reg & SYS_STATUS_ALL_RX_TO )
    temp =2;
    if(status_reg & SYS_STATUS_ALL_RX_ERR )
    temp =3;
    #endif

  /* Increment frame sequence number after transmission of the poll message (modulo 256). */
  //frame_seq_nb++; // Unused now that each node has a specific frame_seq number to increment

    /* Calling function to increment frame of the correct node currently ranging to */
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "frame rx or timeout");

  frame_incr(target_node);

  if (status_reg & SYS_STATUS_RXFCG)
  {		
    uint32 frame_len;

    /* Clear good RX frame event in the DW1000 status register. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);

    /* A frame has been received, read it into the local buffer. */
    frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
   
    if (frame_len <= RX_BUF_LEN)
    {
      dwt_readrxdata(rx_buffer, frame_len, 0);
    }

    /* Check that the frame is the expected response from the companion "SS TWR responder" example.
    * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
    rx_buffer[ALL_MSG_SN_IDX] = 0;
    if (memcmp(rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0)
    {	
      rx_count++;
      //printf("Reception # : %d\r\n",rx_count);
      uint32 poll_tx_ts, resp_rx_ts, poll_rx_ts, resp_tx_ts;
      int32 rtd_init, rtd_resp;
      float clockOffsetRatio ;

      /* Retrieve poll transmission and response reception timestamps. See NOTE 5 below. */
      poll_tx_ts = dwt_readtxtimestamplo32();
      resp_rx_ts = dwt_readrxtimestamplo32();

      /* Read carrier integrator value and calculate clock offset ratio. See NOTE 7 below. */
      clockOffsetRatio = dwt_readcarrierintegrator() * (FREQ_OFFSET_MULTIPLIER * HERTZ_TO_PPM_MULTIPLIER_CHAN_5 / 1.0e6) ;

      /* Get timestamps embedded in response message. */
      resp_msg_get_ts(&rx_buffer[RESP_MSG_POLL_RX_TS_IDX], &poll_rx_ts);
      resp_msg_get_ts(&rx_buffer[RESP_MSG_RESP_TX_TS_IDX], &resp_tx_ts);

      /* Compute time of flight and distance, using clock offset ratio to correct for differing local and remote clock rates */
      rtd_init = resp_rx_ts - poll_tx_ts;
      rtd_resp = resp_tx_ts - poll_rx_ts;

      tof = ((rtd_init - rtd_resp * (1.0f - clockOffsetRatio)) / 2.0f) * DWT_TIME_UNITS; // Specifying 1.0f and 2.0f are floats to clear warning 
      distance_uncal = tof * SPEED_OF_LIGHT;

      /* Distance is transformed from the raw distance measurement to one with a scale and offset that prints range in meters*/

      distance = (distance_uncal*dist_scale)-dist_offset;
    }

      /* Calling function to print ranges to correct node that we are ranging to */

    print_distance(target_node);
  }
  else
  {
    /* Clear RX error/timeout events in the DW1000 status register. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);

    /* Reset RX to properly reinitialise LDE operation. */
    dwt_rxreset();
  }

  /* Execute a delay between ranging exchanges. */
       deca_sleep(RNG_DELAY_MS);
       //SEGGER_RTT_WriteString(0, "Hello World from your target.\n");

  //	return(1);
}

/**@brief SS TWR Initiator task entry function.
*
* @param[in] pvParameter   Pointer that will be used as the parameter for the task.
*/
/*
void ss_initiator_task_function (void * pvParameter)
{
  UNUSED_PARAMETER(pvParameter);

  //dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);

  dwt_setleds(DWT_LEDS_ENABLE);

  while (true)
  {
    ss_init_run();
    /* Delay a task for a given number of ticks *
    vTaskDelay(RNG_DELAY_MS);
    /* Tasks must be implemented to never return... *
  }
}*/

int target_node_select (int target_node)
{

        // Right now the nodes are set to flip flop between the addresses for Pumpernickel (PU) and Challah (CH) 
        // As we add more nodes, we can change the address changes to whatever address designates that node

        if(target_node == 1) 
       {
         tx_poll_msg[DEST_ADDR_1] = 'P';
         tx_poll_msg[DEST_ADDR_2] = 'U';
         rx_resp_msg[SOURCE_ADDR_1] = 'P';
         rx_resp_msg[SOURCE_ADDR_2] = 'U';
         tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb_1;
         //printf("Ranging to Pumpernickel %f\r\n"); // Was used for debugging
       }
        else if(target_node == 2)
       {
         tx_poll_msg[DEST_ADDR_1] = 'C';
         tx_poll_msg[DEST_ADDR_2] = 'H';
         rx_resp_msg[SOURCE_ADDR_1] = 'C';
         rx_resp_msg[SOURCE_ADDR_2] = 'H';
         tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb_2;
         //printf("Ranging to Challah %f\r\n"); //Was used for debugging
       }
        else if(target_node == 3)
       {
         tx_poll_msg[DEST_ADDR_1] = 'B';
         tx_poll_msg[DEST_ADDR_2] = 'A';
         rx_resp_msg[SOURCE_ADDR_1] = 'B';
         rx_resp_msg[SOURCE_ADDR_2] = 'A';
         tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb_3;
       }
       /*
        else if(target_node == 4)
       {
         tx_poll_msg[DEST_ADDR_1] = 'P';
         tx_poll_msg[DEST_ADDR_2] = 'U';
         rx_resp_msg[SOURCE_ADDR_1] = 'P';
         rx_resp_msg[SOURCE_ADDR_2] = 'U';
         tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb_1;
       }
        else if(target_node == 5)
       {
         tx_poll_msg[DEST_ADDR_1] = 'C';
         tx_poll_msg[DEST_ADDR_2] = 'H';
         rx_resp_msg[SOURCE_ADDR_1] = 'C';
         rx_resp_msg[SOURCE_ADDR_2] = 'H';
         tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb_2;
       }
        else if(target_node == 6)
       {
         tx_poll_msg[DEST_ADDR_1] = 'B';
         tx_poll_msg[DEST_ADDR_2] = 'A';
         rx_resp_msg[SOURCE_ADDR_1] = 'B';
         rx_resp_msg[SOURCE_ADDR_2] = 'A';
         tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb_3;
       }
        else if(target_node == 7)
       {
         tx_poll_msg[DEST_ADDR_1] = 'P';
         tx_poll_msg[DEST_ADDR_2] = 'U';
         rx_resp_msg[SOURCE_ADDR_1] = 'P';
         rx_resp_msg[SOURCE_ADDR_2] = 'U';
         tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb_1;
       }
        else if(target_node == 8)
       {
         tx_poll_msg[DEST_ADDR_1] = 'C';
         tx_poll_msg[DEST_ADDR_2] = 'H';
         rx_resp_msg[SOURCE_ADDR_1] = 'C';
         rx_resp_msg[SOURCE_ADDR_2] = 'H';
         tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb_2;
       }
        else if(target_node == 9)
       {
         tx_poll_msg[DEST_ADDR_1] = 'B';
         tx_poll_msg[DEST_ADDR_2] = 'A';
         rx_resp_msg[SOURCE_ADDR_1] = 'B';
         rx_resp_msg[SOURCE_ADDR_2] = 'A';
         tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb_3;
       }
        else if(target_node == 10)
       {
         tx_poll_msg[DEST_ADDR_1] = 'P';
         tx_poll_msg[DEST_ADDR_2] = 'U';
         rx_resp_msg[SOURCE_ADDR_1] = 'P';
         rx_resp_msg[SOURCE_ADDR_2] = 'P';
         tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb_1;
       }
        else if(target_node == 11)
       {
         tx_poll_msg[DEST_ADDR_1] = 'C';
         tx_poll_msg[DEST_ADDR_2] = 'H';
         rx_resp_msg[SOURCE_ADDR_1] = 'C';
         rx_resp_msg[SOURCE_ADDR_2] = 'H';
         tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb_2;
       }
        else if(target_node == 12)
       {
         tx_poll_msg[DEST_ADDR_1] = 'B';
         tx_poll_msg[DEST_ADDR_2] = 'A';
         rx_resp_msg[SOURCE_ADDR_1] = 'B';
         rx_resp_msg[SOURCE_ADDR_2] = 'A';
         tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb_3;
       }*/
}

int frame_incr(int target_node)
{
        // Same deal here, frame sequence flip flops between PU and CH (1 and 2) 
        // Once more nodes are added the frame sequence change will reflect the frame sequence that tracks that node

        if(target_node == 1) 
       {
          frame_seq_nb_1++;
       }
        else if(target_node == 2)
       {
          frame_seq_nb_2++;
       }
        else if(target_node == 3)
       {
          frame_seq_nb_3++;
       }
        else if(target_node == 4)
       {
          frame_seq_nb_1++;
       }
        else if(target_node == 5)
       {
          frame_seq_nb_2++;
       }
        else if(target_node == 6)
       {
          frame_seq_nb_3++;
       }
        else if(target_node == 7)
       {
          frame_seq_nb_1++;
       }
        else if(target_node == 8)
       {
          frame_seq_nb_2++;
       }
        else if(target_node == 9)
       {
          frame_seq_nb_3++;
       }
        else if(target_node == 10)
       {
          frame_seq_nb_1++;
       }
        else if(target_node == 11)
       {
          frame_seq_nb_2++;
       }
        else if(target_node == 12)
       {
          frame_seq_nb_3++;
       }
}

void print_distance(int target_node){
    SEGGER_RTT_printf(0, "node %d, distance %d\n",target_node, (int)(1000*distance));

}
void print_distance2(int target_node)
{
        // Same as before, can change these cases as more nodes are added
        // Can also take out the string message beforehand and just print distances

       if(target_node == 1) 
       {
            printf("Distance from Pumpernickel: %f\r\n",distance);
            //printf("%f\r\n",distance);
       }
       if(target_node == 2) 
       {
            printf("Distance from Challah: %f\r\n",distance);
            //printf("%f\r\n",distance);
       }
       if(target_node == 3) 
       {
            printf("Distance from Barley: %f\r\n",distance);
            //printf("%f\r\n",distance);
       }
       if(target_node == 4) 
       {
           printf("Distance from Pumpernickel: %f\r\n",distance);
           //printf("%f\r\n",distance);
       }
       if(target_node == 5) 
       {
            printf("Distance from Challah: %f\r\n",distance);
            //printf("%f\r\n",distance);
       }
       if(target_node == 6) 
       {
            printf("Distance from Barley: %f\r\n",distance);
            //printf("%f\r\n",distance);
       }
       if(target_node == 7) 
       {
           printf("Distance from Pumpernickel: %f\r\n",distance);
           //printf("%f\r\n",distance);
       }
       if(target_node == 8) 
       {
            printf("Distance from Challah: %f\r\n",distance);
            //printf("%f\r\n",distance);
       }
       if(target_node == 9) 
       {
            printf("Distance from Barley: %f\r\n",distance);
            //printf("%f\r\n",distance);
       }
       if(target_node == 10) 
       {
            printf("Distance from Pumpernickel: %f\r\n",distance);
            //printf("%f\r\n",distance);
       }
       if(target_node == 11) 
       {
            printf("Distance from Challah: %f\r\n",distance);
            //printf("%f\r\n",distance);
       }
       if(target_node == 12) 
       {
            printf("Distance from Barley: %f\r\n",distance);
            //printf("%f\r\n",distance);
       }
}


/*! ------------------------------------------------------------------------------------------------------------------
* @fn get_rx_timestamp_u64()
*
* @brief Get the RX time-stamp in a 64-bit variable.
*        /!\ This function assumes that length of time-stamps is 40 bits, for both TX and RX!
*
* @param  none
*
* @return  64-bit value of the read time-stamp.
*/
static uint64 get_rx_timestamp_u64(void)
{
  uint8 ts_tab[5];
  uint64 ts = 0;
  int i;
  dwt_readrxtimestamp(ts_tab);
  for (i = 4; i >= 0; i--)
  {
    ts <<= 8;
    ts |= ts_tab[i];
  }
  return ts;
}


/*! ------------------------------------------------------------------------------------------------------------------
* @fn final_msg_set_ts()
*
* @brief Fill a given timestamp field in the response message with the given value. In the timestamp fields of the
*        response message, the least significant byte is at the lower address.
*
* @param  ts_field  pointer on the first byte of the timestamp field to fill
*         ts  timestamp value
*
* @return none
*/
static void resp_msg_set_ts(uint8 *ts_field, const uint64 ts)
{
  int i;
  for (i = 0; i < RESP_MSG_TS_LEN; i++)
  {
    ts_field[i] = (ts >> (i * 8)) & 0xFF;
  }
}

/*! ------------------------------------------------------------------------------------------------------------------
* @fn resp_msg_get_ts()
*
* @brief Read a given timestamp value from the response message. In the timestamp fields of the response message, the
*        least significant byte is at the lower address.
*
* @param  ts_field  pointer on the first byte of the timestamp field to get
*         ts  timestamp value
*
* @return none
*/
static void resp_msg_get_ts(uint8_t *ts_field, uint32_t *ts)
{
  int i;
  *ts = 0;
  for (i = 0; i < RESP_MSG_TS_LEN; i++)
  {
    *ts += ts_field[i] << (i * 8);
  }
}

/*****************************************************************************************************************************************************
* NOTES:
*
* 1. The frames used here are Decawave specific ranging frames, complying with the IEEE 802.15.4 standard data frame encoding. The frames are the
*    following:
*     - a poll message sent by the initiator to trigger the ranging exchange.
*     - a response message sent by the responder to complete the exchange and provide all information needed by the initiator to compute the
*       time-of-flight (distance) estimate.
*    The first 10 bytes of those frame are common and are composed of the following fields:
*     - byte 0/1: frame control (0x8841 to indicate a data frame using 16-bit addressing).
*     - byte 2: sequence number, incremented for each new frame.
*     - byte 3/4: PAN ID (0xDECA).
*     - byte 5/6: destination address, see NOTE 2 below.
*     - byte 7/8: source address, see NOTE 2 below.
*     - byte 9: function code (specific values to indicate which message it is in the ranging process).
*    The remaining bytes are specific to each message as follows:
*    Poll message:
*     - no more data
*    Response message:
*     - byte 10 -> 13: poll message reception timestamp.
*     - byte 14 -> 17: response message transmission timestamp.
*    All messages end with a 2-byte checksum automatically set by DW1000.
* 2. Source and destination addresses are hard coded constants in this example to keep it simple but for a real product every device should have a
*    unique ID. Here, 16-bit addressing is used to keep the messages as short as possible but, in an actual application, this should be done only
*    after an exchange of specific messages used to define those short addresses for each device participating to the ranging exchange.
* 3. dwt_writetxdata() takes the full size of the message as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
*    automatically appended by the DW1000. This means that our variable could be two bytes shorter without losing any data (but the sizeof would not
*    work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
* 4. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
*    refer to DW1000 User Manual for more details on "interrupts". It is also to be noted that STATUS register is 5 bytes long but, as the event we
*    use are all in the first bytes of the register, we can use the simple dwt_read32bitreg() API call to access it instead of reading the whole 5
*    bytes.
* 5. The high order byte of each 40-bit time-stamps is discarded here. This is acceptable as, on each device, those time-stamps are not separated by
*    more than 2**32 device time units (which is around 67 ms) which means that the calculation of the round-trip delays can be handled by a 32-bit
*    subtraction.
* 6. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
*     DW1000 API Guide for more details on the DW1000 driver functions.
* 7. The use of the carrier integrator value to correct the TOF calculation, was added Feb 2017 for v1.3 of this example.  This significantly
*     improves the result of the SS-TWR where the remote responder unit's clock is a number of PPM offset from the local inmitiator unit's clock.
*     As stated in NOTE 2 a fixed offset in range will be seen unless the antenna delsy is calibratred and set correctly.
*
****************************************************************************************************************************************************/  	