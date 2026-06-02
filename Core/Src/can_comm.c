#include "main.h"
#include "string.h"
#include "can_comm.h"
#include "MotorControl.h"
#include "diagnostics.h"
#include "stm32g4xx_hal_fdcan.h"




extern FDCAN_HandleTypeDef hfdcan1;
FDCAN_TxHeaderTypeDef TxHeader;
uint8_t TxData[8];

//assume every 50ms call
void CAN_PublishMessages(void){
    static uint8_t counter=0;
    //TxHeader.Identifier=0x11;
    TxHeader.IdType=FDCAN_STANDARD_ID;
    TxHeader.TxFrameType=FDCAN_DATA_FRAME;
    //TxHeader.DataLength=FDCAN_DLC_BYTES_8;
    TxHeader.ErrorStateIndicator=FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch=FDCAN_BRS_OFF;
    TxHeader.FDFormat=FDCAN_CLASSIC_CAN;
    TxHeader.TxEventFifoControl=FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker=0;
    //motor control data every possible time , least priority
    //temperature and voltage every 1-2 second
    //status - highest priority , when some statuses went active
    counter++;
    if(esc_data.alarms || MotorControlParameters.alarms){
        TxHeader.Identifier=ESC_DATA_FILTER|ESC_ALARMS_ID;
        TxHeader.DataLength=FDCAN_DLC_BYTES_4;
        memcpy(TxData,(uint8_t*)&esc_data.alarms,2);
        memcpy(TxData+2,(uint8_t*)&MotorControlParameters.alarms,2);
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1,&TxHeader,TxData);
    }
    else if(counter==20){
        TxHeader.Identifier=ESC_DATA_FILTER|ESC_TEMP_ID;
        TxHeader.DataLength=FDCAN_DLC_BYTES_4;
        memcpy(TxData,(uint8_t*)&esc_data.temperature,4);
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1,&TxHeader,TxData);
    }
    else if(counter==40){
        TxHeader.Identifier=ESC_DATA_FILTER|ESC_VBUS_ID;
        TxHeader.DataLength=FDCAN_DLC_BYTES_4;
        memcpy(TxData,(uint8_t*)&esc_data.vbus_volt,4);
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1,&TxHeader,TxData);
        counter=0;
    }
    else if(MotorControlParameters.actualMotorState==MOTOR_ACTIVE){
        TxHeader.Identifier=ESC_DATA_FILTER|ESC_TELEMETRY_ID;
        TxHeader.DataLength=FDCAN_DLC_BYTES_8;
        memcpy(TxData,(uint8_t*)&MotorControlParameters.Current_Measured,4);
        memcpy(TxData+4,(uint8_t*)&MotorControlParameters.RPM_measured,4);
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1,&TxHeader,TxData);
    }
}