#ifndef __KTV_H__
#define __KTV_H__

#include <stdbool.h>
#include <stdint.h>

#define PRE_SYNC_INT
#define KTV_START_PULSE

#define KTV_USED_NUM 5U

#define KTV_ONLINE    (1U << 0)
#define KTV_TRIGGERED (1U << 1)
#define KTV_ERROR     (1U << 2)

// Maximum number of switches (sensor)
#define KTV_NUM_MAX                 50U
// Position of the first overrun sensor
#define KTV_DPP_POS                 47U
// Number of ms in one tick
#define MS_IN_TICK                  2U
// Number of ticks per quantum
#define TICK_NUM_PER_QUANT          2U
// Number of quanta per bit (Online, Break)
#define QUANT_NUM_KTV_BIT           4U
// Number of ticks per bit
#define TICK_NUM_KTV_BIT            (QUANT_NUM_KTV_BIT * TICK_NUM_PER_QUANT)
// Number of quanta in pause
#define QUANT_NUM_KTV_PAUSE         8U
// Number of ticks in pause
#define TICK_NUM_KTV_PAUSE          (QUANT_NUM_KTV_PAUSE * TICK_NUM_PER_QUANT)
// Number of quanta per sensor
#define QUANT_NUM_KTV               ((QUANT_NUM_KTV_BIT * 2U) + QUANT_NUM_KTV_PAUSE)
// Number of ticks per sensor
#define TICK_NUM_KTV                (QUANT_NUM_KTV * TICK_NUM_PER_QUANT)
// Number of quanta in the starting block
#define QUANT_NUM_KTV_START         16U
// Number of ticks in the starting block
#define TICK_NUM_KTV_START          (QUANT_NUM_KTV_START * TICK_NUM_PER_QUANT)

// Расстояние от конца стартового импульса до начала синхроимпульса
#define SYNC_PULSE_OFFSET_IN_MS     150U

#define SYNC_PULSE_OFFSET_IN_TIMER  (SYNC_PULSE_OFFSET_IN_MS / MS_IN_TICK)
// Sync pulse duration
#define SYNC_PULSE_WIDTH_IN_MS      8U

#define SYNC_PULSE_WIDTH_IN_TIMER   (SYNC_PULSE_WIDTH_IN_MS / MS_IN_TICK)

// Пауза после синхроимпульса перед первым интервалом (КТВ №0)
#define SYNC_PULSE_PAUSE_IN_MS      32U
#define SYNC_PULSE_PAUSE_IN_TIMER   (SYNC_PULSE_PAUSE_IN_MS / MS_IN_TICK)

// Количество тиков перед началом поиска синхроимпульса
#define TICK_NUM_TO_SYNC            (SYNC_PULSE_OFFSET_IN_TIMER - SYNC_PULSE_PAUSE_IN_TIMER)

#define KTV_BITMAP_SIZE             (((KTV_NUM_MAX + 1U) * TICK_NUM_KTV) + TICK_NUM_KTV_START + 128U)
#define KTV_BITMAP_WORDS            ((KTV_BITMAP_SIZE + 63U) / 64U)

#define KTV_PAUSE_IN_TICKS          16U
#define KTV_SYNC_MIN_IN_TICKS       SYNC_PULSE_WIDTH_IN_TIMER
#define KTV_START_PAUSE_IN_TICKS    SYNC_PULSE_PAUSE_IN_TIMER
#define KTV_BOOTUP_INTERVAL         3000U

#define SYNC_PULSE_OFFSET_IN_TICK   75U
#define SYNC_PULSE_WIDTH_IN_TICK    4U

#define KTV_MAX_POLL_INTERVAL       (((KTV_NUM_MAX + 2U) * TICK_NUM_KTV * MS_IN_TICK) + \
                                     ((KTV_PAUSE_IN_TICKS + TICK_NUM_KTV_START + SYNC_PULSE_OFFSET_IN_TICK + \
                                       SYNC_PULSE_WIDTH_IN_TICK) * MS_IN_TICK))

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
} KTVstate;

typedef struct sKtvElem {
  uint8_t Enum;
  bool Changed;
} tsKtvElem;

typedef struct sKtvWebItem {
  uint8_t used;
  uint8_t flags;
  uint8_t before;
  uint8_t online;
  uint8_t gap;
  uint8_t after;
} tsKtvWebItem;

typedef struct sKtvWebSnapshot {
  uint8_t busy;
  uint8_t ready;
  uint8_t init_pin;
  uint8_t line_pin;
  uint8_t state;
  uint8_t poll_pending;
  uint16_t reserved;
  uint32_t poll_counter;
  tsKtvWebItem items[KTV_NUM_MAX + 1U];
} tsKtvWebSnapshot;

extern tsKtvElem aKtvElem[];

void KTV_Init(void);
void KTV_SetTickValue(uint8_t val);
void KTV_Process(KTVmsg msg);
KTVstate KTV_State(void);
uint8_t KTV_RequestPoll(void);
void KTV_GetSnapshot(tsKtvWebSnapshot *snapshot);

#endif /* __KTV_H__ */
