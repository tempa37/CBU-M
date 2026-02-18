/******************************************************************************
**************************Hardware interface layer*****************************
* | file      		:	DEV_Config.c
* |	version			:	V1.0
* | date			:	2020-06-17
* | function		:	Provide the hardware underlying interface	
******************************************************************************/
#include "DEV_Config.h"

#include "stm32f4xx_hal_spi.h"
#include "spi.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>		//printf()
#include <string.h>
#include <stdlib.h>

#ifdef DEBUG
unsigned int spi_error_count = 0;
unsigned int spi_out_count = 0;
#endif

/********************************************************************************
function:	Hardware interface
note:
	SPI4W_Write_Byte(value) : 
		HAL library hardware SPI
		Register hardware SPI
		Gpio analog SPI
	I2C_Write_Byte(value, cmd):
		HAL library hardware I2C
********************************************************************************/
#ifdef DEBUG
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi == &hspi4) {
    spi_out_count++;
  }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
  if (hspi == &hspi4) {
    spi_error_count++;
  }
}
#endif

uint8_t SPI4W_Write_Byte(uint8_t value) {
  
  //HAL_SPI_Transmit_DMA(&hspi4, &value, 1);
  //while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);
  
  //HAL_SPI_Transmit(&hspi4, &value, 1, 100);
  
  HAL_SPI_Transmit_IT(&hspi4, &value, 1);
  while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);

  return *((__IO uint8_t *)(&SPI4->DR));
}

void write_data(uint8_t *data, uint16_t length) {
  HAL_SPI_Transmit_IT(&hspi4, data, length);
}

/*
void I2C_Write_Byte(uint8_t value, uint8_t Cmd) {
  
  int Err;
  uint8_t W_Buf[2];
  W_Buf[0] = Cmd;
  W_Buf[1] = value;

  
  if (HAL_I2C_Master_Transmit(&hi2c1, (0X3C << 1) | 0X00, W_Buf, 2, 0x10) != HAL_OK) {
    Err++;
    if(Err == 1000) {
      printf("send error\r\n");
      return ;
    }
  }

}
*/
/********************************************************************************
function:	Delay function
note:
	Driver_Delay_ms(xms) : Delay x ms
	Driver_Delay_us(xus) : Delay x us
********************************************************************************/
void Driver_Delay_ms(uint32_t xms) {
  osDelay(xms);
}

void Driver_Delay_us(uint32_t xus) {
  int j;
  for(j = xus; j > 0; j--);
}
