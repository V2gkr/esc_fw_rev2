/*
 * UartComm.h
 *
 *  Created on: 27 Jan 2026
 *      Author: vhrysenk
 */

#ifndef INC_UARTCOMM_H_
#define INC_UARTCOMM_H_

#define SOF_BYTE 	0xAA

#define MOTOR_CTRL_REG	0x40
#define MOTOR_DATA_REG	0x20

enum msg_types{
	type_write_data=2,
	type_read_data=3,
	type_trigger=4 /* 0 or 1 */
};
typedef enum{
	type_float=0,
	type_int32=1,
	type_uint8=2
}val_types;

typedef struct{
	uint8_t sof;
	uint8_t msg_type;
	uint8_t addr_low;
	uint8_t addr_high;
	uint8_t payload[8];
	uint8_t crc;
}UartCommFrameStruct;

typedef struct{
	uint8_t addr_low;
	uint8_t addr_high;
	void(*callback)(void);
	void * val;
	val_types val_type;
}uart_element_struct;

void UartCommInit(void);

void UartCommCallback(uint8_t size);

void UartCommService(void);
#endif /* INC_UARTCOMM_H_ */
