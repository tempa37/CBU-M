#ifndef _PIDF
#define _PIDF

#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef struct sPID
{
	float* Input;
	float* Output;
	float* Setpoint;
	float LastInput;
// NORMAL (0) or REVERSE (1)
	uint8_t direction;
// ERROR (0) or RATE (1)
	uint8_t mode;
	float Kp;
	float Ki;
	float Kd;
	int16_t dt;
	float dt_s;
	float OutputSum;
	int16_t OutMin;
	int16_t OutMax;
	uint32_t LastTime;
} tPID;

void PIDf(tPID *uPID, float *Input, float *Output, float *Setpoint, float Kp, float Ki, float Kd);
void PIDf_SetLimits(tPID* uPID, int16_t Min, int16_t Max);
void PIDf_SetSampleTime(tPID* uPID, uint16_t SampleTime);
void PIDf_calc(tPID *uPID);

#endif