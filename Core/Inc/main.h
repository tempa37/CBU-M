/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
  
#define DEBUG
    
#define CHECK_LOAD_SYSTEM 0
  
#define WFLASH 1

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/

#define PARTITION_SIZE          ((uint32_t)917500) /*!< Firmware length in bytes without 4-byte CRC32  */

// sector 5
#define PARTITION_1_ADDR        ((uint32_t)0x08020000)
// sector 11
#define PARTITION_1_CRC_ADDR    ((uint32_t)0x080FFFFC)

// sector 13
#define PARTITION_SELECT_ADDR   ((uint32_t)0x08104000)

// sector 17
#define PARTITION_2_ADDR        ((uint32_t)0x08120000)
// sector 23
#define PARTITION_2_CRC_ADDR    ((uint32_t)0x081FFFFC)

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */


/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

// Private defines
#define BDU1_M_S_Pin GPIO_PIN_3
#define BDU1_M_S_GPIO_Port GPIOE

#define BDU2_M_S_Pin GPIO_PIN_4
#define BDU2_M_S_GPIO_Port GPIOE

#define PWR_KTV_Pin GPIO_PIN_5
#define PWR_KTV_GPIO_Port GPIOE

#define KTV_ADR_Pin GPIO_PIN_0
#define KTV_ADR_GPIO_Port GPIOC

#define WDI_Pin GPIO_PIN_2
#define WDI_GPIO_Port GPIOB

#define EEPROM_WP_Pin GPIO_PIN_5
#define EEPROM_WP_GPIO_Port GPIOB

#define INT_LAN8710_Pin GPIO_PIN_7
#define INT_LAN8710_GPIO_Port GPIOE

#define OLED_DC_Pin GPIO_PIN_10
#define OLED_DC_GPIO_Port GPIOE

#define OLED_CS_Pin GPIO_PIN_11
#define OLED_CS_GPIO_Port GPIOE

#define OLED_RST_Pin GPIO_PIN_15
#define OLED_RST_GPIO_Port GPIOE

#define CON_ON_OFF_Pin GPIO_PIN_8
#define CON_ON_OFF_GPIO_Port GPIOD

#define STM_LOOP_LINK_Pin GPIO_PIN_9
#define STM_LOOP_LINK_GPIO_Port GPIOD

#define ANA_RELE_2_Pin GPIO_PIN_13
#define ANA_RELE_2_GPIO_Port GPIOD

#define CON_1_Pin GPIO_PIN_14
#define CON_1_Port GPIOD

#define CON_2_Pin GPIO_PIN_12
#define CON_2_Port GPIOD

#define MCU_BLK_1_1_Pin GPIO_PIN_6
#define MCU_BLK_1_1_GPIO_Port GPIOC

#define MCU_BLK_1_2_Pin GPIO_PIN_15
#define MCU_BLK_1_2_GPIO_Port GPIOD

#define MCU_BLK_2_1_Pin GPIO_PIN_11
#define MCU_BLK_2_1_GPIO_Port GPIOD

#define MCU_BLK_2_2_Pin GPIO_PIN_10
#define MCU_BLK_2_2_GPIO_Port GPIOD

#define RST_PHYLAN_Pin GPIO_PIN_8
#define RST_PHYLAN_Port GPIOC

#define UART1_RE_DE_Pin GPIO_PIN_12
#define UART1_RE_DE_Port GPIOC

#define UART2_RE_DE_Pin GPIO_PIN_11
#define UART2_RE_DE_Port GPIOA

#define RS485_1_ON_Pin GPIO_PIN_15
#define RS485_1_ON_Port GPIOA

#define RS485_2_ON_Pin GPIO_PIN_9
#define RS485_2_ON_Port GPIOC

// to the right
#define K1_Pin GPIO_PIN_0
#define K1_Port GPIOD

// up
#define K2_Pin GPIO_PIN_2
#define K2_Port GPIOD

// to the left
#define K3_Pin GPIO_PIN_4
#define K3_Port GPIOD

// input
#define K4_Pin GPIO_PIN_5
#define K4_Port GPIOD

// down
#define K5_Pin GPIO_PIN_6
#define K5_Port GPIOD

// cancel
#define K6_Pin GPIO_PIN_7
#define K6_Port GPIOD

// start
#define K7_Pin GPIO_PIN_3
#define K7_Port GPIOB

// stop
#define K8_Pin GPIO_PIN_4
#define K8_Port GPIOB

// emergency
#define K9_Pin GPIO_PIN_1
#define K9_Port GPIOD

extern volatile uint8_t manual_pins_mode;

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
