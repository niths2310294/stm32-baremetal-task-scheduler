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


