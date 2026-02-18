/**
  ******************************************************************************
  * @file    LwIP/LwIP_IAP/Src/flash_if.c 
  * @author  MCD Application Team
  * @brief   This file provides high level routines to manage internal Flash 
  *          programming (erase and write). 
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "flash_if.h"
#include "FreeRTOS.h"
#include "task.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Gets the sector of a given address
  * @param  None
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t Address) {
  uint32_t sector = 0;
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0)) {
    sector = FLASH_SECTOR_0;
  } else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1)) {
    sector = FLASH_SECTOR_1;
  } else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2)) {
    sector = FLASH_SECTOR_2;
  } else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3)) {
    sector = FLASH_SECTOR_3;
  } else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4)) {
    sector = FLASH_SECTOR_4;
  } else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5)) {
    sector = FLASH_SECTOR_5;
  } else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6)) {
    sector = FLASH_SECTOR_6;
  } else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7)) {
    sector = FLASH_SECTOR_7;
  } else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8)) {
    sector = FLASH_SECTOR_8;
  } else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9)) {
    sector = FLASH_SECTOR_9;
  } else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10)) {
    sector = FLASH_SECTOR_10;
  } else if((Address < ADDR_FLASH_SECTOR_12) && (Address >= ADDR_FLASH_SECTOR_11)) {
    sector = FLASH_SECTOR_11;
  } else if((Address < ADDR_FLASH_SECTOR_13) && (Address >= ADDR_FLASH_SECTOR_12)) {
    sector = FLASH_SECTOR_12;
  } else if((Address < ADDR_FLASH_SECTOR_14) && (Address >= ADDR_FLASH_SECTOR_13)) {
    sector = FLASH_SECTOR_13;
  } else if((Address < ADDR_FLASH_SECTOR_15) && (Address >= ADDR_FLASH_SECTOR_14)) {
    sector = FLASH_SECTOR_14;
  } else if((Address < ADDR_FLASH_SECTOR_16) && (Address >= ADDR_FLASH_SECTOR_15)) {
    sector = FLASH_SECTOR_15;
  } else if((Address < ADDR_FLASH_SECTOR_17) && (Address >= ADDR_FLASH_SECTOR_16)) {
    sector = FLASH_SECTOR_16;
  } else if((Address < ADDR_FLASH_SECTOR_18) && (Address >= ADDR_FLASH_SECTOR_17)) {
    sector = FLASH_SECTOR_17;
  } else if((Address < ADDR_FLASH_SECTOR_19) && (Address >= ADDR_FLASH_SECTOR_18)) {
    sector = FLASH_SECTOR_18;
  } else if((Address < ADDR_FLASH_SECTOR_20) && (Address >= ADDR_FLASH_SECTOR_19)) {
    sector = FLASH_SECTOR_19;
  } else if((Address < ADDR_FLASH_SECTOR_21) && (Address >= ADDR_FLASH_SECTOR_20)) {
    sector = FLASH_SECTOR_20;
  } else if((Address < ADDR_FLASH_SECTOR_22) && (Address >= ADDR_FLASH_SECTOR_21)) {
    sector = FLASH_SECTOR_21;
  } else if((Address < ADDR_FLASH_SECTOR_23) && (Address >= ADDR_FLASH_SECTOR_22)) {
    sector = FLASH_SECTOR_22;
  } else // (Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_23)
  {
    sector = FLASH_SECTOR_23;
  }  
  return sector;
}




/**
  * @brief  Gets sector Size
  * @param  None
  * @retval The size of a given sector
  */
/*
static uint32_t GetSectorSize(uint32_t Sector) {
  uint32_t sectorsize = 0x00;
  if((Sector == FLASH_SECTOR_0) || (Sector == FLASH_SECTOR_1) || (Sector == FLASH_SECTOR_2) ||\
     (Sector == FLASH_SECTOR_3) || (Sector == FLASH_SECTOR_12) || (Sector == FLASH_SECTOR_13) ||\
     (Sector == FLASH_SECTOR_14) || (Sector == FLASH_SECTOR_15))
  {
    sectorsize = 16 * 1024;
  } else if((Sector == FLASH_SECTOR_4) || (Sector == FLASH_SECTOR_16)) {
    sectorsize = 64 * 1024;
  } else {
    sectorsize = 128 * 1024;
  }  
  return sectorsize;
}
*/

/**
 * @brief  
 *
 */
HAL_StatusTypeDef FLASH_EraseSector(uint32_t Sector, uint8_t VoltageRange) {
  HAL_StatusTypeDef status = HAL_ERROR;
  FLASH_Erase_Sector(Sector, (uint8_t) VoltageRange);
  // Wait for last operation to be completed
  status = FLASH_WaitForLastOperation((uint32_t)3000);
  // If the erase operation is completed, disable the SER and SNB Bits
  CLEAR_BIT(FLASH->CR, (FLASH_CR_SER | FLASH_CR_SNB));

  return status;
}

/**
 * @brief  
 *
 */
HAL_StatusTypeDef writeFlash (uint32_t addr, uint32_t *data, uint32_t length) {
  HAL_StatusTypeDef status = HAL_ERROR;
  uint32_t FirstSector = GetSector(addr);  
  __disable_irq();
  HAL_FLASH_Unlock();
  
  // Clear pending flags (if any)
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
  
  if (FLASH_EraseSector(FirstSector, VOLTAGE_RANGE_3) != HAL_OK) {
    HAL_FLASH_Lock();
    __enable_irq();
    return HAL_ERROR;
  }
  for (uint8_t i = 0; i < length / 4; i++) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data[i]) == HAL_OK) {
      addr += 4;
    } else {
      break;
    }
  }
  HAL_FLASH_Lock();
  __enable_irq();  
  return status;
}

uint32_t FlashWORD(uint32_t addr, uint32_t *data, uint32_t size) {  
  uint32_t sector = GetSector(addr);
  uint32_t count_word = size / 4;
  uint32_t sector_error = 0;
  
  FLASH_EraseInitTypeDef FLASH_EraseInitStruct;
  FLASH_EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  FLASH_EraseInitStruct.Sector = sector;
  FLASH_EraseInitStruct.NbSectors = 1;
  FLASH_EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  
   // Clear pending flags (if any)
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP|FLASH_FLAG_OPERR|FLASH_FLAG_WRPERR|
                         FLASH_FLAG_PGAERR|FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
  
  // Enable the Flash control register access
  HAL_FLASH_Unlock();
  if (HAL_FLASHEx_Erase(&FLASH_EraseInitStruct, &sector_error) != HAL_OK) {
    // sector_error
    // if HAL_ERROR -> contains the configuration information on faulty sector in case of error
    // if HAL_OK -> 0xFFFFFFFFU means that all the sectors have been correctly erased
    HAL_FLASH_Lock();
    return HAL_FLASH_GetError();
  }
  uint8_t try_count = 0;
  
  // program the Flash area word by word
  for (uint32_t i = 0; i < count_word; i++) {
    // Clear pending flags (if any)
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP|FLASH_FLAG_OPERR|FLASH_FLAG_WRPERR|
                           FLASH_FLAG_PGAERR|FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data[i]) == HAL_OK) {      
      if (*(__IO uint32_t *)addr != data[i]) {
        // Flash content doesn't match SRAM content
        HAL_FLASH_Lock();
        return(5);
      }
      addr += 4;
    } else {
      // Error occurred while writing data in Flash memory
      vTaskDelay(10);
      if (try_count < 3) {
        i--;
        try_count++;
      } else {
        HAL_FLASH_Lock();
        return HAL_FLASH_GetError();        
      }
    }
  }
  HAL_FLASH_Lock();
  return HAL_OK;
}

/**
 * @brief  
 *
 */
void readFlash(uint32_t addr, uint32_t *data, uint32_t length) {
  for (int i = 0; i < length / 4; i++) {
    data[i] = *(__IO uint32_t*)addr;
    addr += 4;
  }
}

/**
  * @brief  This function does an erase of all user flash area
  * @param  StartSector: start of user flash area
  * @retval 0: user flash area successfully erased
  *         1: error occurred
  */
int8_t FLASH_If_Erase(uint32_t addr, uint8_t count) {
  uint32_t sector = GetSector(addr);
  uint32_t sector_error = 0;
  FLASH_EraseInitTypeDef FLASH_EraseInitStruct;    
  FLASH_EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  FLASH_EraseInitStruct.Sector = sector;
  FLASH_EraseInitStruct.NbSectors = count;
  FLASH_EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  // Clear pending flags (if any)
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP|FLASH_FLAG_OPERR|FLASH_FLAG_WRPERR|
                           FLASH_FLAG_PGAERR|FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
    
  if (HAL_FLASHEx_Erase(&FLASH_EraseInitStruct, &sector_error) != HAL_OK) {
    return HAL_FLASH_GetError();
  }
  return (0);
}
/**
  * @brief  This function writes a data buffer in flash (data are 32-bit aligned).
  * @note   After writing data buffer, the flash content is checked.
  * @param  FlashAddress: start address for writing data buffer
  * @param  Data: pointer on data buffer
  * @param  DataLength: length of data buffer (unit is 32-bit word)   
  * @retval 0: Data successfully written to Flash memory
  *         1: Error occurred while writing data in Flash memory
  *         2: Written Data in flash memory is different from expected one
  */
uint32_t FLASH_If_Write(__IO uint32_t* FlashAddress, uint32_t end, uint32_t* data, uint16_t DataLength) {
  uint8_t try_count = 0;
  for (uint32_t i = 0; (i < DataLength) && (*FlashAddress <= (end)); i++) {
    // Clear pending flags (if any)
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP|FLASH_FLAG_OPERR|FLASH_FLAG_WRPERR|
                           FLASH_FLAG_PGAERR|FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
    
    // Device voltage range supposed to be [2.7V to 3.6V], the operation will be done by word
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, *FlashAddress,  *(uint32_t*)(data + i)) == HAL_OK) {
      // Check the written value
      //if (*(__IO uint32_t *)*FlashAddress != *(uint32_t*)(data + i)) {
      //  // Flash content doesn't match SRAM content
      //  return(2);
      //}
      // Increment FLASH destination address
      *FlashAddress += 4;
    } else {
      vTaskDelay(10);
      if (try_count < 3) {
        i--;
        try_count++;
      } else {
        return HAL_FLASH_GetError();
      }
    }
  }
  return (0);
}
