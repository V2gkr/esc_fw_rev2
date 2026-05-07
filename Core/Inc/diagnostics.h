/*
 * diagnostics.h
 *
 *  Created on: 28 Jan 2026
 *      Author: vhrysenk
 */

#ifndef INC_DIAGNOSTICS_H_
#define INC_DIAGNOSTICS_H_



typedef struct{
  float temperature;
  float vbus_volt;
  uint16_t alarms;
}esc_data_struct;
extern esc_data_struct esc_data;
#define ALARM_VBUS_UVLO   0x01
#define ALARM_VBUS_OV     0x02
#define ALARM_TEMPERATURE 0x04

uint16_t CheckAlarms(void);

#endif /* INC_DIAGNOSTICS_H_ */
