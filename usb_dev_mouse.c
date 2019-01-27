
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

#define SYSTICKS_PER_SECOND     100
volatile bool g_bConnected = false;
volatile bool g_bSuspended = false;
volatile uint32_t g_ui32SysTickCount;
#define MAX_SEND_DELAY          50
volatile bool g_bDisplayUpdateRequired;

volatile enum{
    // Unconfigured.
    STATE_UNCONFIGURED,
	// No keys to send and not waiting on data.
    STATE_IDLE,
	// Suspended state
	STATE_SUSPEND,
    // Waiting on data to be sent out.
    STATE_SENDING
}
g_iMouseState = STATE_UNCONFIGURED;


bool WaitForSendIdle(uint_fast32_t ui32TimeoutTicks)
{
    uint32_t ui32Start;
    uint32_t ui32Now;
    uint32_t ui32Elapsed;

    ui32Start = g_ui32SysTickCount;
    ui32Elapsed = 0;

    while(ui32Elapsed < ui32TimeoutTicks)
    {
        // Is the mouse is idle, return immediately.
        if(g_iMouseState == STATE_IDLE)
        {
            return(true);
        }

        // Determine how much time has elapsed since we started waiting.  This
        // should be safe across a wrap of g_ui32SysTickCount.
        ui32Now = g_ui32SysTickCount;
        ui32Elapsed = ((ui32Start < ui32Now) ? (ui32Now - ui32Start) :
                     (((uint32_t)0xFFFFFFFF - ui32Start) + ui32Now + 1));
    }

    // If we get here, we timed out so return a bad return code to let the
    // caller know.
    return(false);
}

uint32_t HIDMouseHandler(void *pvCBData, uint32_t ui32Event,
		uint32_t ui32MsgData, void *pvMsgData) {

	switch (ui32Event)
	{
		case USB_EVENT_CONNECTED:
		{
			g_bConnected = true;
			g_bSuspended = false;
			break;
		}

		// The host has disconnected from us.
		case USB_EVENT_DISCONNECTED:
		{
			g_bConnected = false;
			break;
		}

		// Nos vamos al estado de espera despues de haber hecho algo
		case USB_EVENT_TX_COMPLETE: {
			g_iMouseState = STATE_IDLE;
			break;
		}

		case USB_EVENT_SUSPEND: {
			g_iMouseState = STATE_SUSPEND;
			g_bSuspended = true;
			break;
		}

		case USB_EVENT_RESUME: {
			g_iMouseState = STATE_IDLE;
			g_bSuspended = false;
			break;
		}

		// We ignore all other events.
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
    UARTprintf("*      usb-dev-keyboard      *\n");
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

        // Wait here until USB device is connected to a host.
        while(!g_bConnected)
        {
        }

        // Update the status.
        UARTprintf("\nHost Connected...\n");

        // Enter the idle state.
        g_iMouseState = STATE_IDLE;

        // Button not pressed
        uint8_t press = 0;

        // Assume that the bus is not currently suspended if we have just been
        // configured.
        bLastSuspend = false;

        // Keep transferring data from the UART to the USB host for as
        // long as we are connected to the host.
        while(g_bConnected)
        {

            // Has the suspend state changed since last time we checked?
            if(bLastSuspend != g_bSuspended)
            {
                // Yes - the state changed. Print state to terminal.
                bLastSuspend = g_bSuspended;
                if(bLastSuspend)
                {
                    UARTprintf("\nBus Suspended ... \n");
                }
                else
                {
                    UARTprintf("\nHost Connected ... \n");
                }
            }
            if (g_iMouseState == STATE_IDLE) {

				//
				// See if the buttons updated.
				//
				ButtonsPoll(&ui8ButtonsChanged, &ui8Buttons);

				//
				// Set button 1 if right pressed.
				//
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
