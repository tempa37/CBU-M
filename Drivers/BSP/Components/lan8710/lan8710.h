/**
  ******************************************************************************
  * @file    LAN8710.h
  * @author  MCD Application Team
  * @brief   This file contains all the functions prototypes for the
  *          LAN8710.c PHY driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef LAN8710_H
#define LAN8710_H

#ifdef __cplusplus
 extern "C" {
#endif   
   
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/** @addtogroup BSP
  * @{
  */ 

/** @addtogroup Component
  * @{
  */
    
/** @defgroup LAN8710
  * @{
  */    
/* Exported constants --------------------------------------------------------*/
/** @defgroup LAN8710_Exported_Constants LAN8710 Exported Constants
  * @{
  */ 
  
/** @defgroup LAN8710_Registers_Mapping LAN8710 Registers Mapping
  * @{
  */ 
//  Basic Control Register 
#define LAN8710_BCR      ((uint16_t)0x0000U)
// Basic Status Register
#define LAN8710_BSR      ((uint16_t)0x0001U)
// PHY Identifier 1
#define LAN8710_PHYI1R   ((uint16_t)0x0002U)
// PHY Identifier 2
#define LAN8710_PHYI2R   ((uint16_t)0x0003U)
// Auto-Negotiation Advertisement
#define LAN8710_ANAR     ((uint16_t)0x0004U)
// Auto-Neg. Link Partner Ability
#define LAN8710_ANLPAR   ((uint16_t)0x0005U)
// Auto-Neg. Expansion Register
#define LAN8710_ANER     ((uint16_t)0x0006U)

// Mode Control/Status Register
#define LAN8710_MCSR     ((uint16_t)0x0011U)
// Special Modes Register
#define LAN8710_SMR      ((uint16_t)0x0012U)
// System Error Counter Register
#define LAN8710_SECR     ((uint16_t)0x001AU)
// Control/Status Indication Register   
#define LAN8710_SCSIR    ((uint16_t)0x001BU)
// Interrupt Source Register   
#define LAN8710_ISFR     ((uint16_t)0x001DU)
// Interrupt Mask Register   
#define LAN8710_IMR      ((uint16_t)0x001EU)
// PHY Special Ctrl/Status Register   
#define LAN8710_PHYSCSR  ((uint16_t)0x001FU)

/**
  * @}
  */

/** @defgroup LAN8710_BCR_Bit_Definition LAN8710 BCR Bit Definition
  * @{
  */

// Software Reset
#define LAN8710_BCR_SOFT_RESET         ((uint16_t)0x8000U)
// Loopback mode
#define LAN8710_BCR_LOOPBACK           ((uint16_t)0x4000U)
// Speed Select (1=100Mb/s)
#define LAN8710_BCR_SPEED_SELECT       ((uint16_t)0x2000U)
// Auto Negotiation Enable
#define LAN8710_BCR_AUTONEGO_EN        ((uint16_t)0x1000U)
// LAN8700 Power Down
#define LAN8710_BCR_POWER_DOWN         ((uint16_t)0x0800U)
// Isolate Media interface
#define LAN8710_BCR_ISOLATE            ((uint16_t)0x0400U)
// Restart Auto Negotiation
#define LAN8710_BCR_RESTART_AUTONEGO   ((uint16_t)0x0200U)
// Duplex Mode (1=Full duplex)
#define LAN8710_BCR_DUPLEX_MODE        ((uint16_t)0x0100U)
// Enable Collision Test
#define LAN8710_BCR_COL_TEST           ((uint16_t)0x0080U)
/**
  * @}
  */

/** @defgroup LAN8710_BSR_Bit_Definition LAN8710 BSR Bit Definition
  * @{
  */   
// 100BASE-T4 Capable
#define LAN8710_BSR_100BASE_T4       ((uint16_t)0x8000U)//15
// 100BASE-TX Full Duplex Capable
#define LAN8710_BSR_100BASE_TX_FD    ((uint16_t)0x4000U)//14
// 100BASE-TX Half Duplex Capable
#define LAN8710_BSR_100BASE_TX_HD    ((uint16_t)0x2000U)//13
// 10BASE-T Full Duplex Capable
#define LAN8710_BSR_10BASE_T_FD      ((uint16_t)0x1000U)//12
// 10BASE-T Half Duplex Capable
#define LAN8710_BSR_10BASE_T_HD      ((uint16_t)0x0800U)//11
// 100BASE-T2 Full Duplex Capable
#define LAN8710_BSR_100BASE_T2_FD    ((uint16_t)0x0400U)//10
//100BASE-T2 Half Duplex Capable
#define LAN8710_BSR_100BASE_T2_HD    ((uint16_t)0x0200U)//9
// Extended Status (in register 15)
#define LAN8710_BSR_EXTENDED_STATUS  ((uint16_t)0x0100U)//8
// Auto Negotiation Complete 
#define LAN8710_BSR_AUTONEGO_CPLT    ((uint16_t)0x0020U)//5
// Remote Fault  
#define LAN8710_BSR_REMOTE_FAULT     ((uint16_t)0x0010U)//4
// Auto Negotiation Ability
#define LAN8710_BSR_AUTONEGO_ABILITY ((uint16_t)0x0008U)//3
// Link Status (1=link up) 
#define LAN8710_BSR_LINK_STATUS      ((uint16_t)0x0004U)//2
// Jaber Detect
#define LAN8710_BSR_JABBER_DETECT    ((uint16_t)0x0002U)//1
// Extended Capability
#define LAN8710_BSR_EXTENDED_CAP     ((uint16_t)0x0001U)//0
/**
  * @}
  */

/** @defgroup LAN8710_PHYI1R_Bit_Definition LAN8710 PHYI1R Bit Definition
  * @{
  */
#define LAN8710_PHYI1R_OUI_3_18           ((uint16_t)0xFFFFU)
/**
  * @}
  */

/** @defgroup LAN8710_PHYI2R_Bit_Definition LAN8710 PHYI2R Bit Definition
  * @{
  */
#define LAN8710_PHYI2R_OUI_19_24          ((uint16_t)0xFC00U)
#define LAN8710_PHYI2R_MODEL_NBR          ((uint16_t)0x03F0U)
#define LAN8710_PHYI2R_REVISION_NBR       ((uint16_t)0x000FU)
/**
  * @}
  */

/** @defgroup LAN8710_ANAR_Bit_Definition LAN8710 ANAR Bit Definition
  * @{
  */
#define LAN8710_ANAR_NEXT_PAGE               ((uint16_t)0x8000U)
#define LAN8710_ANAR_REMOTE_FAULT            ((uint16_t)0x2000U)
#define LAN8710_ANAR_PAUSE_OPERATION         ((uint16_t)0x0C00U)
#define LAN8710_ANAR_PO_NOPAUSE              ((uint16_t)0x0000U)
#define LAN8710_ANAR_PO_SYMMETRIC_PAUSE      ((uint16_t)0x0400U)
#define LAN8710_ANAR_PO_ASYMMETRIC_PAUSE     ((uint16_t)0x0800U)
#define LAN8710_ANAR_PO_ADVERTISE_SUPPORT    ((uint16_t)0x0C00U)
#define LAN8710_ANAR_100BASE_TX_FD           ((uint16_t)0x0100U)
#define LAN8710_ANAR_100BASE_TX              ((uint16_t)0x0080U)
#define LAN8710_ANAR_10BASE_T_FD             ((uint16_t)0x0040U)
#define LAN8710_ANAR_10BASE_T                ((uint16_t)0x0020U)
#define LAN8710_ANAR_SELECTOR_FIELD          ((uint16_t)0x000FU)
/**
  * @}
  */

/** @defgroup LAN8710_ANLPAR_Bit_Definition LAN8710 ANLPAR Bit Definition
  * @{
  */
#define LAN8710_ANLPAR_NEXT_PAGE            ((uint16_t)0x8000U)
#define LAN8710_ANLPAR_REMOTE_FAULT         ((uint16_t)0x2000U)
#define LAN8710_ANLPAR_PAUSE_OPERATION      ((uint16_t)0x0C00U)
#define LAN8710_ANLPAR_PO_NOPAUSE           ((uint16_t)0x0000U)
#define LAN8710_ANLPAR_PO_SYMMETRIC_PAUSE   ((uint16_t)0x0400U)
#define LAN8710_ANLPAR_PO_ASYMMETRIC_PAUSE  ((uint16_t)0x0800U)
#define LAN8710_ANLPAR_PO_ADVERTISE_SUPPORT ((uint16_t)0x0C00U)
#define LAN8710_ANLPAR_100BASE_TX_FD        ((uint16_t)0x0100U)
#define LAN8710_ANLPAR_100BASE_TX           ((uint16_t)0x0080U)
#define LAN8710_ANLPAR_10BASE_T_FD          ((uint16_t)0x0040U)
#define LAN8710_ANLPAR_10BASE_T             ((uint16_t)0x0020U)
#define LAN8710_ANLPAR_SELECTOR_FIELD       ((uint16_t)0x000FU)
/**
  * @}
  */

/** @defgroup LAN8710_ANER_Bit_Definition LAN8710 ANER Bit Definition
  * @{
  */
#define LAN8710_ANER_RX_NP_LOCATION_ABLE    ((uint16_t)0x0040U)
#define LAN8710_ANER_RX_NP_STORAGE_LOCATION ((uint16_t)0x0020U)
#define LAN8710_ANER_PARALLEL_DETECT_FAULT  ((uint16_t)0x0010U)
#define LAN8710_ANER_LP_NP_ABLE             ((uint16_t)0x0008U)
#define LAN8710_ANER_NP_ABLE                ((uint16_t)0x0004U)
#define LAN8710_ANER_PAGE_RECEIVED          ((uint16_t)0x0002U)
#define LAN8710_ANER_LP_AUTONEG_ABLE        ((uint16_t)0x0001U)
/**
  * @}
  */

/** @defgroup LAN8710_ANNPTR_Bit_Definition LAN8710 ANNPTR Bit Definition
  * @{
  */
#define LAN8710_ANNPTR_NEXT_PAGE         ((uint16_t)0x8000U)
#define LAN8710_ANNPTR_MESSAGE_PAGE      ((uint16_t)0x2000U)
#define LAN8710_ANNPTR_ACK2              ((uint16_t)0x1000U)
#define LAN8710_ANNPTR_TOGGLE            ((uint16_t)0x0800U)
#define LAN8710_ANNPTR_MESSAGGE_CODE     ((uint16_t)0x07FFU)
/**
  * @}
  */

/** @defgroup LAN8710_ANNPRR_Bit_Definition LAN8710 ANNPRR Bit Definition
  * @{
  */
#define LAN8710_ANNPTR_NEXT_PAGE         ((uint16_t)0x8000U)
#define LAN8710_ANNPRR_ACK               ((uint16_t)0x4000U)
#define LAN8710_ANNPRR_MESSAGE_PAGE      ((uint16_t)0x2000U)
#define LAN8710_ANNPRR_ACK2              ((uint16_t)0x1000U)
#define LAN8710_ANNPRR_TOGGLE            ((uint16_t)0x0800U)
#define LAN8710_ANNPRR_MESSAGGE_CODE     ((uint16_t)0x07FFU)
/**
  * @}
  */

/** @defgroup LAN8710_MMDACR_Bit_Definition LAN8710 MMDACR Bit Definition
  * @{
  */
#define LAN8710_MMDACR_MMD_FUNCTION       ((uint16_t)0xC000U) 
#define LAN8710_MMDACR_MMD_FUNCTION_ADDR  ((uint16_t)0x0000U)
#define LAN8710_MMDACR_MMD_FUNCTION_DATA  ((uint16_t)0x4000U)
#define LAN8710_MMDACR_MMD_DEV_ADDR       ((uint16_t)0x001FU)
/**
  * @}
  */

/** @defgroup LAN8710_ENCTR_Bit_Definition LAN8710 ENCTR Bit Definition
  * @{
  */
#define LAN8710_ENCTR_TX_ENABLE             ((uint16_t)0x8000U)
#define LAN8710_ENCTR_TX_TIMER              ((uint16_t)0x6000U)
#define LAN8710_ENCTR_TX_TIMER_1S           ((uint16_t)0x0000U)
#define LAN8710_ENCTR_TX_TIMER_768MS        ((uint16_t)0x2000U)
#define LAN8710_ENCTR_TX_TIMER_512MS        ((uint16_t)0x4000U)
#define LAN8710_ENCTR_TX_TIMER_265MS        ((uint16_t)0x6000U)
#define LAN8710_ENCTR_RX_ENABLE             ((uint16_t)0x1000U)
#define LAN8710_ENCTR_RX_MAX_INTERVAL       ((uint16_t)0x0C00U)
#define LAN8710_ENCTR_RX_MAX_INTERVAL_64MS  ((uint16_t)0x0000U)
#define LAN8710_ENCTR_RX_MAX_INTERVAL_256MS ((uint16_t)0x0400U)
#define LAN8710_ENCTR_RX_MAX_INTERVAL_512MS ((uint16_t)0x0800U)
#define LAN8710_ENCTR_RX_MAX_INTERVAL_1S    ((uint16_t)0x0C00U)
#define LAN8710_ENCTR_EX_CROSS_OVER         ((uint16_t)0x0002U)
#define LAN8710_ENCTR_EX_MANUAL_CROSS_OVER  ((uint16_t)0x0001U)
/**
  * @}
  */

/** @defgroup LAN8710_MCSR_Bit_Definition LAN8710 MCSR Bit Definition
  * @{
  */
#define LAN8710_MCSR_EDPWRDOWN        ((uint16_t)0x2000U)
#define LAN8710_MCSR_FARLOOPBACK      ((uint16_t)0x0200U)
#define LAN8710_MCSR_ALTINT           ((uint16_t)0x0040U)
#define LAN8710_MCSR_ENERGYON         ((uint16_t)0x0002U)
/**
  * @}
  */

/** @defgroup LAN8710_SMR_Bit_Definition LAN8710 SMR Bit Definition
  * @{
  */
    
#define LAN8710_SMR_MIIMODE    ((uint16_t)0x4000)
// Transceiver mode of operation   
#define LAN8710_SMR_MODE       ((uint16_t)0x00E0U)
// PHY address
#define LAN8710_SMR_PHY_ADDR   ((uint16_t)0x001FU)
/**
  * @}
  */

/** @defgroup LAN8710_TPDCR_Bit_Definition LAN8710 TPDCR Bit Definition
  * @{
  */
#define LAN8710_TPDCR_DELAY_IN                 ((uint16_t)0x8000U)
#define LAN8710_TPDCR_LINE_BREAK_COUNTER       ((uint16_t)0x7000U)
#define LAN8710_TPDCR_PATTERN_HIGH             ((uint16_t)0x0FC0U)
#define LAN8710_TPDCR_PATTERN_LOW              ((uint16_t)0x003FU)
/**
  * @}
  */

/** @defgroup LAN8710_TCSR_Bit_Definition LAN8710 TCSR Bit Definition
  * @{
  */
#define LAN8710_TCSR_TDR_ENABLE           ((uint16_t)0x8000U)
#define LAN8710_TCSR_TDR_AD_FILTER_ENABLE ((uint16_t)0x4000U)
#define LAN8710_TCSR_TDR_CH_CABLE_TYPE    ((uint16_t)0x0600U)
#define LAN8710_TCSR_TDR_CH_CABLE_DEFAULT ((uint16_t)0x0000U)
#define LAN8710_TCSR_TDR_CH_CABLE_SHORTED ((uint16_t)0x0200U)
#define LAN8710_TCSR_TDR_CH_CABLE_OPEN    ((uint16_t)0x0400U)
#define LAN8710_TCSR_TDR_CH_CABLE_MATCH   ((uint16_t)0x0600U)
#define LAN8710_TCSR_TDR_CH_STATUS        ((uint16_t)0x0100U)
#define LAN8710_TCSR_TDR_CH_LENGTH        ((uint16_t)0x00FFU)
/**
  * @}
  */

/** @defgroup LAN8710_SCSIR_Bit_Definition LAN8710 SCSIR Bit Definition
  * @{
  */
#define LAN8710_SCSIR_AUTO_MDIX_ENABLE    ((uint16_t)0x8000U)
#define LAN8710_SCSIR_CHANNEL_SELECT      ((uint16_t)0x2000U)
#define LAN8710_SCSIR_SQE_DISABLE         ((uint16_t)0x0800U)
#define LAN8710_SCSIR_XPOLALITY           ((uint16_t)0x0010U)
/**
  * @}
  */

/** @defgroup LAN8710_CLR_Bit_Definition LAN8710 CLR Bit Definition
  * @{
  */
#define LAN8710_CLR_CABLE_LENGTH       ((uint16_t)0xF000U)
/**
  * @}
  */

/** @defgroup LAN8710_IMR_ISFR_Bit_Definition LAN8710 IMR ISFR Bit Definition
  * @{
  */

#define LAN8710_INT_7       ((uint16_t)0x0080U)
#define LAN8710_INT_6       ((uint16_t)0x0040U)
#define LAN8710_INT_5       ((uint16_t)0x0020U)
#define LAN8710_INT_4       ((uint16_t)0x0010U)
#define LAN8710_INT_3       ((uint16_t)0x0008U)
#define LAN8710_INT_2       ((uint16_t)0x0004U)
#define LAN8710_INT_1       ((uint16_t)0x0002U)
/**
  * @}
  */

/** @defgroup LAN8710_PHYSCSR_Bit_Definition LAN8710 PHYSCSR Bit Definition
  * @{
  */
#define LAN8710_PHYSCSR_AUTONEGO_DONE   ((uint16_t)0x1000U)
#define LAN8710_PHYSCSR_HCDSPEEDMASK    ((uint16_t)0x001CU)
#define LAN8710_PHYSCSR_10BT_HD         ((uint16_t)0x0004U)
#define LAN8710_PHYSCSR_10BT_FD         ((uint16_t)0x0014U)
#define LAN8710_PHYSCSR_100BTX_HD       ((uint16_t)0x0008U)
#define LAN8710_PHYSCSR_100BTX_FD       ((uint16_t)0x0018U)
/**
  * @}
  */
    
/** @defgroup LAN8710_Status LAN8710 Status
  * @{
  */    

#define  LAN8710_STATUS_READ_ERROR            ((int32_t)-5)
#define  LAN8710_STATUS_WRITE_ERROR           ((int32_t)-4)
#define  LAN8710_STATUS_ADDRESS_ERROR         ((int32_t)-3)
#define  LAN8710_STATUS_RESET_TIMEOUT         ((int32_t)-2)
#define  LAN8710_STATUS_ERROR                 ((int32_t)-1)
#define  LAN8710_STATUS_OK                    ((int32_t) 0)
#define  LAN8710_STATUS_LINK_DOWN             ((int32_t) 1)
#define  LAN8710_STATUS_100MBITS_FULLDUPLEX   ((int32_t) 2)
#define  LAN8710_STATUS_100MBITS_HALFDUPLEX   ((int32_t) 3)
#define  LAN8710_STATUS_10MBITS_FULLDUPLEX    ((int32_t) 4)
#define  LAN8710_STATUS_10MBITS_HALFDUPLEX    ((int32_t) 5)
#define  LAN8710_STATUS_AUTONEGO_NOTDONE      ((int32_t) 6)
/**
  * @}
  */

/** @defgroup LAN8710_IT_Flags LAN8710 IT Flags
  * @{
  */     
#define  LAN8710_WOL_IT                        LAN8710_INT_8
#define  LAN8710_ENERGYON_IT                   LAN8710_INT_7
#define  LAN8710_AUTONEGO_COMPLETE_IT          LAN8710_INT_6
#define  LAN8710_REMOTE_FAULT_IT               LAN8710_INT_5
#define  LAN8710_LINK_DOWN_IT                  LAN8710_INT_4
#define  LAN8710_AUTONEGO_LP_ACK_IT            LAN8710_INT_3
#define  LAN8710_PARALLEL_DETECTION_FAULT_IT   LAN8710_INT_2
#define  LAN8710_AUTONEGO_PAGE_RECEIVED_IT     LAN8710_INT_1
/**
  * @}
  */

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/ 
/** @defgroup LAN8710_Exported_Types LAN8710 Exported Types
  * @{
  */
typedef int32_t  (*lan8710_Init_Func) (void); 
typedef int32_t  (*lan8710_DeInit_Func) (void);
typedef int32_t  (*lan8710_ReadReg_Func)   (uint32_t, uint32_t, uint32_t *);
typedef int32_t  (*lan8710_WriteReg_Func)  (uint32_t, uint32_t, uint32_t);
typedef int32_t  (*lan8710_GetTick_Func)  (void);

typedef struct 
{                   
  lan8710_Init_Func      Init; 
  lan8710_DeInit_Func    DeInit;
  lan8710_WriteReg_Func  WriteReg;
  lan8710_ReadReg_Func   ReadReg; 
  lan8710_GetTick_Func   GetTick;   
} lan8710_IOCtx_t;  

  
typedef struct 
{
  uint32_t            DevAddr;
  uint32_t            Is_Initialized;
  lan8710_IOCtx_t     IO;
  void               *pData;
}lan8710_Object_t;
/**
  * @}
  */ 

/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/** @defgroup LAN8710_Exported_Functions LAN8710 Exported Functions
  * @{
  */
int32_t LAN8710_RegisterBusIO(lan8710_Object_t *pObj, lan8710_IOCtx_t *ioctx);
int32_t LAN8710_Init(lan8710_Object_t *pObj);
int32_t LAN8710_DeInit(lan8710_Object_t *pObj);
int32_t LAN8710_DisablePowerDownMode(lan8710_Object_t *pObj);
int32_t LAN8710_EnablePowerDownMode(lan8710_Object_t *pObj);
int32_t LAN8710_StartAutoNego(lan8710_Object_t *pObj);
int32_t LAN8710_GetLinkState(lan8710_Object_t *pObj);
int32_t LAN8710_SetLinkState(lan8710_Object_t *pObj, uint32_t LinkState);
int32_t LAN8710_EnableLoopbackMode(lan8710_Object_t *pObj);
int32_t LAN8710_DisableLoopbackMode(lan8710_Object_t *pObj);
int32_t LAN8710_EnableIT(lan8710_Object_t *pObj, uint32_t Interrupt);
int32_t LAN8710_DisableIT(lan8710_Object_t *pObj, uint32_t Interrupt);
int32_t LAN8710_ClearIT(lan8710_Object_t *pObj, uint32_t Interrupt);
int32_t LAN8710_GetITStatus(lan8710_Object_t *pObj, uint32_t Interrupt);
/**
  * @}
  */ 

#ifdef __cplusplus
}
#endif
#endif /* LAN8710_H */


/**
  * @}
  */ 

/**
  * @}
  */

/**
  * @}
  */ 

/**
  * @}
  */       
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
