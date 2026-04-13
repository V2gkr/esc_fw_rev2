#ifndef __MOTOR_CONFIG_H 
#define __MOTOR_CONFIG_H

#define hoverboard_motor

#define DC_VOLTAGE          12

#define FULL_CYCLE_FREQ     100
#define SIX_STEP_FREQ       (FULL_CYCLE_FREQ*6)

#ifdef drone_motor
#define MOTOR_KV            1200
#define MOTOR_POLE_PAIR     7
#define MAX_RPM             (MOTOR_KV/DC_VOLTAGE)
#define MAX_RPM_ALLOWED     ((MAX_RPM*100)/80) /* max speed without load*/
#endif

#ifdef hoverboard_motor
//phase a yellow, b -green , c - blue /hall a yellow , hall b - blue , hall c - green
#define HALL_EVENTS_PER_ETURN 6
#define MOTOR_POLE_PAIR     (15*HALL_EVENTS_PER_ETURN)
#endif

#define HCLK_CLOCK          160000000
#define HALL_PRESCALER      1599
#define HALL_CLOCK          (HCLK_CLOCK)

#define PHASE_SHIFT_30      (int16_t)(65536/12)
#define PHASE_SHIFT_60      (int16_t)(65536/6)
#define PHASE_SHIFT_90      (int16_t)(65536/4)
#define PHASE_SHIFT_120     (int16_t)(65536/3)

#define TIMER_FREQ          16000
#define TIMER_PERIOD        ((HCLK_CLOCK/(TIMER_FREQ*2))-1) /* x2 because center aligned mode*/
#endif
