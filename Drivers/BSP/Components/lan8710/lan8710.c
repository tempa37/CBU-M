/** 
  ******************************************************************************
  * @file    lan8710.c
  * @author  MCD Application Team
  * @brief   This file provides a set of functions needed to manage the LAN742
  *          PHY devices.
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

/* Includes ------------------------------------------------------------------*/
#include "lan8710.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup Component
  * @{
  */ 
  
/** @defgroup LAN8710 LAN8710
  * @{
  */   
  
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup LAN8710_Private_Defines LAN8710 Private Defines
  * @{
  */
#define LAN8710_SW_RESET_TO    ((uint32_t)500U)
#define LAN8710_INIT_TO        ((uint32_t)3000U)
#define LAN8710_MAX_DEV_ADDR   ((uint32_t)31U)
/**
  * @}
  */
 
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/** @defgroup LAN8710_Private_Functions LAN8710 Private Functions
  * @{
  */
    
/**
  * @brief  Register IO functions to component object
  * @param  pObj: device object  of LAN8710_Object_t. 
  * @param  ioctx: holds device IO functions.  
  * @retval LAN8710_STATUS_OK  if OK
  *         LAN8710_STATUS_ERROR if missing mandatory function
  */
int32_t LAN8710_RegisterBusIO(lan8710_Object_t *pObj, lan8710_IOCtx_t *ioctx)
{
  if(!pObj || !ioctx->ReadReg || !ioctx->WriteReg || !ioctx->GetTick)
  {
    return LAN8710_STATUS_ERROR;
  }
  
  pObj->IO.Init = ioctx->Init;
  pObj->IO.DeInit = ioctx->DeInit;
  pObj->IO.ReadReg = ioctx->ReadReg;
  pObj->IO.WriteReg = ioctx->WriteReg;
  pObj->IO.GetTick = ioctx->GetTick;
  
  return LAN8710_STATUS_OK;
}

/**
  * @brief  Initialize the lan8710 and configure the needed hardware resources
  * @param  pObj: device object LAN8710_Object_t. 
  * @retval LAN8710_STATUS_OK  if OK
  *         LAN8710_STATUS_ADDRESS_ERROR if cannot find device address
  *         LAN8710_STATUS_READ_ERROR if connot read register
  *         LAN8710_STATUS_WRITE_ERROR if connot write to register
  *         LAN8710_STATUS_RESET_TIMEOUT if cannot perform a software reset
  */
 int32_t LAN8710_Init(lan8710_Object_t *pObj)
 {
   //pObj->DevAddr = 7;  
   
   uint32_t tickstart = 0, regvalue = 0, addr = 0;

   int32_t status = LAN8710_STATUS_OK;

   if(pObj->Is_Initialized == 0)
   {
     if(pObj->IO.Init != 0)
     {
       // GPIO and Clocks initialization
       pObj->IO.Init();
     }
   
     // for later check
     pObj->DevAddr = LAN8710_MAX_DEV_ADDR + 1;
   
     // Get the device address from special mode register 
     for(addr = 0; addr <= LAN8710_MAX_DEV_ADDR; addr ++)
     {
       if(pObj->IO.ReadReg(addr, LAN8710_SMR, &regvalue) < 0)
       { 
         status = LAN8710_STATUS_READ_ERROR;
         // Can't read from this device address 
         //   continue with next address
         continue;
       }
     
       if((regvalue & LAN8710_SMR_PHY_ADDR) == addr)
       {
         pObj->DevAddr = addr;
         status = LAN8710_STATUS_OK;
         break;
       }
     }
   
     if(pObj->DevAddr > LAN8710_MAX_DEV_ADDR)
     {
       status = LAN8710_STATUS_ADDRESS_ERROR;
     }     
     
     // if device address is matched
     if(status == LAN8710_STATUS_OK)
     {
       // set a software reset
       if(pObj->IO.WriteReg(pObj->DevAddr, LAN8710_BCR, LAN8710_BCR_SOFT_RESET) >= 0)
       { 
         // get software reset status 
         if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_BCR, &regvalue) >= 0)
         { 
           tickstart = pObj->IO.GetTick();
           
           // wait until software reset is done or timeout occured 
           while(regvalue & LAN8710_BCR_SOFT_RESET)
           {
             if((pObj->IO.GetTick() - tickstart) <= LAN8710_SW_RESET_TO)
             {
               if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_BCR, &regvalue) < 0)
               { 
                 status = LAN8710_STATUS_READ_ERROR;
                 break;
               }
             }
             else
             {
               status = LAN8710_STATUS_RESET_TIMEOUT;
               break;
             }
           } 
         }
         else
         {
           status = LAN8710_STATUS_READ_ERROR;
         }
       }
       else
       {
         status = LAN8710_STATUS_WRITE_ERROR;
       }
     }     
   }
   
   //pObj->IO.ReadReg(pObj->DevAddr, LAN8710_SMR, &regvalue);
   
   //regvalue |= LAN8710_SMR_MIIMODE;
   
   //pObj->IO.WriteReg(pObj->DevAddr, LAN8710_SMR, regvalue);
   
   pObj->IO.WriteReg(pObj->DevAddr, LAN8710_BCR, LAN8710_BCR_AUTONEGO_EN);
   
   if(status == LAN8710_STATUS_OK)
   {     
     tickstart =  pObj->IO.GetTick();
     
     // Wait for 2s to perform initialization
     while((pObj->IO.GetTick() - tickstart) <= LAN8710_INIT_TO)
     {
     }     
     pObj->Is_Initialized = 1;
   }
       
   //pObj->IO.WriteReg(pObj->DevAddr, LAN8710_BCR, LAN8710_BCR_RESTART_AUTONEGO);
   
   return status;
 }

/**
  * @brief  De-Initialize the lan8710 and it's hardware resources
  * @param  pObj: device object LAN8710_Object_t. 
  * @retval None
  */
int32_t LAN8710_DeInit(lan8710_Object_t *pObj)
{
  if(pObj->Is_Initialized)
  {
    if(pObj->IO.DeInit != 0)
    {
      if(pObj->IO.DeInit() < 0)
      {
        return LAN8710_STATUS_ERROR;
      }
    }
  
    pObj->Is_Initialized = 0;  
  }
  
  return LAN8710_STATUS_OK;
}

/**
  * @brief  Disable the LAN8710 power down mode.
  * @param  pObj: device object LAN8710_Object_t.  
  * @retval LAN8710_STATUS_OK  if OK
  *         LAN8710_STATUS_READ_ERROR if connot read register
  *         LAN8710_STATUS_WRITE_ERROR if connot write to register
  */
int32_t LAN8710_DisablePowerDownMode(lan8710_Object_t *pObj)
{
  uint32_t readval = 0;
  int32_t status = LAN8710_STATUS_OK;
  
  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_BCR, &readval) >= 0)
  {
    readval &= ~LAN8710_BCR_POWER_DOWN;
  
    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, LAN8710_BCR, readval) < 0)
    {
      status =  LAN8710_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = LAN8710_STATUS_READ_ERROR;
  }
   
  return status;
}

/**
  * @brief  Enable the LAN8710 power down mode.
  * @param  pObj: device object LAN8710_Object_t.  
  * @retval LAN8710_STATUS_OK  if OK
  *         LAN8710_STATUS_READ_ERROR if connot read register
  *         LAN8710_STATUS_WRITE_ERROR if connot write to register
  */
int32_t LAN8710_EnablePowerDownMode(lan8710_Object_t *pObj)
{
  uint32_t readval = 0;
  int32_t status = LAN8710_STATUS_OK;
  
  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_BCR, &readval) >= 0)
  {
    readval |= LAN8710_BCR_POWER_DOWN;
  
    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, LAN8710_BCR, readval) < 0)
    {
      status =  LAN8710_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = LAN8710_STATUS_READ_ERROR;
  }
   
  return status;
}

/**
  * @brief  Start the auto negotiation process.
  * @param  pObj: device object LAN8710_Object_t.  
  * @retval LAN8710_STATUS_OK  if OK
  *         LAN8710_STATUS_READ_ERROR if connot read register
  *         LAN8710_STATUS_WRITE_ERROR if connot write to register
  */
int32_t LAN8710_StartAutoNego(lan8710_Object_t *pObj)
{
  uint32_t readval = 0;
  int32_t status = LAN8710_STATUS_OK;
  
  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_BCR, &readval) >= 0)
  {
    readval |= LAN8710_BCR_RESTART_AUTONEGO;
  
    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, LAN8710_BCR, readval) < 0)
    {
      status =  LAN8710_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = LAN8710_STATUS_READ_ERROR;
  }
   
  return status;
}

/**
  * @brief  Get the link state of LAN8710 device.
  * @param  pObj: Pointer to device object. 
  * @param  pLinkState: Pointer to link state
  * @retval LAN8710_STATUS_LINK_DOWN  if link is down
  *         LAN8710_STATUS_AUTONEGO_NOTDONE if Auto nego not completed 
  *         LAN8710_STATUS_100MBITS_FULLDUPLEX if 100Mb/s FD
  *         LAN8710_STATUS_100MBITS_HALFDUPLEX if 100Mb/s HD
  *         LAN8710_STATUS_10MBITS_FULLDUPLEX  if 10Mb/s FD
  *         LAN8710_STATUS_10MBITS_HALFDUPLEX  if 10Mb/s HD       
  *         LAN8710_STATUS_READ_ERROR if connot read register
  *         LAN8710_STATUS_WRITE_ERROR if connot write to register
  */
int32_t LAN8710_GetLinkState(lan8710_Object_t *pObj)
{
  uint32_t readval = 0;
  
  /* Read Status register  */
  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_BSR, &readval) < 0)
  {
    return LAN8710_STATUS_READ_ERROR;
  }
/*  
  // Read Status register again
  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_BSR, &readval) < 0)
  {
    return LAN8710_STATUS_READ_ERROR;
  }
*/
  
  if((readval & LAN8710_BSR_LINK_STATUS) == 0)
  {
    /* Return Link Down status */
    return LAN8710_STATUS_LINK_DOWN;    
  }
  
  /* Check Auto negotiaition */
  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_BCR, &readval) < 0)
  {
    return LAN8710_STATUS_READ_ERROR;
  }
  
  if((readval & LAN8710_BCR_AUTONEGO_EN) != LAN8710_BCR_AUTONEGO_EN)
  {
    if(((readval & LAN8710_BCR_SPEED_SELECT) == LAN8710_BCR_SPEED_SELECT) && ((readval & LAN8710_BCR_DUPLEX_MODE) == LAN8710_BCR_DUPLEX_MODE)) 
    {
      return LAN8710_STATUS_100MBITS_FULLDUPLEX;
    }
    else if ((readval & LAN8710_BCR_SPEED_SELECT) == LAN8710_BCR_SPEED_SELECT)
    {
      return LAN8710_STATUS_100MBITS_HALFDUPLEX;
    }        
    else if ((readval & LAN8710_BCR_DUPLEX_MODE) == LAN8710_BCR_DUPLEX_MODE)
    {
      return LAN8710_STATUS_10MBITS_FULLDUPLEX;
    }
    else
    {
      return LAN8710_STATUS_10MBITS_HALFDUPLEX;
    }  		
  }
  else /* Auto Nego enabled */
  {  
    if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_PHYSCSR, &readval) < 0)
    {
      return LAN8710_STATUS_READ_ERROR;
    }    
    /* Check if auto nego not done */
    if((readval & LAN8710_PHYSCSR_AUTONEGO_DONE) == 0)
    {
      return LAN8710_STATUS_AUTONEGO_NOTDONE;
    }    
    if((readval & LAN8710_PHYSCSR_HCDSPEEDMASK) == LAN8710_PHYSCSR_100BTX_FD)
    {
      return LAN8710_STATUS_100MBITS_FULLDUPLEX;
    }
    else if ((readval & LAN8710_PHYSCSR_HCDSPEEDMASK) == LAN8710_PHYSCSR_100BTX_HD)
    {
      return LAN8710_STATUS_100MBITS_HALFDUPLEX;
    }
    else if ((readval & LAN8710_PHYSCSR_HCDSPEEDMASK) == LAN8710_PHYSCSR_10BT_FD)
    {
      return LAN8710_STATUS_10MBITS_FULLDUPLEX;
    }
    else
    {
      return LAN8710_STATUS_10MBITS_HALFDUPLEX;
    }				
  }
}

/**
  * @brief  Set the link state of LAN8710 device.
  * @param  pObj: Pointer to device object. 
  * @param  pLinkState: link state can be one of the following
  *         LAN8710_STATUS_100MBITS_FULLDUPLEX if 100Mb/s FD
  *         LAN8710_STATUS_100MBITS_HALFDUPLEX if 100Mb/s HD
  *         LAN8710_STATUS_10MBITS_FULLDUPLEX  if 10Mb/s FD
  *         LAN8710_STATUS_10MBITS_HALFDUPLEX  if 10Mb/s HD   
  * @retval LAN8710_STATUS_OK  if OK
  *         LAN8710_STATUS_ERROR  if parameter error  
  *         LAN8710_STATUS_READ_ERROR if connot read register
  *         LAN8710_STATUS_WRITE_ERROR if connot write to register
  */
int32_t LAN8710_SetLinkState(lan8710_Object_t *pObj, uint32_t LinkState)
{
  uint32_t bcrvalue = 0;
  int32_t status = LAN8710_STATUS_OK;
  
  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_BCR, &bcrvalue) >= 0)
  {
    /* Disable link config (Auto nego, speed and duplex) */
    bcrvalue &= ~(LAN8710_BCR_AUTONEGO_EN | LAN8710_BCR_SPEED_SELECT | LAN8710_BCR_DUPLEX_MODE);
    
    if(LinkState == LAN8710_STATUS_100MBITS_FULLDUPLEX)
    {
      bcrvalue |= (LAN8710_BCR_SPEED_SELECT | LAN8710_BCR_DUPLEX_MODE);
    }
    else if (LinkState == LAN8710_STATUS_100MBITS_HALFDUPLEX)
    {
      bcrvalue |= LAN8710_BCR_SPEED_SELECT;
    }
    else if (LinkState == LAN8710_STATUS_10MBITS_FULLDUPLEX)
    {
      bcrvalue |= LAN8710_BCR_DUPLEX_MODE;
    }
    else
    {
      /* Wrong link status parameter */
      status = LAN8710_STATUS_ERROR;
    }	
  }
  else
  {
    status = LAN8710_STATUS_READ_ERROR;
  }
  
  if(status == LAN8710_STATUS_OK)
  {
    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, LAN8710_BCR, bcrvalue) < 0)
    {
      status = LAN8710_STATUS_WRITE_ERROR;
    }
  }
  
  return status;
}

/**
  * @brief  Enable loopback mode.
  * @param  pObj: Pointer to device object. 
  * @retval LAN8710_STATUS_OK  if OK
  *         LAN8710_STATUS_READ_ERROR if connot read register
  *         LAN8710_STATUS_WRITE_ERROR if connot write to register
  */
int32_t LAN8710_EnableLoopbackMode(lan8710_Object_t *pObj)
{
  uint32_t readval = 0;
  int32_t status = LAN8710_STATUS_OK;
  
  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_BCR, &readval) >= 0)
  {
    readval |= LAN8710_BCR_LOOPBACK;
    
    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, LAN8710_BCR, readval) < 0)
    {
      status = LAN8710_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = LAN8710_STATUS_READ_ERROR;
  }
  
  return status;
}

/**
  * @brief  Disable loopback mode.
  * @param  pObj: Pointer to device object. 
  * @retval LAN8710_STATUS_OK  if OK
  *         LAN8710_STATUS_READ_ERROR if connot read register
  *         LAN8710_STATUS_WRITE_ERROR if connot write to register
  */
int32_t LAN8710_DisableLoopbackMode(lan8710_Object_t *pObj)
{
  uint32_t readval = 0;
  int32_t status = LAN8710_STATUS_OK;
  
  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_BCR, &readval) >= 0)
  {
    readval &= ~LAN8710_BCR_LOOPBACK;
  
    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, LAN8710_BCR, readval) < 0)
    {
      status =  LAN8710_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = LAN8710_STATUS_READ_ERROR;
  }
   
  return status;
}

/**
  * @brief  Enable IT source.
  * @param  pObj: Pointer to device object. 
  * @param  Interrupt: IT source to be enabled
  *         should be a value or a combination of the following:
  *         LAN8710_WOL_IT                     
  *         LAN8710_ENERGYON_IT                
  *         LAN8710_AUTONEGO_COMPLETE_IT       
  *         LAN8710_REMOTE_FAULT_IT            
  *         LAN8710_LINK_DOWN_IT               
  *         LAN8710_AUTONEGO_LP_ACK_IT         
  *         LAN8710_PARALLEL_DETECTION_FAULT_IT
  *         LAN8710_AUTONEGO_PAGE_RECEIVED_IT
  * @retval LAN8710_STATUS_OK  if OK
  *         LAN8710_STATUS_READ_ERROR if connot read register
  *         LAN8710_STATUS_WRITE_ERROR if connot write to register
  */
int32_t LAN8710_EnableIT(lan8710_Object_t *pObj, uint32_t Interrupt)
{
  uint32_t readval = 0;
  int32_t status = LAN8710_STATUS_OK;
  
  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_IMR, &readval) >= 0)
  {
    readval |= Interrupt;
  
    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, LAN8710_IMR, readval) < 0)
    {
      status =  LAN8710_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = LAN8710_STATUS_READ_ERROR;
  }
   
  return status;
}

/**
  * @brief  Disable IT source.
  * @param  pObj: Pointer to device object. 
  * @param  Interrupt: IT source to be disabled
  *         should be a value or a combination of the following:
  *         LAN8710_WOL_IT                     
  *         LAN8710_ENERGYON_IT                
  *         LAN8710_AUTONEGO_COMPLETE_IT       
  *         LAN8710_REMOTE_FAULT_IT            
  *         LAN8710_LINK_DOWN_IT               
  *         LAN8710_AUTONEGO_LP_ACK_IT         
  *         LAN8710_PARALLEL_DETECTION_FAULT_IT
  *         LAN8710_AUTONEGO_PAGE_RECEIVED_IT
  * @retval LAN8710_STATUS_OK  if OK
  *         LAN8710_STATUS_READ_ERROR if connot read register
  *         LAN8710_STATUS_WRITE_ERROR if connot write to register
  */
int32_t LAN8710_DisableIT(lan8710_Object_t *pObj, uint32_t Interrupt)
{
  uint32_t readval = 0;
  int32_t status = LAN8710_STATUS_OK;
  
  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_IMR, &readval) >= 0)
  {
    readval &= ~Interrupt;
  
    /* Apply configuration */
    if(pObj->IO.WriteReg(pObj->DevAddr, LAN8710_IMR, readval) < 0)
    {
      status = LAN8710_STATUS_WRITE_ERROR;
    }
  }
  else
  {
    status = LAN8710_STATUS_READ_ERROR;
  }
   
  return status;
}

/**
  * @brief  Clear IT flag.
  * @param  pObj: Pointer to device object. 
  * @param  Interrupt: IT flag to be cleared
  *         should be a value or a combination of the following:
  *         LAN8710_WOL_IT                     
  *         LAN8710_ENERGYON_IT                
  *         LAN8710_AUTONEGO_COMPLETE_IT       
  *         LAN8710_REMOTE_FAULT_IT            
  *         LAN8710_LINK_DOWN_IT               
  *         LAN8710_AUTONEGO_LP_ACK_IT         
  *         LAN8710_PARALLEL_DETECTION_FAULT_IT
  *         LAN8710_AUTONEGO_PAGE_RECEIVED_IT
  * @retval LAN8710_STATUS_OK  if OK
  *         LAN8710_STATUS_READ_ERROR if connot read register
  */
int32_t  LAN8710_ClearIT(lan8710_Object_t *pObj, uint32_t Interrupt)
{
  uint32_t readval = 0;
  int32_t status = LAN8710_STATUS_OK;  
  
  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_ISFR, &readval) < 0)
  {
    status =  LAN8710_STATUS_READ_ERROR;
  }
  
  return status;
}

/**
  * @brief  Get IT Flag status.
  * @param  pObj: Pointer to device object. 
  * @param  Interrupt: IT Flag to be checked, 
  *         should be a value or a combination of the following:
  *         LAN8710_WOL_IT                     
  *         LAN8710_ENERGYON_IT                
  *         LAN8710_AUTONEGO_COMPLETE_IT       
  *         LAN8710_REMOTE_FAULT_IT            
  *         LAN8710_LINK_DOWN_IT               
  *         LAN8710_AUTONEGO_LP_ACK_IT         
  *         LAN8710_PARALLEL_DETECTION_FAULT_IT
  *         LAN8710_AUTONEGO_PAGE_RECEIVED_IT  
  * @retval 1 IT flag is SET
  *         0 IT flag is RESET
  *         LAN8710_STATUS_READ_ERROR if connot read register
  */
int32_t LAN8710_GetITStatus(lan8710_Object_t *pObj, uint32_t Interrupt)
{
  uint32_t readval = 0;
  int32_t status = 0;

  if(pObj->IO.ReadReg(pObj->DevAddr, LAN8710_ISFR, &readval) >= 0)
  {
    status = ((readval & Interrupt) == Interrupt);
  }
  else
  {
    status = LAN8710_STATUS_READ_ERROR;
  }
	
  return status;
}

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
