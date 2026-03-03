/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include <string.h>

UART_HandleTypeDef huart1;

DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

UART_HandleTypeDef huart3;

DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;

// USART1 init function
void MX_USART1_UART_Init(void) {
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    Error_Handler();
  }
}

// USART3 init function
void MX_USART3_UART_Init(void) {
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK) {
    Error_Handler();
  }
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (uartHandle->Instance == USART1) {
    // USART1 clock enable
    __HAL_RCC_USART1_CLK_ENABLE();
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // USART1 DMA Init
    
    // USART1_RX Init
    hdma_usart1_rx.Instance = DMA2_Stream2;
    hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode = DMA_NORMAL;
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK) {
      Error_Handler();
    }
    
    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);
    
    // USART1_TX Init
    hdma_usart1_tx.Instance = DMA2_Stream7;
    hdma_usart1_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }
    
    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);
    
    // USART1 interrupt Init
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  } else if (uartHandle->Instance == USART3) {
    // USART3 clock enable
    __HAL_RCC_USART3_CLK_ENABLE();
    
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PC10     ------> USART3_TX
    PC11     ------> USART3_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    // USART3 DMA Init
    
    // USART3_RX Init
    hdma_usart3_rx.Instance = DMA1_Stream1;
    hdma_usart3_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart3_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart3_rx.Init.Mode = DMA_NORMAL;
    hdma_usart3_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart3_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart3_rx) != HAL_OK)
    {
      Error_Handler();
    }
    
    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart3_rx);
    
    // USART3_TX Init
    hdma_usart3_tx.Instance = DMA1_Stream3;
    hdma_usart3_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart3_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart3_tx.Init.Mode = DMA_NORMAL;
    hdma_usart3_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart3_tx) != HAL_OK)
    {
      Error_Handler();
    }
    
    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart3_tx);
    
    // USART3 interrupt Init
    HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle) {
  if (uartHandle->Instance == USART1) {
    // Peripheral clock disable
    __HAL_RCC_USART1_CLK_DISABLE();
    
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
    
    // USART1 DMA DeInit
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);
    
    // USART1 interrupt Deinit
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  } else if(uartHandle->Instance == USART3) {
    // Peripheral clock disable
    __HAL_RCC_USART3_CLK_DISABLE();
    
    /**USART3 GPIO Configuration
    PC10     ------> USART3_TX
    PC11     ------> USART3_RX
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_11);
    
    /* USART3 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);
    
    // USART3 interrupt Deinit
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  }
}

HAL_StatusTypeDef UART_RS485_SetDirection(UART_HandleTypeDef *huart, uint8_t tx_enable) {
  GPIO_PinState pin_state = tx_enable ? GPIO_PIN_SET : GPIO_PIN_RESET;

  if (huart == &huart1) {
    HAL_GPIO_WritePin(UART1_RE_DE_Port, UART1_RE_DE_Pin, pin_state);
    return HAL_OK;
  }

  if (huart == &huart3) {
    HAL_GPIO_WritePin(UART2_RE_DE_Port, UART2_RE_DE_Pin, pin_state);
    return HAL_OK;
  }

  return HAL_ERROR;
}

HAL_StatusTypeDef UART_SendPacket(UART_HandleTypeDef *huart, const uint8_t *data, uint16_t len, uint32_t timeout) {
  if (huart == NULL || data == NULL || len == 0U) {
    return HAL_ERROR;
  }

  if (UART_RS485_SetDirection(huart, 1U) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_UART_Transmit(huart, (uint8_t *)data, len, timeout);
}

HAL_StatusTypeDef UART_ReceivePacket(UART_HandleTypeDef *huart, uint8_t *data, uint16_t len, uint32_t timeout) {
  if (huart == NULL || data == NULL || len == 0U) {
    return HAL_ERROR;
  }

  if (UART_RS485_SetDirection(huart, 0U) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_UART_Receive(huart, data, len, timeout);
}

HAL_StatusTypeDef UART_SendReceivePacket(UART_HandleTypeDef *tx_uart, UART_HandleTypeDef *rx_uart, uint8_t tx_uart1, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len, uint32_t tx_timeout, uint32_t rx_timeout) {
  if (tx_uart == NULL || rx_uart == NULL || tx_data == NULL || rx_data == NULL || len == 0U) {
    return HAL_ERROR;
  }

  HAL_GPIO_WritePin(RS485_1_ON_Port, RS485_1_ON_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(RS485_2_ON_Port, RS485_2_ON_Pin, GPIO_PIN_SET);
  HAL_Delay(2);

  if (UART_RS485_SetDirection(&huart1, tx_uart1 ? 1U : 0U) != HAL_OK) {
    return HAL_ERROR;
  }

  if (UART_RS485_SetDirection(&huart3, tx_uart1 ? 0U : 1U) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_UART_Abort(tx_uart) != HAL_OK || HAL_UART_Abort(rx_uart) != HAL_OK) {
    return HAL_ERROR;
  }

  if (HAL_UART_Transmit(tx_uart, (uint8_t *)tx_data, len, tx_timeout) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_UART_Receive(rx_uart, rx_data, len, rx_timeout);
}
