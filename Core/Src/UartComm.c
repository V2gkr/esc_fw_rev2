/*
 * UartComm.c
 *
 *  Created on: 27 Jan 2026
 *      Author: vhrysenk
 */
#include "main.h"
#include <stdint.h>
#include <stddef.h>
#include "UartComm.h"
#include "MotorControl.h"

typedef union{
	float fword;
	uint32_t iword;
	uint8_t bword[4];
}MemDivider;

typedef enum{
  IDLE=0,
  BUSY=1,
}UartState;

uint8_t UartCommBuf[30];
uint8_t UartTxBuf[sizeof(UartCommFrameStruct)];
uint8_t RxBufPointer;
UartState RxState=IDLE;

uint8_t CalculateCRC(uint8_t * buf);
void EmptyFunc(void){

}

const uart_element_struct MotorSetDirection={.addr_low=MOTOR_CTRL_REG,.addr_high=0x01,.val=&MotorControlParameters.Direction,.val_type=type_uint8,.callback=&EmptyFunc};

const uart_element_struct MotorTriggerTurnOn={.addr_low=MOTOR_CTRL_REG,.addr_high=0x02,.callback=&MotorTurnOn};
const uart_element_struct MotorTriggerTurnOff={.addr_low=MOTOR_CTRL_REG,.addr_high=0x03,.callback=&MotorTurnOffSlow};

const uart_element_struct MotorTriggerActDiagnostics={.addr_low=MOTOR_DATA_REG,.addr_high=0x01,.callback=&MotorTurnOn};
const uart_element_struct MotorGetRotationSpeed={.addr_low=MOTOR_DATA_REG,.addr_high=0x02,.val=&MotorControlParameters.RPM_measured,.val_type=type_float,.callback=&EmptyFunc};
const uart_element_struct MotorGetCurrent={.addr_low=MOTOR_DATA_REG,.addr_high=0x03,.val=&MotorControlParameters.Current_Measured,.val_type=type_float,.callback=&EmptyFunc};

const uart_element_struct * const MotorSettingsGroup[]={&MotorSetDirection,&MotorTriggerTurnOn,&MotorTriggerTurnOff};

const uart_element_struct * const MotorDataGroup[]={&MotorTriggerActDiagnostics,&MotorGetRotationSpeed,&MotorGetCurrent};

void UartCommInit(void){
  HAL_UARTEx_ReceiveToIdle_DMA(&huart3,UartCommBuf,30);
}

void UartCommCallback(uint8_t size){
  RxBufPointer=size;
  RxState=BUSY;
  HAL_UARTEx_ReceiveToIdle_DMA(&huart3,UartCommBuf,30);
}

void UartCommService(void){
	UartCommFrameStruct * pFrame=(UartCommFrameStruct*)UartCommBuf;
	UartCommFrameStruct * pTxFrame=(UartCommFrameStruct*)UartTxBuf;
	uart_element_struct ** requestedRegGroup=NULL;
	MemDivider mem_divider;
	if(RxState!=BUSY)
		return;
	RxState=IDLE;
	if(CalculateCRC(UartCommBuf)!=pFrame->crc)
		return;
	if(pFrame->sof!=SOF_BYTE)
		return;
	//estimation of region of registers
	switch(pFrame->addr_low){
		case MOTOR_CTRL_REG:
			requestedRegGroup=(uart_element_struct ** )MotorSettingsGroup;
			break;
		case MOTOR_DATA_REG:
			requestedRegGroup=(uart_element_struct ** )MotorDataGroup;
			break;
		default:
			requestedRegGroup=(uart_element_struct ** )NULL;
			break;
	}
	if(requestedRegGroup==NULL)
		return;

	switch(pFrame->msg_type){
	case type_write_data:
		if(requestedRegGroup[pFrame->addr_high-1]->val==NULL)
			return;
		switch(requestedRegGroup[pFrame->addr_high-1]->val_type){
		case type_float:
			for(uint8_t i=0;i<sizeof(MemDivider);i++)
				mem_divider.bword[i]=pFrame->payload[i];
			*(float*)(requestedRegGroup[pFrame->addr_high-1]->val)=mem_divider.fword;
			break;
		case type_int32:
			for(uint8_t i=0;i<sizeof(MemDivider);i++)
				mem_divider.bword[i]=pFrame->payload[i];
			*(int32_t*)(requestedRegGroup[pFrame->addr_high-1]->val)=mem_divider.iword;
			break;
		case type_uint8:
			*(uint8_t*)(requestedRegGroup[pFrame->addr_high-1]->val)=pFrame->payload[0];
			break;

		}
		//*requestedRegGroup[pFrame->addr_high]->val=pFrame->payload[0];
		break;
	case type_read_data:
		if(requestedRegGroup[pFrame->addr_high-1]->val==NULL)
			return;
		pTxFrame->sof=0xAA;
		pTxFrame->addr_low=pFrame->addr_low;
		pTxFrame->addr_high=pFrame->addr_high;
		switch(requestedRegGroup[pFrame->addr_high-1]->val_type){
		case type_float:
			mem_divider.fword=*(float*)(requestedRegGroup[pFrame->addr_high-1]->val);
			for(uint8_t i=0;i<sizeof(MemDivider);i++)
				pTxFrame->payload[i]=mem_divider.bword[i];
			break;
		case type_int32:
			mem_divider.iword=*(int32_t*)(requestedRegGroup[pFrame->addr_high-1]->val);
			for(uint8_t i=0;i<sizeof(MemDivider);i++)
				pTxFrame->payload[i]=mem_divider.bword[i];
			break;
		case type_uint8:
			pTxFrame->payload[0]=*(uint8_t*)(requestedRegGroup[pFrame->addr_high-1]->val);
			break;
		}
		pTxFrame->crc=CalculateCRC(UartTxBuf);
		HAL_UART_Transmit_DMA(&huart3,UartTxBuf, sizeof(UartTxBuf));
		//pFrame->payload[0]=*requestedRegGroup[pFrame->addr_high]->val;
		break;
	case type_trigger:
		if(requestedRegGroup[pFrame->addr_high-1]->callback!=&EmptyFunc)
			requestedRegGroup[pFrame->addr_high-1]->callback();
		break;
	}

}

uint8_t CalculateCRC(uint8_t * buf){
  uint8_t crc = 0;
  size_t i, j;
  for (i = 0; i < sizeof(UartCommFrameStruct)-1; i++) {
      crc ^= buf[i];
      for (j = 0; j < 8; j++) {
          if ((crc & 0x80) != 0)
              crc = (uint8_t)((crc << 1) ^ 0x07);
          else
              crc <<= 1;
      }
  }
  return crc;
}
