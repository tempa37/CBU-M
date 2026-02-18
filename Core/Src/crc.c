#include "stm32f4xx.h"

uint32_t hw_crc32(uint8_t* data, uint32_t len) {
   uint32_t * pBuffer = (uint32_t *)data;
   uint32_t BufferLength = len / 4;
   uint32_t index = 0;
   //
   RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
   // Reset CRC generator
   CRC->CR = CRC_CR_RESET;
   //
   for (index = 0; index < BufferLength; index++) {
    CRC->DR = (pBuffer[index]);
   }
   return (CRC->DR);
}

