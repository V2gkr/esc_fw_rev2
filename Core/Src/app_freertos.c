/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "MotorControl.h"
#include "UartComm.h"
#include "adc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for MotorCtrlTask */
osThreadId_t MotorCtrlTaskHandle;
uint32_t MotorCtrlTaskBuffer[ 128 ];
osStaticThreadDef_t MotorCtrlTaskControlBlock;
const osThreadAttr_t MotorCtrlTask_attributes = {
  .name = "MotorCtrlTask",
  .stack_mem = &MotorCtrlTaskBuffer[0],
  .stack_size = sizeof(MotorCtrlTaskBuffer),
  .cb_mem = &MotorCtrlTaskControlBlock,
  .cb_size = sizeof(MotorCtrlTaskControlBlock),
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for CommTask */
osThreadId_t CommTaskHandle;
uint32_t CommTaskBuffer[ 128 ];
osStaticThreadDef_t CommTaskControlBlock;
const osThreadAttr_t CommTask_attributes = {
  .name = "CommTask",
  .stack_mem = &CommTaskBuffer[0],
  .stack_size = sizeof(CommTaskBuffer),
  .cb_mem = &CommTaskControlBlock,
  .cb_size = sizeof(CommTaskControlBlock),
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartMotorCtrlTask(void *argument);
void StartCommTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of MotorCtrlTask */
  MotorCtrlTaskHandle = osThreadNew(StartMotorCtrlTask, NULL, &MotorCtrlTask_attributes);

  /* creation of CommTask */
  CommTaskHandle = osThreadNew(StartCommTask, NULL, &CommTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartMotorCtrlTask */
/**
  * @brief  Function implementing the MotorCtrlTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartMotorCtrlTask */
void StartMotorCtrlTask(void *argument)
{
  /* USER CODE BEGIN StartMotorCtrlTask */
  /* Infinite loop */
  for(;;)
  {
    MotorFSMService();
    osDelay(10);
  }
  /* USER CODE END StartMotorCtrlTask */
}

/* USER CODE BEGIN Header_StartCommTask */
/**
* @brief Function implementing the CommTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCommTask */
void StartCommTask(void *argument)
{
  /* USER CODE BEGIN StartCommTask */
  /* Infinite loop */
  for(;;)
  {
    UartCommService();
    VbusService();
    osDelay(50);
  }
  /* USER CODE END StartCommTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

