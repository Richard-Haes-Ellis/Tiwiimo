//*****************************************************************************
//
// uart_echo.c - Example for reading data from and writing data to the UART in
//               an interrupt driven fashion.
//
// Copyright (c) 2013-2017 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.4.178 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>UART Echo (uart_echo)</h1>
//!
//! This example application utilizes the UART to echo text.  The first UART
//! (connected to the USB debug virtual serial port on the evaluation board)
//! will be configured in 115,200 baud, 8-n-1 mode.  All characters received on
//! the UART are transmitted back to the UART.
//
//*****************************************************************************

//****************************************************************************
//
// System clock rate in Hz.
//
//****************************************************************************
uint32_t g_ui32SysClock;

#define N 20

uint8_t UART0read[N];
uint8_t UART1read[N];
int32_t sizeBuff = 0;
uint8_t flag_UART0=0;
uint8_t flag_UART1=0;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void
UARTIntHandler(void)
{
    uint32_t ui32Status;

    //
    // Get the interrrupt status.
    //
    ui32Status = UARTIntStatus(UART0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    UARTIntClear(UART0_BASE, ui32Status);

    //
    // Loop while there are characters in the receive FIFO.
    //
    unsigned char i=0;

    while(UARTCharsAvail(UART0_BASE))
    {
        //
        // Read the next character from the UART and write it back to the UART.
        //
        //UARTCharPutNonBlocking(UART0_BASE,UARTCharGetNonBlocking(UART0_BASE));
        UART0read[i]=(uint8_t)UARTCharGetNonBlocking(UART0_BASE);
        i++;
        //
        // Blink the LED to show a character transfer is occuring.
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);

        //
        // Delay for 1 millisecond.  Each SysCtlDelay is about 3 clocks.
        //
        SysCtlDelay(g_ui32SysClock / (1000 * 3));

        //
        // Turn off the LED
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
    }
}

void
UART1IntHandler(void)
{
    uint32_t ui32Status;

    //
    // Get the interrrupt status.
    //
    ui32Status = UARTIntStatus(UART1_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    UARTIntClear(UART1_BASE, ui32Status);

    //
    // Loop while there are characters in the receive FIFO.
    //
    unsigned char j=0;
    while(UARTCharsAvail(UART1_BASE))
    {

        // Read the next character from the UART and write it back to the UART.
        UART1read[j]=(uint8_t)UARTCharGetNonBlocking(UART1_BASE);
        j++;
        //
        // Blink the LED to show a character transfer is occuring.
        //
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);

        //
        // Delay for 1 millisecond.  Each SysCtlDelay is about 3 clocks.
        //
        SysCtlDelay(g_ui32SysClock / (1000 * 3));

        //
        // Turn off the LED
        //
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
    }
}
//*****************************************************************************
//
// Send a string to the UART.
//
//*****************************************************************************
void
UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    //
    // Loop while there are more characters to send.
    //
    while(ui32Count--)
    {
        //
        // Write the next character to the UART.
        //
        UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}

void
UART1Send(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    //
    // Loop while there are more characters to send.
    //
    while(ui32Count--)
    {
        //
        // Write the next character to the UART.
        //
        UARTCharPutNonBlocking(UART1_BASE, *pui8Buffer++);
    }
}

//*****************************************************************************
//
// This example demonstrates how to send a string of data to the UART.
//
//*****************************************************************************
int
main(void)
{
    //
    // Set the clocking to run directly from the crystal at 120MHz.
    //
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);
    //
    // Enable the GPIO port that is used for the on-board LED.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    //
    // Enable the GPIO pins for the LED (PN0).
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);

    //
    // Enable the peripherals used by this example.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));
    UARTConfigSetExpClk(UART1_BASE, g_ui32SysClock, 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));
    //
    // Enable the UART interrupt.
    //
    IntEnable(INT_UART0);
    IntEnable(INT_UART1);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);
    //
    // Prompt for text to be entered.
    //
    UARTSend((uint8_t *)"\033[2JUART0: ", 11);
    UART1Send((uint8_t *)"\033[2JUART1: ", 11);

    //
    // Loop forever echoing data through the UART.
    //
    while(1)
    {
        //Se manda a la UART1 lo escrito por la UART0(Debug)
        if (UART0read[0]!=0){
            uint32_t sizeBuff = 0;
            while(UART0read[sizeBuff]!=0){sizeBuff++;}
            UART1Send( UART0read, sizeBuff);
            uint32_t i = 0;
            while(i<N){UART0read[i]=0;i++;}
        }

        {
            //Se manda a la UART0(Debug) lo escrito por la UART1
            if (UART1read[0]!=0){
                uint32_t sizeBuff = 0;
                while(UART1read[sizeBuff]!=0){sizeBuff++;}
                UARTSend( UART1read, sizeBuff);
                uint32_t i = 0;
                while(i<N){UART1read[i]=0;i++;}
            }

    }
    }
}
