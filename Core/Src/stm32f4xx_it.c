/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "main.h"
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */
#define NUM_TASKS 6

typedef struct
{
    void (*task)(void);
    uint32_t period;
    uint32_t last_run;
    uint32_t deadline;
    uint32_t stack_ptr;
    uint32_t deadline_missed;
    uint32_t priority;
    uint32_t run_count;
} TCB_t;

extern TCB_t tcb[NUM_TASKS];
extern volatile uint32_t GlobalTick;
extern volatile uint8_t current_task;
extern volatile uint8_t next_task;
/* USER CODE END EV */




/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
    printf("HardFault!\n");
    while(1);
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */

extern volatile uint8_t system_stop;

__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile (
        "CPSID I                 \n" // Disable interrupts

        "MRS R0, PSP             \n" // Get current task stack pointer
        "CBZ R0, skip_save       \n" // First time -> skip saving

        /* Save R4-R11 of current task */
        "STMDB R0!, {R4-R11}     \n"

        /* Save updated stack pointer into TCB */
        "LDR R1, =current_task   \n"
        "LDRB R1, [R1]           \n"
        "LDR R2, =tcb            \n"
        "MOV R3, #32             \n" // sizeof(TCB_t)
        "MLA R1, R1, R3, R2      \n"
        "STR R0, [R1, #16]       \n" // store stack_ptr

    "skip_save:                 \n"

        /* Load next task index */
        "LDR R1, =next_task      \n"
        "LDRB R1, [R1]           \n"

        /* Update current_task = next_task */
        "LDR R0, =current_task   \n"
        "STRB R1, [R0]           \n"

        /* Load next task stack pointer */
        "LDR R2, =tcb            \n"
        "MOV R3, #32             \n"
        "MLA R1, R1, R3, R2      \n"
        "LDR R0, [R1, #16]       \n"

        /* Restore next task registers */
        "LDMIA R0!, {R4-R11}     \n"

        "MSR PSP, R0             \n" // Set PSP

        /* Return to thread mode using PSP */
        "MOV LR, #0xFFFFFFFD     \n"

        "CPSIE I                 \n" // Enable interrupts
        "BX LR                   \n"
    );
}

/**
  * @brief This function handles System tick timer.
  */
extern volatile uint32_t GlobalTick;
void SysTick_Handler(void)
{
    HAL_IncTick();
    GlobalTick++;

    /* Pause scheduler if system stopped */
    if(system_stop)
        return;

    /* Deadline monitoring */
    for(int i = 0; i < NUM_TASKS; i++)
    {
        if((GlobalTick - tcb[i].last_run) > tcb[i].deadline)
        {
            tcb[i].deadline_missed++;

            /* reset timestamp so it doesn't count repeatedly */
            tcb[i].last_run = GlobalTick;
        }
    }

    /* Scheduler trigger (round robin) */
    if(GlobalTick % 5 == 0)
    {
        next_task = (current_task + 1) % NUM_TASKS;

        /* Trigger PendSV for context switch */
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line4 interrupt.
  */
void EXTI4_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI4_IRQn 0 */

  /* USER CODE END EXTI4_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
  /* USER CODE BEGIN EXTI4_IRQn 1 */

  /* USER CODE END EXTI4_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
