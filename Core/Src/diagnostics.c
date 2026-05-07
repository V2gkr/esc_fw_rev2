/*
 * diagnostics.c
 *
 *  Created on: 28 Jan 2026
 *      Author: vhrysenk
 */

#include "main.h"
#include "stm32g4xx_hal_gpio.h"
#include "diagnostics.h"

esc_data_struct esc_data;

uint16_t CheckAlarms(void){
    if(esc_data.alarms){
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, 1);
    }
    else {
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, 0);
    }
    return esc_data.alarms;
}