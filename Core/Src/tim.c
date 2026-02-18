/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "tim.h"

TIM_HandleTypeDef htim12;
TIM_HandleTypeDef htim14;

// TIM12 init function
void MX_TIM12_Init(void) {
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};

  htim12.Instance = TIM12;
  htim12.Init.Prescaler = 179;
  htim12.Init.CounterMode = TIM_COUNTERMODE_UP;
  //htim12.Init.Period = 399; // 0.8 ms
  htim12.Init.Period = 199; // 0.4 ms
  htim12.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim12) != HAL_OK) {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim12, &sClockSourceConfig) != HAL_OK) {
    Error_Handler();
  }
}

// TIM14 init function
void MX_TIM14_Init(void) {
  htim14.Instance = TIM14;
  htim14.Init.Prescaler = 179;
  htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim14.Init.Period = 999; // 2.0 ms
  htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim14) != HAL_OK) {
    Error_Handler();
  }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle) {
  if (tim_baseHandle->Instance == TIM12) {
    // TIM12 clock enable
    __HAL_RCC_TIM12_CLK_ENABLE();

    // TIM12 interrupt Init
    HAL_NVIC_SetPriority(TIM8_BRK_TIM12_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM8_BRK_TIM12_IRQn);
  } else if(tim_baseHandle->Instance == TIM14) {
    // TIM14 clock enable
    __HAL_RCC_TIM14_CLK_ENABLE();

    // TIM14 interrupt Init
    HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle) {
  if (tim_baseHandle->Instance == TIM12) {
    // Peripheral clock disable
    __HAL_RCC_TIM12_CLK_DISABLE();

    // TIM12 interrupt Deinit
    HAL_NVIC_DisableIRQ(TIM8_BRK_TIM12_IRQn);
  } else if(tim_baseHandle->Instance == TIM14) {
    // Peripheral clock disable
    __HAL_RCC_TIM14_CLK_DISABLE();

    // TIM14 interrupt Deinit
    HAL_NVIC_DisableIRQ(TIM8_TRG_COM_TIM14_IRQn);
  }
}

void Start_IT_TIM12() {
  HAL_TIM_Base_Start_IT(&htim12);
}
  
void Start_IT_TIM14() {
  HAL_TIM_Base_Start_IT(&htim14);
}
