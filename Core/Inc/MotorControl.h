#ifndef __MOTOR_CONTROL_H
#define __MOTOR_CONTROL_H

#include <stdint.h>

#define MOTOR_SPEED_LIMIT_ALARM     0x01
#define MOTOR_CURRENT_LIMIT_ALARM   0x02
#define MOTOR_SPEED_REG_SAT_ALARM   0x04
#define MOTOR_CURR_REG_SAT_ALARM    0x08


typedef enum{
  MOTOR_NOT_ACTIVE=0,
  MOTOR_SOFT_START=1,
  MOTOR_ACTIVE=2,
  MOTOR_SOFT_BRAKING=3,
  MOTOR_RECUPERATION=4
}MotorStates;

typedef struct{
  float RPM_reference;
  float RPM_measured;
  float Current_Measured;
  /** 1 - ccw , 0 - cw*/
  uint8_t Direction;
  uint8_t PrevDirection;
  uint8_t HallState;
  uint8_t PrevHallState;
  int16_t ElectricalAngle;
  uint8_t Step;
  uint8_t StepEvent;
  uint16_t alarms;
  MotorStates actualMotorState;
}MotorControlParameterStruct;
extern MotorControlParameterStruct MotorControlParameters;

void MotorControlInit(void);

void MotorSixStepSwitch(void);

void MotorTurnOn(void);

void MotorTurnOff(void);

float MotorCalculateNewRPM(uint16_t new_value);

void MotorCalculateRotationSpeed(uint32_t time);
void MotorGetActualHallState(void);
void MotorCalculateNewHallState(void);
void MotorLoadNewStep(void);
void MotorGetNextStep(void);

void MotorUpdateTimePulse(uint16_t pulse);

void CallbackUpdateTimerPulse(void);

void MotorTurnOffSlow(void);

void MotorFSMService(void);

void MotorEstimateDcCurrentFromPhaseShunt(void);

void MotorControlStartCurrentSenseCalibration(void);

void MotorControlCurrentSenseCalibrationCallback(void);
#endif
