/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */

#ifdef i2c
#define EEPROM_24C64_I2C_ADDR          (0x50U << 1)
#define EEPROM_I2C_READY_TRIES         3U
#define EEPROM_I2C_READY_TIMEOUT_MS    100U
#define EEPROM_TEST_ADDR               0x0000U
#define EEPROM_WRITE_TIMEOUT_MS        100U
#define EEPROM_READ_TIMEOUT_MS         100U
#define EEPROM_WRITE_VERIFY_BYTE        0xA5U
#define EEPROM_POST_WRITE_READY_TRIES   10U

static HAL_StatusTypeDef EEPROM_VerifyReadWrite(uint16_t mem_addr_size);
static HAL_StatusTypeDef EEPROM_VerifyReadable(uint16_t mem_addr_size);

static volatile uint8_t eeprom_ready = 0U;
#endif

/* USER CODE END 0 */

#ifdef i2c
I2C_HandleTypeDef hi2c1;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* I2C1 interrupt Init */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

    /* I2C1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

void EEPROM_Probe(void) {
  eeprom_ready = 0U;

  if (HAL_I2C_IsDeviceReady(&hi2c1, EEPROM_24C64_I2C_ADDR, EEPROM_I2C_READY_TRIES, EEPROM_I2C_READY_TIMEOUT_MS) == HAL_OK) {
    if (EEPROM_VerifyReadWrite(I2C_MEMADD_SIZE_16BIT) == HAL_OK ||
        EEPROM_VerifyReadWrite(I2C_MEMADD_SIZE_8BIT) == HAL_OK) {
      eeprom_ready = 1U;
    } else if (EEPROM_VerifyReadable(I2C_MEMADD_SIZE_16BIT) == HAL_OK ||
               EEPROM_VerifyReadable(I2C_MEMADD_SIZE_8BIT) == HAL_OK) {
      /* Write may be blocked by hardware WP pin: treat readable EEPROM as present. */
      eeprom_ready = 1U;
    }
  }
}

static HAL_StatusTypeDef EEPROM_VerifyReadWrite(uint16_t mem_addr_size) {
  uint8_t original_byte = 0U;
  uint8_t verify_byte = 0U;
  uint8_t write_byte = EEPROM_WRITE_VERIFY_BYTE;

  if (HAL_I2C_Mem_Read(&hi2c1,
                       EEPROM_24C64_I2C_ADDR,
                       EEPROM_TEST_ADDR,
                       mem_addr_size,
                       &original_byte,
                       1U,
                       EEPROM_READ_TIMEOUT_MS) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_I2C_Mem_Write(&hi2c1,
                        EEPROM_24C64_I2C_ADDR,
                        EEPROM_TEST_ADDR,
                        mem_addr_size,
                        &write_byte,
                        1U,
                        EEPROM_WRITE_TIMEOUT_MS) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_I2C_IsDeviceReady(&hi2c1,
                            EEPROM_24C64_I2C_ADDR,
                            EEPROM_POST_WRITE_READY_TRIES,
                            EEPROM_I2C_READY_TIMEOUT_MS) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_I2C_Mem_Read(&hi2c1,
                       EEPROM_24C64_I2C_ADDR,
                       EEPROM_TEST_ADDR,
                       mem_addr_size,
                       &verify_byte,
                       1U,
                       EEPROM_READ_TIMEOUT_MS) != HAL_OK ||
      verify_byte != write_byte) {
    return HAL_ERROR;
  }

  if (HAL_I2C_Mem_Write(&hi2c1,
                        EEPROM_24C64_I2C_ADDR,
                        EEPROM_TEST_ADDR,
                        mem_addr_size,
                        &original_byte,
                        1U,
                        EEPROM_WRITE_TIMEOUT_MS) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_I2C_IsDeviceReady(&hi2c1,
                            EEPROM_24C64_I2C_ADDR,
                            EEPROM_POST_WRITE_READY_TRIES,
                            EEPROM_I2C_READY_TIMEOUT_MS) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

static HAL_StatusTypeDef EEPROM_VerifyReadable(uint16_t mem_addr_size) {
  uint8_t value = 0U;

  if (HAL_I2C_Mem_Read(&hi2c1,
                       EEPROM_24C64_I2C_ADDR,
                       EEPROM_TEST_ADDR,
                       mem_addr_size,
                       &value,
                       1U,
                       EEPROM_READ_TIMEOUT_MS) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

uint8_t EEPROM_IsReady(void) {
  return eeprom_ready;
}

/* USER CODE END 1 */

#endif /* i2c */
