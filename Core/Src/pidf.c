#include "pidf.h"

void PIDf(tPID *uPID, float *Input, float *Output, float *Setpoint, float Kp, float Ki, float Kd) {
  uPID->Output = Output;
  uPID->Input = Input;
  uPID->Setpoint = Setpoint;
  uPID->LastInput = 0;
  uPID->direction = 0;
  uPID->OutputSum = 0;
  uPID->Kp = Kp;
  uPID->Ki = Ki;
  uPID->Kd = Kd;
  uPID->LastTime = 0;
  uPID->mode = 1;
  uPID->OutMin = -32768;
  uPID->OutMax = 32767;
  uPID->dt_s = 0.1f;
  uPID->dt = 100;
}

void PIDf_SetLimits(tPID* uPID, int16_t Min, int16_t Max) {
  uPID->OutMin = Min;
  uPID->OutMax = Max;
}

void PIDf_SetSampleTime(tPID* uPID, uint16_t SampleTime) {
  uPID->dt_s = SampleTime / 1000.0f;
  uPID->dt = SampleTime;
}

void PIDf_calc(tPID *uPID) {
  uint32_t timeChange = HAL_GetTick() - uPID->LastTime;
  
  if (timeChange >= uPID->dt) {
    uPID->LastTime = HAL_GetTick();
    
    float output = 0;
    float error = *uPID->Setpoint - *uPID->Input;
    float delta_input = uPID->LastInput - *uPID->Input;
    uPID->LastInput = *uPID->Input;
    
    if (uPID->direction) {
      error = -error;
      delta_input = -delta_input;
    }
    
    if (uPID->mode) {
      output = uPID->Kp * error;
    } else {
      output = 0;
    }
    
    output += uPID->Kd * delta_input / uPID->dt_s;
    
    uPID->OutputSum += uPID->Ki * error * uPID->dt_s;
    
    if (uPID->mode) {
      uPID->OutputSum += uPID->Kp * delta_input;
    }
    
    if (uPID->OutputSum > uPID->OutMax) {
      uPID->OutputSum = uPID->OutMax;
    } else if (uPID->OutputSum < uPID->OutMin) {
      uPID->OutputSum = uPID->OutMin;
    }
    
    output += uPID->OutputSum;
    
    if (output > uPID->OutMax) {
      output = uPID->OutMax;
    } else if (output < uPID->OutMin) {
      output = uPID->OutMin;
    }
    
    *uPID->Output = output;
  }
}