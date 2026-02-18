#include "main.h"

#include "string.h"
#include "tim.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ktv.h"

typedef struct sKtvRezult { 
  uint8_t Before; 
  uint8_t Online; 
  uint8_t Break;  
  uint8_t After;  
} tsKtvRezult;

KTVstate State;

uint64_t Bitmap[KTV_BITMAP_SIZE / 64 + 1];

tsKtvElem aKtvElem[KTV_NUM_MAX + 1] = {ksNone, false};

tsKtvRezult aKtvRezult[KTV_NUM_MAX + 1];

int16_t BuffIdx;

int32_t Counter;

#ifdef DEBUG
uint32_t gKtvTickCount;
#endif

void KTV_Start();
uint64_t KTV_GetCurrProf(int iBitmapIdx);
void KTV_ClearElem();
bool KTV_ProcessProf();
bool KTV_Triggered();
void KTV_ProcessKb();
void KTV_ProcessRead();

KTVstate KTV_State() {
  return State;
}

/**
 * @brief Initialization
 *
 */
void KTV_Init() {
#ifdef DEBUG
  gKtvTickCount = 0;
#endif
  memset((void *)Bitmap, 0, sizeof(Bitmap));
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
  Counter = KTV_BOOTUP_INTERVAL / MS_IN_TICK;
  // Start timer, period = 0.5 quantum
  Start_IT_TIM14();
  State = ksStart;
}

/**
 * @brief Starting the process of polling sensors
 *
 */
void KTV_Start() {
#ifdef KTV_START_PULSE
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET);
#endif
  Counter = TICK_NUM_KTV_START;
  State = ksStPulse;
}

/**
 * @brief Get the response profile as a uint64_t
 *
 */
uint64_t KTV_GetCurrProf(int iBitmapIdx) {
  uint64_t cBit = 1;
  uint64_t cMask = (cBit << TICK_NUM_KTV_AREA);
  cMask -= 1;
  uint64_t cProf;
  if (iBitmapIdx < 0) {
    cProf = (Bitmap[0] << -iBitmapIdx);
    cProf &= cMask;
    return cProf;
  }
  int cPos = iBitmapIdx / 64;
  uint64_t cOffset = iBitmapIdx % 64;
  uint64_t cRem = 64 - cOffset;
  uint64_t cRem1 = TICK_NUM_KTV_AREA - cRem;
  uint64_t cMask1;
  cMask = ((cBit << cRem) - 1);

  uint64_t cProf1 = Bitmap[cPos];
  cProf = (cProf1 >> cOffset) & cMask;
  if (cRem1 > 0) {
    cMask1 = ((cBit << cRem1) - 1);
    cProf1 = Bitmap[cPos + 1];
    cProf1 &= cMask1;
    cProf1 = cProf1 << cRem; 
    cProf |= cProf1;
  }
  return cProf;
}

/**
 * @brief 
 *
 */
void KTV_ClearElem() {
  memset((uint8_t *)aKtvRezult, 0, sizeof(aKtvRezult));
}

/**
 * @brief Processing the buffer
 *
 */
bool KTV_ProcessProf() {
  tsKtvElem * cpKtvElem = aKtvElem;
  uint32_t cStart = Bitmap[0];
  uint32_t cStartIdx = 0;
  uint32_t cCount = 0, cBaseStart = 4;
  int32_t cStart1;
  while (cCount < 3) {
    cStart1 = -1;
    for (int i = cBaseStart; i < 24; ++i) {
      if ((cStart & (1 << i)) != 0) {
        cStart1 = i;
        break;
      }
    }
    
    if (cStart1 < 0) {
      cStart1 = 0;
      KTV_ClearElem();
      State = ksNoActive;
      return true;
    }
    cCount = 0;
    for (int i = cStart1; i < 32; ++i) {
      if ((cStart & (1 << i)) != 0) {
        ++cCount;
      } else {
        if ((cStart & (1 << i)) != 0)
          break;
      }
    }
    cBaseStart = cStart1 + cCount;
  }

  cStartIdx = cStart1 + KTV_START_PAUSE_IN_TICKS;
  
  for (int i = 0; i < 32; ++i) {
    if (cStart & (1 << i))
      ++cCount;
  }

  uint64_t cProf, cBit = 1;
  uint8_t cValue;
  bool bChanged = false;
  
  for (int i = 0; i < (KTV_NUM_MAX + 1); ++i) {
    if ((i > KTV_USED_NUM) && (i < KTV_DPP_POS)) {
      continue;
    }

    cValue = cpKtvElem[i].Enum;
    cpKtvElem[i].Enum = 0;
    cProf = KTV_GetCurrProf((TICK_NUM_KTV * i) + cStartIdx);

    uint8_t cIdx = TICK_NUM_KTV_PAUSE;
    cCount = 0;
    for (int k = cIdx - 1; k >= 0; --k) {
      if (cProf & (cBit << k)) {
        ++cCount;
      } else { 
        break;
      }
    }
    aKtvRezult[i].Before = cCount;
    cCount = 0;
    for ( ; cIdx < (TICK_NUM_KTV_PAUSE + TICK_NUM_KTV_BIT); ++cIdx) {
      if (cProf & (cBit << cIdx))
        ++cCount;
    }
    aKtvRezult[i].Online = cCount;

    cCount = 0;
    for ( ; cIdx < (TICK_NUM_KTV_PAUSE + (TICK_NUM_KTV_BIT * 2)); ++cIdx) {
      if (cProf & (cBit << cIdx))
        ++cCount;
    }
    aKtvRezult[i].Break = cCount;

    cCount = 0;
    int cBorder = (TICK_NUM_KTV_PAUSE + (TICK_NUM_KTV_BIT * 2) + (TICK_NUM_KTV_PAUSE));
    for ( ; cIdx < cBorder; ++cIdx) {
      if (cProf & (cBit << cIdx)) {
        ++cCount;
      } else {
        break;
      }
    }
    aKtvRezult[i].After = cCount;
    
    int cLength = aKtvRezult[i].Online + aKtvRezult[i].Break;
    if (aKtvRezult[i].Before == 0) {
      cLength += aKtvRezult[i].After;
      if (aKtvRezult[i].After >= TICK_NUM_KTV_PAUSE) {
        cpKtvElem[i].Enum |= KTV_ERROR;
      }
    } else if (aKtvRezult[i].After == 0) {
      cLength += aKtvRezult[i].Before;
      if (aKtvRezult[i].Before >= TICK_NUM_KTV_PAUSE) {
        cpKtvElem[i].Enum |= KTV_ERROR;
      }
    }
    if (cLength >= (TICK_NUM_KTV_BIT - 2)) {
      cpKtvElem[i].Enum |= KTV_ONLINE;
    }
    
    if (cLength >= ((TICK_NUM_KTV_BIT - 2) * 2)) {
      cpKtvElem[i].Enum |= KTV_TRIGGERED;
    }

    cpKtvElem[i].Changed = (cpKtvElem[i].Enum != cValue);
    
    if (cpKtvElem[i].Changed)
      bChanged = true;
  }

  return bChanged;
}

/**
 * @brief 
 *
 */
void KTV_SetTickValue(uint8_t val) {
  int cPos, cIdx;
  uint64_t cBit = 1;
  switch (State) {
  case ksStart:    
    if (--Counter <= 0) {
      State = ksStarted;
    }
    break;
  case ksWait:
    break;
  case ksStPulse:
    if (--Counter <= 0) {
      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
#ifdef PRE_SYNC_INT 
      Counter = TICK_NUM_TO_SYNC;
      State = ksSync;
#else
      BuffIdx = 0;
      State = ksRead;
#endif
    }
    break;
  case ksSync:
    if (--Counter <= 0) {
      BuffIdx = 0;
      State = ksRead;
    }
    break;
  case ksRead:
    cPos = BuffIdx / 64;
    cIdx = BuffIdx % 64;
    if (val) {
      Bitmap[cPos] |= (cBit << cIdx);
    } else {
      Bitmap[cPos] &= ~(cBit << cIdx);
    }
    ++BuffIdx;
    if (BuffIdx >= KTV_BITMAP_SIZE) {
      State = ksEnd;
    }
    break;
    
  case ksNone:
  case ksStarted:
  case ksEnd:
  case ksNoActive:
    break;    
  }
#ifdef DEBUG
  gKtvTickCount++;
#endif
}

/**
 * @brief Search for triggered sensors
 *
 */
bool KTV_Triggered() {
  for (uint16_t i = 0; i < KTV_NUM_MAX + 1; ++i) {
    if (aKtvElem[i].Enum & KTV_TRIGGERED) {
      return true;
    }
  }
  return false;
}

extern bool IsKbNorm();

/**
 * @brief 
 *
 */
void KTV_ProcessKb() {
  static int8_t sbLastKbFault = 0;
#ifdef KTV_REPEAT_ON_KB_OFF
  if (!IsKbNorm()) {
    sbLastKbFault = 2;
    Counter = KTV_TEST_INTERVAL / MS_IN_TICK;
    State = ksStart;
  } else {
    if (sbLastKbFault > 1) {
      --sbLastKbFault;
      Counter = KTV_TEST_INTERVAL / MS_IN_TICK;
      State = ksStart;
    } else {
      if (sbLastKbFault > 0) {
        sbLastKbFault = 0;
        
        if (KTV_Triggered()) {
          Counter = 500 / MS_IN_TICK;
          State = ksStart;
        } else {
          State = ksWait;
        }
      }
    }
  }
#else
 #ifdef KTV_TEST
  Counter = KTV_TEST_INTERVAL / MS_IN_TICK;
  State = ksStart;
 #else
  if (!IsKbNorm()) {
    sbLastKbFault = true;
    State = ksWait;
  } else {
    if (sbLastKbFault) {
      sbLastKbFault = false;
      Counter = KTV_TEST_INTERVAL / MS_IN_TICK;
      State = ksStart;
    } else {
      State = ksWait;
    }
  }
 #endif
#endif
}

/**
 * @brief 
 *
 */
void KTV_ProcessRead() {
  while (State != ksEnd) {
    vTaskDelay(10);
  }
  if (KTV_ProcessProf()) {
    if (State == ksNoActive) {
      uint16_t cNumber = KTV_NUM_MAX;
      for (uint16_t i = 0; i < (cNumber + 1); ++i) {
        if (aKtvElem[i].Enum != 0) {
          aKtvElem[i].Enum = 0;
          aKtvElem[i].Changed = true;
        }
      }
    }
  }
  KTV_ProcessKb();
}

/**
 * @brief 
 *
 */
void KTV_Process(KTVmsg msg) {
  if (msg != kmNone) {
    if (msg == kmStart) {
      KTV_Start();
      KTV_ProcessRead();
    } else {
      if (State == ksStarted) {
        KTV_ProcessRead();
      }
    }
  } else {
    if (State == ksStarted) {
      KTV_Start();
      KTV_ProcessRead();
    }
  }
}
