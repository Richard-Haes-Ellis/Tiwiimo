/* CODIGO TOMADO DEL EJEMPLO uart_echo.c DE TEXAS INSTRUMENT */

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

/* ***************************************************************************
 * VARIABLES NECESARIAS PARA EMPLEAR LAS FUNCIONES, 
 * YA QUE ES EL VECTOR EN EL QUE SE GUARDARA EL VALOR LEIDO POR UART 
 * ************************************************************************** */
#define N 20
uint8_t UART0read[N];
uint8_t UART1read[N];

/* ****************************************************************************
 *
 * Seria necesario asociar en el fichero startup_ccs.c las interrupciones aqui descritas
 * a las UART 0 y 1.
 * 
 * **************************************************************************** */
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif


void UARTIntHandler(void)
{
    uint32_t ui32Status;

    // Get the interrrupt status.
    ui32Status = UARTIntStatus(UART0_BASE, true);

    // Clear the asserted interrupts.
    UARTIntClear(UART0_BASE, ui32Status);

    // Loop while there are characters in the receive FIFO.
    unsigned char i=0;

    while(UARTCharsAvail(UART0_BASE))
    {
        
        // Read the next character from the UART and write it back to the UART.
        UART0read[i]=(uint8_t)UARTCharGetNonBlocking(UART0_BASE);
        i++;
        
        // Blink the LED to show a character transfer is occuring.
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);

        
        // Delay for 1 millisecond.  Each SysCtlDelay is about 3 clocks.
        SysCtlDelay(g_ui32SysClock / (1000 * 3));

        // Turn off the LED
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
    }
}
/* ****************************************************************** */

void UART1IntHandler(void)
{
    uint32_t ui32Status;

    // Get the interrrupt status.
    ui32Status = UARTIntStatus(UART1_BASE, true);


    // Clear the asserted interrupts.
    UARTIntClear(UART1_BASE, ui32Status);

    // Loop while there are characters in the receive FIFO.
    unsigned char j=0;
    while(UARTCharsAvail(UART1_BASE))
    {
        // Read the next character from the UART
        UART1read[j]=(uint8_t)UARTCharGetNonBlocking(UART1_BASE);
        j++;
        
        // Blink the LED to show a character transfer is occuring.
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);

        
        // Delay for 1 millisecond.  Each SysCtlDelay is about 3 clocks.
        SysCtlDelay(g_ui32SysClock / (1000 * 3));

        // Turn off the LED
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
    }
}
/* ****************************************************************** */

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    // Loop while there are more characters to send.
    while(ui32Count--)
    {
        // Write the next character to the UART.
        UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}
/* ****************************************************************** */
void UART1Send(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    // Loop while there are more characters to send.
    while(ui32Count--)
    {
        
        // Write the next character to the UART.
        UARTCharPutNonBlocking(UART1_BASE, *pui8Buffer++);
    }
}

/*  ****************************************************************** 
 *  EJEMPLO DE FUNCION ECHO GUARDANDO EL DATO LEIDO EN UNA VARIABLE 
 *  ***************************************************************** */
 
/*
   if (UART0read[0]!=0){
        uint32_t sizeBuff = 0;
        while(UART0read[sizeBuff]!=0){sizeBuff++;}
        UARTSend( UART0read, sizeBuff);
        uint32_t i = 0;
        while(i<N){UART0read[i]=0;i++;}
    }
*/

/*
   if (UART1read[0]!=0){
        uint32_t sizeBuff = 0;
        while(UART1read[sizeBuff]!=0){sizeBuff++;}
        UART1Send( UART1read, sizeBuff);
        uint32_t i = 0;
        while(i<N){UART1read[i]=0;i++;}
    }
*/
/* ****************************************************************** */

