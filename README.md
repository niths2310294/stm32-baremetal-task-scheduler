# Bare-Metal Preemptive Task Scheduler on STM32

## Overview

This project implements a bare-metal real-time task scheduler on an STM32F407VET6 ARM CORTEX M4 Microcontoller without using any external RTOS such as FreeRTOS or Zephyr. The objective of the project is to demonstrate how real-time scheduling, context switching, and task management can be implemented directly at the firmware level.

The scheduler uses the SysTick timer for periodic timing and the PendSV for context switching. Multiple independent tasks run concurrently, each with its own stack. Task execution is managed using Task Control Blocks (TCB), which store scheduling parameters such as period, deadline, priority, and stack pointer.

The system simulates a simple vehicle monitoring and control system with tasks for motor control, rain detection, distance tracking, emergency stop handling, and system reporting.

---

## Features

- Custom bare-metal task scheduler
- Context switching using PendSV interrupt
- Periodic timing using SysTick timer
- Task Control Blocks (TCB) for task management
- Individual task stacks using PSP (Process Stack Pointer)
- Deadline monitoring for real-time constraints
- UART debugging and system reporting
- PWM motor control
- Rain sensor monitoring
- Emergency stop functionality

---

## System Architecture

The scheduler relies on the ARM Cortex-M interrupt mechanism to manage task execution.

SysTick generates periodic interrupts that trigger the scheduler. The scheduler determines which task should run next. PendSV then performs the context switch between tasks.

```
SysTick Interrupt
        ↓
Scheduler decides next task
        ↓
PendSV interrupt triggered
        ↓
Context switch
        ↓
Next task execution
```

Main components used:

- **SysTick** – Generates periodic interrupts for scheduling
- **PendSV** – Handles context switching
- **PSP (Process Stack Pointer)** – Maintains independent task stacks
- **TCB (Task Control Block)** – Stores task scheduling parameters

---

## Tasks Implemented

The scheduler manages multiple tasks that simulate a small vehicle control system.

| Task ID | Task Name | Description |
|-------|------------|-------------|
| Task 0 | Deadline Test | Intentionally delays execution to demonstrate deadline violations |
| Task 1 | Rain Detection | Monitors rain sensor input |
| Task 2 | Motor Control | Controls a DC motor using PWM |
| Task 3 | Distance Tracking | Simulates distance travelled by the vehicle |
| Task 4 | Emergency Stop | Stops the motor when an external stop event occurs |
| Task 5 | System Reporter | Prints scheduler information and deadline violations via UART |

---

## Task Control Block Structure

Each task is represented using a Task Control Block structure.

```c
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
    uint32_t next_due;
} TCB_t;
```

The TCB stores:

- Task function pointer
- Execution period
- Deadline constraint
- Stack pointer
- Task priority
- Deadline miss count

---

## Context Switching

Context switching is performed inside the PendSV interrupt handler.

Steps involved in a context switch:

1. Save registers of the currently running task.
2. Store the updated stack pointer in the task's TCB.
3. Load the next task’s stack pointer.
4. Restore the registers of the next task.
5. Resume execution of the next task.

This mechanism is similar to how many RTOS kernels implement task switching internally.

---

## Deadline Monitoring

The scheduler continuously checks whether tasks meet their deadlines. If a task exceeds its deadline, the violation is reported through UART.


---

## Hardware Components

- STM32F407VET6 ARM Cortex-M4 Microcontroller
- DC Motor Driver
- Rain Sensor Module
- Push Button (Emergency Stop)
- UART interface for debugging
- Servo motor

---



## Project Structure

```
project/
│
├── Core
│   ├── Inc
│   └── Src
│
├── Drivers
│
├── cooperative_scheduler.ioc
├── STM32F407VETX_FLASH.ld
├── STM32F407VETX_RAM.ld
└── README.md
```

---

## How to Run the Project

1. Open **STM32CubeIDE**
2. Import the project

```
File → Import → Existing Projects into Workspace
```

3. Build the project

```
Project → Build Project
```

4. Connect the STM32 board using **ST-Link**

5. Flash the firmware

```
Run → Debug
```

6. Open a serial terminal to monitor UART output.



---



## Author

**Nithish S**




