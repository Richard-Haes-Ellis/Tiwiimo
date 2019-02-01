
// Librarias
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidkeyb.h"
#include <usblib/device/usbdhidmouse.h>
#include "drivers/buttons.h"
#include "drivers/pinout.h"
#include "usb_mouse_structs.h"
#include "utils/uartstdio.h"

// Flags de operacion
volatile bool g_bConnected = false; // Varibale que indica si esta conectado a PC
volatile bool g_bSuspended = false; // Variable que indica si se ha descconectado del bus USB
volatile uint32_t g_ui32SysTickCount;
#define SYSTICKS_PER_SECOND     100

// Varibles de estado del raton
volatile enum{
    // Raton sin configurar
    STATE_UNCONFIGURED,
	// Nada que mandar y a la espera de datos
    STATE_IDLE,
	// Estado de suspenso
	STATE_SUSPEND,
    // Esperando a los datos para enviar (No lo usamos)
    STATE_SENDING
}

// Inicialmente marcamos el dispositivo como no configurado
g_iMouseState = STATE_UNCONFIGURED;

// Runtina de manejo de eventos referidos al puerto USB
uint32_t HIDMouseHandler(void *pvCBData, uint32_t ui32Event,
		uint32_t ui32MsgData, void *pvMsgData) {

	switch (ui32Event)
	{
		// Si se conecta al bus ui32Event se pondra a USB_EVENT_CONNECTED
		case USB_EVENT_CONNECTED:
		{
			g_bConnected = true;
			g_bSuspended = false;
			break;
		}

		// Si se desconecta al bus ui32Event se pondra a USB_EVENT_DISCONNECTED.
		case USB_EVENT_DISCONNECTED:
		{
			g_bConnected = false;
			break;
		}

		// Nos vamos al estado de espera despues de haber enviado informacion
		case USB_EVENT_TX_COMPLETE: {
			g_iMouseState = STATE_IDLE;
			break;
		}

		// Si se ha suspendido el bus USB ui32Event saltara al estado USB_EVENT_SUSPEND
		case USB_EVENT_SUSPEND: {
			g_iMouseState = STATE_SUSPEND;
			g_bSuspended = true;
			break;
		}

		// Si el bus se recupera volvemos al estado de IDLE
		case USB_EVENT_RESUME: {
			g_iMouseState = STATE_IDLE;
			g_bSuspended = false;
			break;
		}

		// Cualquier otro evento la ignoramos
		default:
		{
			break;
		}
	}

	return (0);
}

void SysTickIntHandler(void){
    g_ui32SysTickCount++;
}

int main(void)
{
	uint_fast32_t ui32LastTickCount;
    bool bLastSuspend;
    uint32_t ui32SysClock;
    uint32_t ui32PLLRate;

    uint8_t xDistance=0,yDistance=0;

    // Run from the PLL at 120 MHz.
    ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480), 120000000);

    // Configure the device pins for this board.
    PinoutSet(false, true);

    // Initialize the buttons driver
    ButtonsInit();

    // Enable UART0
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    // Initialize the UART for console I/O.
    UARTStdioConfig(0, 115200, ui32SysClock);

    // Not configured initially.
    g_bConnected = false;
    g_bSuspended = false;
    bLastSuspend = false;

    // Initialize the USB stack for device mode.
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    // Tell the USB library the CPU clock and the PLL frequency.  This is a
    // new requirement for TM4C129 devices.
    SysCtlVCOGet(SYSCTL_XTAL_25MHZ, &ui32PLLRate);
    USBDCDFeatureSet(0, USBLIB_FEATURE_CPUCLK, &ui32SysClock);
    USBDCDFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

    // Pass our device information to the USB HID device class driver,
    // initialize the USB controller and connect the device to the bus.
    USBDHIDMouseInit(0, &g_sMouseDevice);

    // Set the system tick to fire 100 times per second.
    ROM_SysTickPeriodSet(ui32SysClock / SYSTICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();


    // Initial Message
    UARTprintf("\033[2J\033[H\n");
    UARTprintf("******************************\n");
    UARTprintf("*         usb-mouse	         *\n");
    UARTprintf("******************************\n");

    // The main loop starts here.  We begin by waiting for a host connection
    // then drop into the main keyboard handling section.  If the host
    // disconnects, we return to the top and wait for a new connection.
    while(1)
    {
        uint8_t ui8Buttons;
        uint8_t ui8ButtonsChanged;

        // Tell the user what we are doing and provide some basic instructions.
        UARTprintf("\nWaiting For Host...\n");

        // Nos quedamos esperado si no esta conectado al host (PC)
        while(!g_bConnected){}

        // Una vez connectada informamos por UART
        UARTprintf("\nHost Connected...\n");

        // Marcamos el estado de espera
        g_iMouseState = STATE_IDLE;

        // Declaramos variable de boton
        uint8_t press = 0;

        // En principio marcamos como bus no suspenso (Ya que nos acabamos de conectar)
        bLastSuspend = false;

        // Continuamos con nuestra logica de programa (Funcionnalidad del raton)
        // mientras estamos conectados. Esta variable es manejada por el MouseHandler
        // en funcion de los eventos que ocurran
        while(g_bConnected){
            // Comprobamos si el estado de suspenso ha cambiado
            if(bLastSuspend != g_bSuspended){
                // En caso de que si informamos por UART
                bLastSuspend = g_bSuspended;
                if(bLastSuspend){
                    UARTprintf("\nBus Suspended ... \n");
                }else{
                    UARTprintf("\nHost Connected ... \n");
                }
            }
            // Si estamos en el estado de espera podemos realizar las funcionalidades normales
            if (g_iMouseState == STATE_IDLE) {
            	// Comprobamos si los botones han sido pulsados
				ButtonsPoll(&ui8ButtonsChanged, &ui8Buttons);

				//
				// Set button 1 if right pressed.
				if (ui8Buttons & RIGHT_BUTTON)
				{

					// calculate the movement distance
					int8_t xDistance = 100;
					int8_t yDistance = 0;

					GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);

					USBDHIDMouseStateChange((void *) &g_sMouseDevice, xDistance,
											yDistance, MOUSE_REPORT_SIZE);
				}

				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);

				USBDHIDMouseStateChange((void *) &g_sMouseDevice, 0, 0,
				MOUSE_REPORT_SIZE);

				//
				// Set button 2 if left pressed.
				//
				if (ui8Buttons & LEFT_BUTTON)
				{

					if (!press)
					{
						USBDHIDMouseStateChange((void *) &g_sMouseDevice, 0, 0,MOUSE_REPORT_BUTTON_1); //press
						press = 1;
					}
					else
					{
						USBDHIDMouseStateChange((void *) &g_sMouseDevice, 0, 0,0);
						press = 0;
					}
				}
			}
		}
    }
}
