#include "ktv.h"

#include "main.h"
#include "string.h"

typedef struct sKtvRezult {
  uint8_t Before;
  uint8_t Online;
  uint8_t Break;
  uint8_t After;
} tsKtvRezult;

static volatile KTVstate State = ksNone;

static uint64_t Bitmap[KTV_BITMAP_WORDS];

tsKtvElem aKtvElem[KTV_NUM_MAX + 1U] = {0};

static tsKtvRezult aKtvRezult[KTV_NUM_MAX + 1U];
static tsKtvWebSnapshot sKtvSnapshot = {0};

static int16_t BuffIdx = 0;
static int32_t Counter = 0;
static volatile uint8_t sKtvPollPending = 0U;

#ifdef DEBUG
uint32_t gKtvTickCount = 0U;
#endif

static uint32_t KTV_Lock(void);
static void KTV_Unlock(uint32_t primask);
static bool KTV_IsUsedIndex(uint16_t index);
static bool KTV_IsBusyState(KTVstate state);
static uint8_t KTV_ReadAddressPin(void);
static uint8_t KTV_ReadInitPin(void);
static uint8_t KTV_NormalizeLineActive(uint8_t val);
static void KTV_SetInitPinActive(uint8_t active);
static void KTV_ResetMeasureBuffer(void);
static void KTV_ClearDecodedItems(void);
static void KTV_Start(void);
static uint64_t KTV_GetCurrProf(int32_t bitmap_idx);
static bool KTV_ProcessProf(void);
static void KTV_UpdateSnapshotItems(void);

KTVstate KTV_State(void) {
  return State;
}

static uint32_t KTV_Lock(void) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  return primask;
}

static void KTV_Unlock(uint32_t primask) {
  if (primask == 0U) {
    __enable_irq();
  }
}

static bool KTV_IsUsedIndex(uint16_t index) {
  return (index <= KTV_USED_NUM) || (index >= KTV_DPP_POS);
}

static bool KTV_IsBusyState(KTVstate state) {
  return (state == ksStPulse) || (state == ksSync) || (state == ksRead);
}

static uint8_t KTV_ReadAddressPin(void) {
  return (HAL_GPIO_ReadPin(KTV_ADR_GPIO_Port, KTV_ADR_Pin) == GPIO_PIN_SET) ? 1U : 0U;
}

static uint8_t KTV_ReadInitPin(void) {
  return (HAL_GPIO_ReadPin(PWR_KTV_GPIO_Port, PWR_KTV_Pin) == GPIO_PIN_SET) ? 1U : 0U;
}

static uint8_t KTV_NormalizeLineActive(uint8_t val) {
  /* KTV line is active-low: a device response pulls KTV_ADR down to 0. */
  return (val == 0U) ? 1U : 0U;
}

static void KTV_SetInitPinActive(uint8_t active) {
  HAL_GPIO_WritePin(PWR_KTV_GPIO_Port, PWR_KTV_Pin, active != 0U ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void KTV_ResetMeasureBuffer(void) {
  memset(Bitmap, 0, sizeof(Bitmap));
  BuffIdx = 0;
  memset(aKtvRezult, 0, sizeof(aKtvRezult));
}

static void KTV_ClearDecodedItems(void) {
  memset(aKtvElem, 0, sizeof(aKtvElem));
  memset(aKtvRezult, 0, sizeof(aKtvRezult));
}

static void KTV_Start(void) {
  KTV_ResetMeasureBuffer();
  KTV_SetInitPinActive(1U);
  Counter = (int32_t)TICK_NUM_KTV_START;
  State = ksStPulse;
}

static uint64_t KTV_GetCurrProf(int32_t bitmap_idx) {
  uint64_t profile = 0U;
  uint32_t bit_idx;

  for (bit_idx = 0U; bit_idx < TICK_NUM_KTV_AREA; ++bit_idx) {
    int32_t src_idx = bitmap_idx + (int32_t)bit_idx;

    if ((src_idx >= 0) && ((uint32_t)src_idx < KTV_BITMAP_SIZE)) {
      uint32_t word = (uint32_t)src_idx / 64U;
      uint32_t word_bit = (uint32_t)src_idx % 64U;

      if ((Bitmap[word] & (1ULL << word_bit)) != 0U) {
        profile |= (1ULL << bit_idx);
      }
    }
  }

  return profile;
}

static bool KTV_ProcessProf(void) {
  uint64_t start_window = Bitmap[0];
  int32_t start_idx = -1;
  int32_t run_start = -1;
  uint32_t run_len = 0U;
  uint16_t i;

  for (i = 4U; i < 64U; ++i) {
    if ((start_window & (1ULL << i)) != 0U) {
      if (run_len == 0U) {
        run_start = (int32_t)i;
      }
      ++run_len;
      if (run_len >= 5U) {
        start_idx = run_start + (int32_t)KTV_START_PAUSE_IN_TICKS;
        break;
      }
    } else {
      run_len = 0U;
      run_start = -1;
    }
  }

  if (start_idx < 0) {
    KTV_ClearDecodedItems();
    State = ksNoActive;
    return false;
  }

  for (i = 0U; i <= KTV_NUM_MAX; ++i) {
    uint8_t previous_flags = aKtvElem[i].Enum;
    uint64_t profile;
    uint32_t count;
    uint32_t bit_idx;
    uint32_t bit;
    uint32_t border;
    uint32_t length;

    aKtvElem[i].Enum = 0U;
    aKtvElem[i].Changed = false;
    memset(&aKtvRezult[i], 0, sizeof(aKtvRezult[i]));

    if (!KTV_IsUsedIndex(i)) {
      continue;
    }

    profile = KTV_GetCurrProf(((int32_t)TICK_NUM_KTV * (int32_t)i) + start_idx);

    count = 0U;
    for (bit = TICK_NUM_KTV_PAUSE; bit > 0U; --bit) {
      if ((profile & (1ULL << (bit - 1U))) != 0U) {
        ++count;
      } else {
        break;
      }
    }
    aKtvRezult[i].Before = (uint8_t)count;

    count = 0U;
    bit_idx = TICK_NUM_KTV_PAUSE;
    for (; bit_idx < (TICK_NUM_KTV_PAUSE + TICK_NUM_KTV_BIT); ++bit_idx) {
      if ((profile & (1ULL << bit_idx)) != 0U) {
        ++count;
      }
    }
    aKtvRezult[i].Online = (uint8_t)count;

    count = 0U;
    for (; bit_idx < (TICK_NUM_KTV_PAUSE + (TICK_NUM_KTV_BIT * 2U)); ++bit_idx) {
      if ((profile & (1ULL << bit_idx)) != 0U) {
        ++count;
      }
    }
    aKtvRezult[i].Break = (uint8_t)count;

    count = 0U;
    border = TICK_NUM_KTV_PAUSE + (TICK_NUM_KTV_BIT * 2U) + TICK_NUM_KTV_PAUSE;
    for (; bit_idx < border; ++bit_idx) {
      if ((profile & (1ULL << bit_idx)) != 0U) {
        ++count;
      } else {
        break;
      }
    }
    aKtvRezult[i].After = (uint8_t)count;

    length = (uint32_t)aKtvRezult[i].Online + (uint32_t)aKtvRezult[i].Break;

    if (aKtvRezult[i].Before == 0U) {
      length += aKtvRezult[i].After;
      if (aKtvRezult[i].After >= TICK_NUM_KTV_PAUSE) {
        aKtvElem[i].Enum |= KTV_ERROR;
      }
    } else if (aKtvRezult[i].After == 0U) {
      length += aKtvRezult[i].Before;
      if (aKtvRezult[i].Before >= TICK_NUM_KTV_PAUSE) {
        aKtvElem[i].Enum |= KTV_ERROR;
      }
    }

    if (length >= (TICK_NUM_KTV_BIT - 2U)) {
      aKtvElem[i].Enum |= KTV_ONLINE;
    }

    if (length >= ((TICK_NUM_KTV_BIT - 2U) * 2U)) {
      aKtvElem[i].Enum |= KTV_TRIGGERED;
    }

    aKtvElem[i].Changed = (aKtvElem[i].Enum != previous_flags);
  }

  return true;
}

static void KTV_UpdateSnapshotItems(void) {
  uint32_t primask = KTV_Lock();
  uint16_t i;

  for (i = 0U; i <= KTV_NUM_MAX; ++i) {
    sKtvSnapshot.items[i].used = KTV_IsUsedIndex(i) ? 1U : 0U;
    sKtvSnapshot.items[i].flags = aKtvElem[i].Enum;
    sKtvSnapshot.items[i].before = aKtvRezult[i].Before;
    sKtvSnapshot.items[i].online = aKtvRezult[i].Online;
    sKtvSnapshot.items[i].gap = aKtvRezult[i].Break;
    sKtvSnapshot.items[i].after = aKtvRezult[i].After;
  }

  sKtvSnapshot.ready = 1U;
  ++sKtvSnapshot.poll_counter;
  KTV_Unlock(primask);
}

void KTV_Init(void) {
  uint16_t i;
#ifdef DEBUG
  gKtvTickCount = 0U;
#endif
  KTV_ClearDecodedItems();
  KTV_ResetMeasureBuffer();
  memset(&sKtvSnapshot, 0, sizeof(sKtvSnapshot));
  for (i = 0U; i <= KTV_NUM_MAX; ++i) {
    sKtvSnapshot.items[i].used = KTV_IsUsedIndex(i) ? 1U : 0U;
  }
  Counter = 0;
  sKtvPollPending = 0U;
  KTV_SetInitPinActive(0U);
  State = ksWait;
}

void KTV_SetTickValue(uint8_t val) {
  switch (State) {
    case ksStPulse: {
      --Counter;
      if (Counter <= 0) {
        KTV_SetInitPinActive(0U);
#ifdef PRE_SYNC_INT
        Counter = (int32_t)TICK_NUM_TO_SYNC;
        State = ksSync;
#else
        BuffIdx = 0;
        State = ksRead;
#endif
      }
      break;
    }
    case ksSync: {
      --Counter;
      if (Counter <= 0) {
        BuffIdx = 0;
        State = ksRead;
      }
      break;
    }
    case ksRead: {
      uint32_t word = (uint32_t)BuffIdx / 64U;
      uint32_t bit = (uint32_t)BuffIdx % 64U;

      if (KTV_NormalizeLineActive(val) != 0U) {
        Bitmap[word] |= (1ULL << bit);
      } else {
        Bitmap[word] &= ~(1ULL << bit);
      }

      ++BuffIdx;
      if ((uint32_t)BuffIdx >= KTV_BITMAP_SIZE) {
        State = ksEnd;
      }
      break;
    }
    default: {
      break;
    }
  }

#ifdef DEBUG
  ++gKtvTickCount;
#endif
}

uint8_t KTV_RequestPoll(void) {
  uint8_t accepted = 0U;
  uint32_t primask = KTV_Lock();

  if ((sKtvPollPending == 0U) && !KTV_IsBusyState(State) && (State != ksEnd)) {
    sKtvPollPending = 1U;
    sKtvSnapshot.ready = 0U;
    accepted = 1U;
  }

  KTV_Unlock(primask);
  return accepted;
}

void KTV_GetSnapshot(tsKtvWebSnapshot *snapshot) {
  uint32_t primask;
  uint8_t poll_pending;
  KTVstate state;

  if (snapshot == NULL) {
    return;
  }

  primask = KTV_Lock();
  memcpy(snapshot, &sKtvSnapshot, sizeof(*snapshot));
  poll_pending = sKtvPollPending;
  state = State;
  KTV_Unlock(primask);

  snapshot->poll_pending = poll_pending;
  snapshot->busy = ((poll_pending != 0U) || KTV_IsBusyState(state) || (state == ksEnd)) ? 1U : 0U;
  snapshot->state = (uint8_t)state;
  snapshot->init_pin = KTV_ReadInitPin();
  snapshot->line_pin = KTV_ReadAddressPin();
}

void KTV_Process(KTVmsg msg) {
  if (msg == kmStart) {
    (void)KTV_RequestPoll();
  } else if (msg == kmFinish) {
    uint32_t primask = KTV_Lock();
    sKtvPollPending = 0U;
    KTV_Unlock(primask);
    KTV_SetInitPinActive(0U);
    State = ksWait;
    return;
  }

  if ((sKtvPollPending != 0U) && !KTV_IsBusyState(State) && (State != ksEnd)) {
    uint32_t primask = KTV_Lock();
    sKtvPollPending = 0U;
    KTV_Unlock(primask);
    KTV_Start();
    return;
  }

  if (State == ksEnd) {
    (void)KTV_ProcessProf();
    if (State == ksEnd) {
      State = ksWait;
    }
    KTV_UpdateSnapshotItems();
  }
}
