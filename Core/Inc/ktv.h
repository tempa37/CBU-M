#ifndef __KTV_H__
#define __KTV_H__

#include <stdbool.h>

#define PRE_SYNC_INT
#define KTV_REPEAT_ON_KB_OFF
#define KTV_START_PULSE
//#define KTV_TEST

#define KTV_USED_NUM                20

#define KTV_ONLINE    (1 << 0)
#define KTV_TRIGGERED (1 << 1)
#define KTV_ERROR     (1 << 2)

// Maximum number of switches (sensor)
#define KTV_NUM_MAX                 50
// Position of the first overrun sensor
#define KTV_DPP_POS                 47
// Number of ms in one tick
#define MS_IN_TICK                  2
// Number of ticks per quantum
#define TICK_NUM_PER_QUANT          2
// Number of quanta per bit (Online, Break)
#define QUANT_NUM_KTV_BIT           4
// Number of ticks per bit
#define TICK_NUM_KTV_BIT            (QUANT_NUM_KTV_BIT * TICK_NUM_PER_QUANT)
// Number of quanta in pause
#define QUANT_NUM_KTV_PAUSE         8
// Number of ticks in pause
#define TICK_NUM_KTV_PAUSE          (QUANT_NUM_KTV_PAUSE * TICK_NUM_PER_QUANT)
// Number of quanta per sensor
#define QUANT_NUM_KTV               ((QUANT_NUM_KTV_BIT * 2) + QUANT_NUM_KTV_PAUSE)
// Number of ticks per sensor
#define TICK_NUM_KTV                (QUANT_NUM_KTV * TICK_NUM_PER_QUANT)
// Number of quanta in the starting block
#define QUANT_NUM_KTV_START         16
// Number of ticks in the starting block
#define TICK_NUM_KTV_START          (QUANT_NUM_KTV_START * TICK_NUM_PER_QUANT)
                                                                         // = момент включения КТВ = начало отсчёта

// Расстояние от конца стартового импульса до начала синхроимпульса
#define SYNC_PULSE_OFFSET_IN_MS     150

#define SYNC_PULSE_OFFSET_IN_TIMER  (SYNC_PULSE_OFFSET_IN_MS / MS_IN_TICK)
// Sync pulse duration
#define SYNC_PULSE_WIDTH_IN_MS      8

#define SYNC_PULSE_WIDTH_IN_TIMER   (SYNC_PULSE_WIDTH_IN_MS / MS_IN_TICK)

//Пауза после синхроимпульса перед первым интервалом (КТВ №0)
#define SYNC_PULSE_PAUSE_IN_MS      32
#define SYNC_PULSE_PAUSE_IN_TIMER   (SYNC_PULSE_PAUSE_IN_MS / MS_IN_TICK)

//Количество тиков перед началом поиска синхроимпульса (= 67 тиков)
#define TICK_NUM_TO_SYNC            (SYNC_PULSE_OFFSET_IN_TIMER - SYNC_PULSE_PAUSE_IN_TIMER)

#define QUANT_NUM_KTV               ((QUANT_NUM_KTV_BIT * 2) + QUANT_NUM_KTV_PAUSE) 

#define KTV_BITMAP_SIZE             (((KTV_NUM_MAX + 1) * TICK_NUM_KTV) + TICK_NUM_KTV_START + 128)

#define KTV_PAUSE_IN_TICKS          16
#define KTV_START_PAUSE_IN_TICKS    8
#define KTV_BOOTUP_INTERVAL         3000
#define KTV_TEST_INTERVAL           5000

#define SYNC_PULSE_OFFSET_IN_TICK   75
#define SYNC_PULSE_WIDTH_IN_TICK    4

#define KTV_MAX_POLL_INTERVAL       ((KTV_NUM_MAX + 2) * TICK_NUM_KTV * MS_IN_TICK) + \
            (KTV_PAUSE_IN_TICKS + TICK_NUM_KTV_START + SYNC_PULSE_OFFSET_IN_TICK + SYNC_PULSE_WIDTH_IN_TICK) * MS_IN_TICK;

#define TICK_NUM_TO_SYNC            (SYNC_PULSE_OFFSET_IN_TIMER - SYNC_PULSE_PAUSE_IN_TIMER)

#define TICK_NUM_KTV_AREA           (TICK_NUM_KTV + TICK_NUM_KTV_PAUSE)

typedef enum eKtvMsg { 
  kmNone,     
  kmStart,    
  kmFinish,   
  kmCount
} KTVmsg;

typedef enum eKtvState {
  ksNone,     
  ksStart,    
  ksStarted,  
  ksWait,     
  ksStPulse,  
  ksSync,    
  ksRead,     
  ksEnd,      
  ksNoActive, 
  //ksCount,
} KTVstate;

typedef struct sKtvElem {
  uint8_t Enum;
  bool Changed;
} tsKtvElem;

void KTV_Init();
void KTV_SetTickValue(uint8_t val);
void KTV_Process(KTVmsg msg);
KTVstate KTV_State();

#endif /* __KTV_H__ */