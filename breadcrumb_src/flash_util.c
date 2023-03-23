#include "flash_util.h"
#include "nrf_nvmc.h"

//flash memory stuff
static const uint32_t f_addr = 0x0007f000;//flash mem last page start address
//static uint32_t *p_addr = (uint32_t*)f_addr;

uint32_t readWordFlash(void){
    return (uint32_t) *(uint32_t*)f_addr;
}

void writeWordFlash(uint32_t word){
    nrf_nvmc_page_erase(f_addr);
    nrf_nvmc_write_word(f_addr,word);
}