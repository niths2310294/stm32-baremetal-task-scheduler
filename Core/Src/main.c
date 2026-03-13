/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdlib.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define NUM_TASKS 6
#define STACK_SIZE 512

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

/* Function Prototype */
void Task_Stack_Init(int i);
/* USER CODE END PTD */

/* USER CODE BEGIN PV */
volatile uint32_t GlobalTick = 0;
volatile uint8_t current_task = 0;
volatile uint8_t next_task = 0;

TCB_t tcb[NUM_TASKS];
uint32_t task_stacks[NUM_TASKS][STACK_SIZE];

volatile uint32_t distance = 0;
volatile uint8_t system_stop = 0;
volatile uint8_t stop = 0;


/* USER CODE END PV */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Task_Stack_Init(int i);
/* USER CODE BEGIN PFP */
void Task_DeadlineTest(void);
void Task_Motor(void);
void Task_Rain(void);
void Task_distance(void);
void Task_stop(void);
void Task_Reporter(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#include <stdio.h>
volatile uint8_t rain_detected = 0;


uint32_t motor_stop_time = 0;

volatile uint32_t rain_start_time = 0;
volatile uint8_t rain_deadline_reported = 0;

// System State Variables
uint32_t fuel_val = 0;
uint8_t safety_status = 0; // 0 = OK, 1 = CRASH



void os_delay(uint32_t ms)
{
    uint32_t start = GlobalTick;

    while((GlobalTick - start) < ms)
    {
        if(system_stop)
        {
              while(system_stop)
            {
                __WFI();   // sleep until interrupt
            }
        }

        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
}


void Task_Motor(void)
{
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);

    while(1)
    {
        if(system_stop == 0)
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);   // STBY
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);   // AIN1
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); // AIN2

            __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 900);
        }
        else
        {
            __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
        }
        tcb[2].last_run = GlobalTick;
        tcb[2].run_count++;   // track execution
        os_delay(100);
    }
}
static int pos = 1000;
static int dir = 1;

void Task_Rain(void)
{
    tcb[1].last_run = GlobalTick;

    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
    {
        pos += dir*10;

        if(pos >= 2000 || pos <= 1000)
            dir = -dir;

        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pos);
    }

    tcb[1].run_count++;
    os_delay(20);
}
void Task_distance(void)
{
    while(1)
    {
    	tcb[3].last_run = GlobalTick;
        while(system_stop)
        {
            __WFI();
        }

        distance += 1;   // simple deterministic increase

        tcb[3].run_count++;


        os_delay(500);
    }
}

void Task_Reporter(void)
{
    static uint32_t last_miss[NUM_TASKS] = {0};

    while(1)
    {
        for(int i = 0; i < NUM_TASKS; i++)
        {
            if(tcb[i].deadline_missed > last_miss[i])
            {
                printf("Deadline Missed -> Task %d | Count:%lu | Tick:%lu\r\n",
                       i,
                       tcb[i].deadline_missed,
                       GlobalTick);

                last_miss[i] = tcb[i].deadline_missed;
            }
        }

        tcb[5].run_count++;
        tcb[5].last_run = GlobalTick;

        os_delay(200);
    }
}

void Task_stop(void) {
    while(1) {
    	tcb[4].last_run = GlobalTick;
        if(stop) {
            system_stop = 1; // Set global flag

            // EMERGENCY OVERRIDE: Kill hardware immediately here
            __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);

            stop = 0;
        }

        tcb[4].run_count++;
        os_delay(10);
    }
}

void Task_DeadlineTest(void)
{
    while(1)
    {
        tcb[0].last_run = GlobalTick;   // MOVE HERE


        tcb[0].run_count++;

        os_delay(tcb[0].period + 200);
    }
}

void Scheduler_Init(void)
{
    //Task 0: Deadline Test
    tcb[0].task = Task_DeadlineTest;
    tcb[0].period = 150; tcb[0].deadline = 150; tcb[0].priority = 3;
    Task_Stack_Init(0);

    // Task 1: Rain
    tcb[1].task = Task_Rain;
    tcb[1].period = 500; tcb[1].deadline = 1500; tcb[1].priority = 1;
    Task_Stack_Init(1);

    // Task 2: Motor
    tcb[2].task = Task_Motor;
    tcb[2].period = 100; tcb[2].deadline = 150; tcb[2].priority = 1;
    Task_Stack_Init(2);

    // Task 3: Fuel
    tcb[3].task = Task_distance;
    tcb[3].period = 500; tcb[3].deadline = 600; tcb[3].priority = 2;
    Task_Stack_Init(3);

    // Task 4: Safety (New)
    tcb[4].task = Task_stop;
    tcb[4].period = 100; tcb[4].deadline = 200; tcb[4].priority = 0; // Highest priority
    Task_Stack_Init(4);

    tcb[5].task = Task_Reporter;
    tcb[5].period = 1000;
    tcb[5].deadline = 1200;
    tcb[5].priority = 1; // Lower priority than sensors
    Task_Stack_Init(5);

    for(int i = 0; i < NUM_TASKS; i++) {
            tcb[i].last_run = HAL_GetTick();
            tcb[i].deadline_missed = 0;
    }
}




void I2C_Bus_Reset(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE(); // Check if your I2C is on GPIOB

    // Manually control SCL (Pin 6/8 usually)
    GPIO_InitStruct.Pin = GPIO_PIN_6; // Change to your SCL Pin
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // Open Drain
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    for(int i=0; i<9; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_Delay(1);
    }
}



void Idle_Task(void)
{
    __WFI();
}

void Task_Stack_Init(int i) {
    // Point to the end of the stack array (stacks grow downward)
    uint32_t *pStack = &task_stacks[i][STACK_SIZE - 1];

    /* Hardware Saved Stack Frame (Automatic pop on return) */
    *(--pStack) = 0x01000000;             // xPSR (Thumb bit must be 1)
    *(--pStack) = (uint32_t)tcb[i].task;    // PC (Task Entry Point)
    *(--pStack) = 0x00000000;             // LR
    *(--pStack) = 0x12121212;             // R12
    *(--pStack) = 0x03030303;             // R3
    *(--pStack) = 0x02020202;             // R2
    *(--pStack) = 0x01010101;             // R1
    *(--pStack) = 0x00000000;             // R0

    /* Software Saved Stack Frame (Manual pop in PendSV) */
    *(--pStack) = 0x11111111;             // R11
    *(--pStack) = 0x10101010;             // R10
    *(--pStack) = 0x09090909;             // R9
    *(--pStack) = 0x08080808;             // R8
    *(--pStack) = 0x07070707;             // R7
    *(--pStack) = 0x06060606;             // R6
    *(--pStack) = 0x05050505;             // R5
    *(--pStack) = 0x04040404;             // R4

    tcb[i].stack_ptr = (uint32_t)pStack;
}

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, 10);
    return ch;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  // Peripheral Init
  MX_GPIO_Init();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);
  //MX_I2C1_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  //ADXL345_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  printf("TCB size: %d\r\n", sizeof(TCB_t));
  Scheduler_Init();

  current_task = 0;
  next_task = 0;

  // Tell PendSV this is the very first run by setting PSP to 0
  __set_PSP(0);

  // Set PendSV to lowest priority, SysTick to highest
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);

  // Trigger!
  SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
  /* USER CODE END 2 */

/**
  * @brief System Clock Configuration
  * @retval None
  */
}
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
volatile uint8_t crash_flag = 0;
/* USER CODE BEGIN 4 */
/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_press = 0;

    if(GPIO_Pin == GPIO_PIN_4)
    {
        // debounce
        if(HAL_GetTick() - last_press < 200)
            return;

        last_press = HAL_GetTick();

        system_stop = !system_stop;   // TOGGLE

        if(system_stop)
        {
            // Stop motor
            __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
        }
        else
        {
            // Resume motor standby
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
        }
    }
}
/* USER CODE END 4 */
/* USER CODE END 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
