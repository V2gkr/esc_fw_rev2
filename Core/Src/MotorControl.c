#include "main.h"
//#include "DRV8320S.h"
#include "board_common.h"
#include "MotorControl.h"
#include "MotorConfig.h"
#include "stm32g4xx_hal_adc_ex.h"
#include "stm32g4xx_hal_tim.h"
#include <stdint.h>
volatile uint8_t bldc_count=0;

float speed_log[1024];
uint16_t speed_counter=0;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim6;
extern OPAMP_HandleTypeDef hopamp1;
extern OPAMP_HandleTypeDef hopamp2;
extern OPAMP_HandleTypeDef hopamp3;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

uint16_t opamp1_data;
uint16_t opamp2_data;
uint16_t opamp3_data;
MotorControlParameterStruct MotorControlParameters={0};
// #define CURRENT_SENSE_CIRCUIT_EQ_RESISTANCE (float)583.33
#define CURRENT_SENSE_CIRCUIT_EQ_RESISTANCE (float)175

#define ADC_ZERO_CURRENT_VALUE_1            1424
#define ADC_ZERO_CURRENT_VALUE_2            1424
#define ADC_ZERO_CURRENT_VALUE_3            1436
#define PGA_GAIN                            8
/* step 1 - phase 3 pwm , phase 2 force inactive , phase 1 off*/
#define STEP1_CCMR1   0x4808
#define STEP1_CCMR2   0x6868
#define STEP14_CCER   0x1550

/* step 2 - phase 3 off , phase 2 force inactive , phase 1 pwm*/
#define STEP2_CCMR1   0x4868
#define STEP2_CCMR2   0x6808
#define STEP25_CCER   0x1055

/* step 3 - phase 3 force inactive , phase 2 off , phase 1 pwm*/
#define STEP3_CCMR1   0x0868
#define STEP3_CCMR2   0x6848
#define STEP36_CCER   0x1505

/* step 4 - phase 3 force inactive , phase 2 pwm , phase 1 off*/
#define STEP4_CCMR1   0x6808
#define STEP4_CCMR2   0x6848

/* step 5 - phase 3 off , phase 2 pwm , phase 1 force inactive*/
#define STEP5_CCMR1   0x6848
#define STEP5_CCMR2   0x6808

/* step 6 - phase 3 pwm , phase 2 off , phase 1 force inactive*/
#define STEP6_CCMR1   0x0848
#define STEP6_CCMR2   0x6868

#define LOW_SPEED_MEASUREMENT_LIMIT 13333

#define CURRENT_SENSE_CALIBRATION_COUNTER 1000
uint8_t soft_start_counter=0;
volatile uint8_t soft_start_update_event=0;

volatile uint8_t CurrentCalibrationState=1;
uint16_t CurrentCalibrationCounter=0;
uint16_t CurrentShuntRawData[3];

typedef enum{
  HALL_STATE_1_101=0b101,
  HALL_STATE_2_100=0b100,
  HALL_STATE_3_110=0b110,
  HALL_STATE_4_010=0b010,
  HALL_STATE_5_011=0b011,
  HALL_STATE_6_001=0b001
}HallStates;

uint32_t currentsensecalibration[3]={0};

void MotorControlStartCurrentSenseCalibration(void){
  HAL_TIM_Base_Start_IT(&htim1);
  TIM1->CR2 |= (uint16_t) TIM_CR2_CCPC;
  __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_CC4);
  //stops main until calibration is complete
  while(CurrentCalibrationState);
}

void MotorControlCurrentSenseCalibrationCallback(void){
  if(CurrentCalibrationState==0)
    return;
  if(CurrentCalibrationCounter<CURRENT_SENSE_CALIBRATION_COUNTER && CurrentCalibrationState){
    CurrentCalibrationCounter++;
    currentsensecalibration[0]+=HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
    currentsensecalibration[1]+=HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);
    currentsensecalibration[2]+=HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_2);
  }
  else{
    currentsensecalibration[0]/=CURRENT_SENSE_CALIBRATION_COUNTER;
    currentsensecalibration[1]/=CURRENT_SENSE_CALIBRATION_COUNTER;
    currentsensecalibration[2]/=CURRENT_SENSE_CALIBRATION_COUNTER;
    CurrentCalibrationState=0;
    HAL_TIM_Base_Stop_IT(&htim1);
  }
}

void MotorControlInit(void){
  MotorControlParameters.RPM_reference=30;//MotorCalculateNewRPM(SIX_STEP_FREQ);
  HAL_ADCEx_Calibration_Start(&hadc1, ADC_DIFFERENTIAL_ENDED);
  HAL_ADCEx_Calibration_Start(&hadc2, ADC_DIFFERENTIAL_ENDED);
  HAL_TIMEx_HallSensor_Start_IT(&htim4);
  HAL_OPAMP_SelfCalibrate(&hopamp1);
  HAL_OPAMP_SelfCalibrate(&hopamp2);
  HAL_OPAMP_SelfCalibrate(&hopamp3);
  HAL_OPAMP_Start(&hopamp1);
  HAL_OPAMP_Start(&hopamp2);
  HAL_OPAMP_Start(&hopamp3);
  //3 phase measurement start and shit
  HAL_ADCEx_InjectedStart(&hadc1);
  HAL_ADCEx_InjectedStart(&hadc2);
}

void MotorUpdateTimePulse(uint16_t pulse){
  TIM1->CCR1 = pulse;
  TIM1->CCR2 = pulse;
  TIM1->CCR3 = pulse;
}

void MotorLoadNewStep(void){
  if(MotorControlParameters.StepEvent){
    MotorControlParameters.StepEvent=0;
    switch(MotorControlParameters.Step){
      case 1:
        /* step 1 - phase 3 pwm , phase 2 force inactive , phase 1 off
         *  phase 3 Vm , phase 2 gnd - looks like its 110*/
        TIM1->CCER=STEP14_CCER;
        TIM1->CCMR1=STEP1_CCMR1;
        TIM1->CCMR2=STEP1_CCMR2;
        break;
      case 2:
        /* step 2 - phase 3 off , phase 2 force inactive , phase 1 pwm
         * phase 1 vm phase 2 gnd looks like its 010*/
        TIM1->CCER=STEP25_CCER;
        TIM1->CCMR1=STEP2_CCMR1;
        TIM1->CCMR2=STEP2_CCMR2;
        break;
      case 3:
        /* step 3 - phase 3 force inactive , phase 2 off , phase 1 pwm
         * phase 1 vm phase 3 gnd looks like its 011*/
        TIM1->CCER=STEP36_CCER;
        TIM1->CCMR1=STEP3_CCMR1;
        TIM1->CCMR2=STEP3_CCMR2;
        break;
      case 4:
        /* step 4 - phase 3 force inactive , phase 2 pwm , phase 1 off
         * phase 2 vm , phase 3 gnd looks like its 001*/
        TIM1->CCER=STEP14_CCER;
        TIM1->CCMR1=STEP4_CCMR1;
        TIM1->CCMR2=STEP4_CCMR2;
        break;
      case 5:
        /* step 5 - phase 3 off , phase 2 pwm , phase 1 force inactive
         * phase 2 vm phase 1 gnd , looks like its 101 */
        TIM1->CCER=STEP25_CCER;
        TIM1->CCMR1=STEP5_CCMR1;
        TIM1->CCMR2=STEP5_CCMR2;
        break;
      case 6:
        /* step 6 - phase 3 pwm , phase 2 off , phase 1 force inactive
         * phase 3 vm phase 1 gnd looks like its 100*/
        TIM1->CCER=STEP36_CCER;
        TIM1->CCMR1=STEP6_CCMR1;
        TIM1->CCMR2=STEP6_CCMR2;
        break;
      default:
        break;
    }
    TIM1->EGR |= (uint16_t) TIM_EGR_COMG;
  }
}

void MotorTurnOn(void){
  //DRV8320_SetEnable();
  HAL_TIM_Base_Start_IT(&htim6);
  speed_counter=0;
	if(MotorControlParameters.actualMotorState==MOTOR_NOT_ACTIVE){
		MotorControlParameters.actualMotorState=MOTOR_SOFT_START;
		soft_start_counter=0;
		TIM1->CCR1 = 0;
		TIM1->CCR2 = 0;
		TIM1->CCR3 = 0;
		TIM1->CCR4 = 1;

		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
		HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
		HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
		HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
		TIM1->CR2 |= (uint16_t) TIM_CR2_CCPC;
		__HAL_TIM_ENABLE_IT(&htim1, TIM_IT_CC4);
		__HAL_TIM_MOE_ENABLE(&htim1);

	}
}

void MotorTurnOff(void){
  //DRV8320_ResetEnable();
  HAL_TIM_Base_Stop_IT(&htim6);
  __HAL_TIM_MOE_DISABLE_UNCONDITIONALLY(&htim1);
}

void MotorTurnOffSlow(void){
	if(MotorControlParameters.actualMotorState==MOTOR_ACTIVE)
		MotorControlParameters.actualMotorState=MOTOR_SOFT_BRAKING;
}


/* new value is a value inside ARR register*/
float MotorCalculateNewRPM(uint16_t new_value){
  return (float)(new_value/(MOTOR_POLE_PAIR*6.0f))*60;
}

//calculates rotation in rpm
void MotorCalculateRotationSpeed(uint32_t time){
  /* time is ccr1 register which is in a units of prescaler*/
  if(time>LOW_SPEED_MEASUREMENT_LIMIT)
    MotorControlParameters.RPM_measured=0;
  else{
    float ftime=(float)time/(HALL_CLOCK/(HALL_PRESCALER+1));
    MotorControlParameters.RPM_measured=(float)(60/(MOTOR_POLE_PAIR*ftime));
  }
}

void MotorGetActualHallState(void){
  /* pa5 - tim2 ch1 a), pb3 - tim2 ch2(b) , pb10 - tim2 ch3 (c)*/
  MotorControlParameters.HallState=(((GPIOB->IDR&GPIO_PIN_6)>>6)<<2)
                                    |(((GPIOB->IDR&GPIO_PIN_7)>>7)<<1)
                                    |(((GPIOB->IDR&GPIO_PIN_8))>>8);
}

void MotorEstimateDcCurrentFromPhaseShunt(void){
	int16_t adc_current_readings=0;
  opamp1_data=HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
  opamp2_data=HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);
  opamp3_data=HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_2);
	switch(MotorControlParameters.Step){
    case 1:
    case 2:
      /* step 1 - phase 3 pwm , phase 2 force inactive , phase 1 off
       *  phase 3 Vm , phase 2 gnd - read current phase 2*/
      /* step 2 - phase 3 off , phase 2 force inactive , phase 1 pwm
       * phase 1 vm phase 2 gnd - read current phase 2*/
      
      adc_current_readings=opamp2_data-currentsensecalibration[1];//1220
      break;
    case 3:
    case 4:
      /* step 3 - phase 3 force inactive , phase 2 off , phase 1 pwm
       * phase 1 vm phase 3 gnd - read current phase 3*/
      /* step 4 - phase 3 force inactive , phase 2 pwm , phase 1 off
       * phase 2 vm , phase 3 gnd - read current phase 3*/      
      adc_current_readings=opamp3_data-currentsensecalibration[2];//1220
      break;
    case 5:
    case 6:
      /* step 5 - phase 3 off , phase 2 pwm , phase 1 force inactive
       * phase 2 vm phase 1 gnd , - read current phase 1 */
      /* step 6 - phase 3 pwm , phase 2 off , phase 1 force inactive
       * phase 3 vm phase 1 gnd - read current phase 1*/      
      adc_current_readings=opamp1_data-currentsensecalibration[0];//1200
      break;
    default:
    //if no step is active here we should signal an error
      adc_current_readings=0;
      break;
	}
	//MotorControlParameters.Current_Measured=((int16_t)(CurrentShuntRawData[0]-ADC_ZERO_CURRENT_VALUE))*ADC_VOLTAGE_REFERENCE*CURRENT_SENSE_CIRCUIT_EQ_RESISTANCE/ADC_MAX_VALUE;
	MotorControlParameters.Current_Measured=((adc_current_readings*ADC_VOLTAGE_REFERENCE/ADC_MAX_VALUE)/PGA_GAIN)*CURRENT_SENSE_CIRCUIT_EQ_RESISTANCE;
	//calculate current itd
  if(speed_counter<1024 && MotorControlParameters.actualMotorState==MOTOR_ACTIVE){
    speed_log[speed_counter]=MotorControlParameters.Current_Measured;
    speed_counter++;
  }
}

void MotorCalculateNewHallState(void){
  uint8_t direction = MotorControlParameters.Direction;
  switch(MotorControlParameters.HallState){
    case HALL_STATE_1_101:
    // (direction) ? ccw : cw
      MotorControlParameters.Step = (direction) ? 4 : 5;
      break;
    case HALL_STATE_2_100:
      MotorControlParameters.Step = (direction) ? 3 : 4;
      break;
    case HALL_STATE_3_110:
      MotorControlParameters.Step = (direction) ? 2 : 3;
      break;
    case HALL_STATE_4_010:
      MotorControlParameters.Step = (direction) ? 1 : 2;
      break;
    case HALL_STATE_5_011:
      MotorControlParameters.Step = (direction) ? 6 : 1;
      break;
    case HALL_STATE_6_001:
      MotorControlParameters.Step = (direction) ? 5 : 6;
      break;
    default:
    // if no suitable hall state is active - we should consider an error 
      break;
  }
  if(MotorControlParameters.PrevHallState!=MotorControlParameters.HallState){
    MotorControlParameters.StepEvent=1;
  }
  MotorControlParameters.PrevHallState=MotorControlParameters.HallState;
}

void MotorGetNextStep(void){
  if(MotorControlParameters.ElectricalAngle==0)
    MotorControlParameters.Step=1;
  else if(MotorControlParameters.ElectricalAngle==PHASE_SHIFT_60)
    MotorControlParameters.Step=2;
  else if(MotorControlParameters.ElectricalAngle==PHASE_SHIFT_120)
    MotorControlParameters.Step=3;
  else if(MotorControlParameters.ElectricalAngle==(PHASE_SHIFT_120+PHASE_SHIFT_60))
    MotorControlParameters.Step=4;
  else if(MotorControlParameters.ElectricalAngle==(-PHASE_SHIFT_120))
    MotorControlParameters.Step=5;
  else if(MotorControlParameters.ElectricalAngle==(-PHASE_SHIFT_60))
    MotorControlParameters.Step=6;

}

void MotorFSMService(void){
	switch(MotorControlParameters.actualMotorState){
	case MOTOR_SOFT_BRAKING:
		soft_start_counter--;
		soft_start_update_event=1;
		if(soft_start_counter==0){
			MotorControlParameters.actualMotorState=MOTOR_NOT_ACTIVE;
			MotorTurnOff();
		}
		break;
	case MOTOR_SOFT_START:
		soft_start_counter++;
		soft_start_update_event=1;
		if(soft_start_counter>=20)
			MotorControlParameters.actualMotorState=MOTOR_ACTIVE;
		break;
	case MOTOR_ACTIVE:
		if(MotorControlParameters.PrevDirection!=MotorControlParameters.Direction){
			MotorTurnOffSlow();
		}
	}


}

void CallbackUpdateTimerPulse(void){
	if(soft_start_update_event){
	  soft_start_update_event=0;
	  MotorUpdateTimePulse(soft_start_counter*25);
	}
}
