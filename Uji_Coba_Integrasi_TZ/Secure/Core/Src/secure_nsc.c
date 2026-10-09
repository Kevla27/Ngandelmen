/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    Secure/Src/secure_nsc.c
  * @author  MCD Application Team
  * @brief   This file contains the non-secure callable APIs (secure world)
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

/* USER CODE BEGIN Non_Secure_CallLib */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "secure_nsc.h"
/** @addtogroup STM32U5xx_HAL_Examples

  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Global variables ----------------------------------------------------------*/
void *pSecureFaultCallback = NULL;   /* Pointer to secure fault callback in Non-secure */
void *pSecureErrorCallback = NULL;   /* Pointer to secure error callback in Non-secure */
static funcptr_NS pSecureTamperCallback = NULL; /* Pointer to tamper callback in Non-secure */

/* Pascabayar (Post-Paid) Secure Cumulative Energy Register (Anti-Tamper & Anti-Rollback) */
/* Baseline 125.43 kWh (125,430,000 mWh) */
static uint64_t s_secure_total_energy_mwh = 125430ULL * 1000ULL;

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Secure registration of non-secure callback.
  * @param  CallbackId  callback identifier
  * @param  func        pointer to non-secure function
  * @retval None
  */
CMSE_NS_ENTRY void SECURE_RegisterCallback(SECURE_CallbackIDTypeDef CallbackId, void *func)
{
  if(func != NULL)
  {
    switch(CallbackId)
    {
      case SECURE_FAULT_CB_ID:           /* SecureFault Interrupt occurred */
      pSecureFaultCallback = func;
      break;
      case GTZC_ERROR_CB_ID:             /* GTZC Interrupt occurred */
      pSecureErrorCallback = func;
      break;
      case SECURE_TAMPER_CB_ID:          /* Tamper Case Open Interrupt occurred */
      pSecureTamperCallback = (funcptr_NS)func;
      break;
      default:
      /* unknown */
      break;
    }
  }
}

/**
  * @brief  Trigger callback to Non-Secure world when PC13 Tamper interrupt occurs.
  * @retval None
  */
void Secure_TriggerTamperCallback(void)
{
  if (pSecureTamperCallback != NULL)
  {
    pSecureTamperCallback();
  }
}

/**
  * @brief  Menambahkan akumulasi energi aktif dalam satuan Watt-hour (Wh).
  * @param  delta_wh Nilai penambahan energi aktif (Wh)
  * @retval None
  */
CMSE_NS_ENTRY void Secure_AddEnergyWh(uint32_t delta_wh)
{
  s_secure_total_energy_mwh += ((uint64_t)delta_wh * 1000ULL);
}

/**
  * @brief  Menambahkan akumulasi energi aktif dalam satuan milli-Watt-hour (mWh).
  * @param  delta_mwh Nilai penambahan energi aktif (mWh)
  * @retval None
  */
CMSE_NS_ENTRY void Secure_AddEnergyMilliWh(uint32_t delta_mwh)
{
  s_secure_total_energy_mwh += (uint64_t)delta_mwh;
}

/**
  * @brief  Membaca total akumulasi energi aktif dalam satuan Watt-hour (Wh).
  * @retval Total energi aktif (Wh)
  */
CMSE_NS_ENTRY uint64_t Secure_GetTotalEnergyWh(void)
{
  return (s_secure_total_energy_mwh / 1000ULL);
}

/**
  * @brief  Membaca total akumulasi energi aktif dalam satuan milli-Watt-hour (mWh).
  * @retval Total energi aktif (mWh)
  */
CMSE_NS_ENTRY uint64_t Secure_GetTotalEnergyMilliWh(void)
{
  return s_secure_total_energy_mwh;
}

/**
  * @}
  */

/**
  * @}
  */
/* USER CODE END Non_Secure_CallLib */

