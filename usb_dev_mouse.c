// Librarias
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Librerias genericas
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
// Libreria del puerto USB y dispositivos HID
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidkeyb.h"
#include <usblib/device/usbdhidmouse.h>
// Libreria para los GPIO
#include "drivers/buttons.h"
#include "drivers/pinout.h"
// Informacion relativa a nuestro raton
#include "usb_mouse_structs.h"
// Libreria del puerto serie
#include "utils/uartstdio.h"
// Librerias para el sensor gyroscopo
#include "HAL_I2C.h"
#include "sensorlib2.h"
#include "driverlib2.h"


// Flags de operacion
volatile bool g_bConnected = false; // Varibale que indica si esta conectado a PC
volatile bool g_bSuspended = false; // Variable que indica si se ha descconectado del bus USB
volatile uint32_t g_ui32SysTickCount;
uint32_t g_ui32PrevSysTickCount = 0;

#define SYSTICKS_PER_SECOND     100
#define MAX_SEND_DELAY          80
#define MOUSE_REPORT_BUTTON_RELEASE 0x00

// Buffer para el UART
char string[50];
// Infor del sensor
int  DevID=0;

// BMI160/BMM150
int8_t returnValue;
struct bmi160_gyro_t s_gyroXYZ;

//Calibration off-sets
int16_t gyro_off_x = 6;
int16_t gyro_off_y = -19;
int16_t gyro_off_z = -19;

#define N 11 // TIENE QUE SER IMPAR!!
int32_t xfilterBuff[N];
int32_t yfilterBuff[N];

uint8_t cod_err=0;
#define BP 2 // Posicion del booosterpack
uint8_t Bme_OK = 0, Bmi_OK;

int32_t xdata = 0;
int32_t ydata = 0;

int8_t xDistance = 0;
int8_t yDistance = 0;

uint8_t movChange = 0;
uint8_t butChange = 0;

// Varibles de estado del raton
volatile enum
{
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
							uint32_t ui32MsgData, void *pvMsgData)
{

	switch (ui32Event)
	{
	// Si se conecta al bus ui32Event se pondra a USB_EVENT_CONNECTED
	case USB_EVENT_CONNECTED:
	{
		g_iMouseState = STATE_IDLE;
		g_bConnected = true;
		g_bSuspended = false;
		break;
	}

		// Si se desconecta al bus ui32Event se pondra a USB_EVENT_DISCONNECTED.
	case USB_EVENT_DISCONNECTED:
	{
		g_iMouseState = STATE_UNCONFIGURED;
		g_bConnected = false;
		break;
	}

		// Nos vamos al estado de espera despues de haber enviado informacion
	case USB_EVENT_TX_COMPLETE:
	{
		g_iMouseState = STATE_IDLE;
		break;
	}

		// Si se ha suspendido el bus USB ui32Event saltara al estado USB_EVENT_SUSPEND
	case USB_EVENT_SUSPEND:
	{
		g_iMouseState = STATE_SUSPEND;
		g_bSuspended = true;
		break;
	}

		// Si el bus se recupera volvemos al estado de IDLE
	case USB_EVENT_RESUME:
	{
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

void SysTickIntHandler(void)
{
	g_ui32SysTickCount++;
}


bool WaitForSendIdle(uint32_t ui32TimeoutTicks){
    uint32_t ui32Start, ui32Now, ui32Elapsed;

    ui32Start = g_ui32SysTickCount;
    ui32Elapsed = 0;

    while(ui32Elapsed < ui32TimeoutTicks)
    {
        //
        // Is the mouse is idle or we have disconnected, return immediately.
        //
        if((g_iMouseState == STATE_IDLE) ||
           (g_iMouseState == STATE_UNCONFIGURED))
        {
            return(true);
        }

        // Determine how much time has elapsed since we started waiting.  This
        // should be safe across a wrap of g_ui32SysTickCount.  I suspect you
        // won't likely leave the app running for the 497.1 days it will take
        // for this to occur but you never know...

        ui32Now = g_ui32SysTickCount;
        ui32Elapsed = (ui32Start < ui32Now) ? (ui32Now - ui32Start) :
                           (((uint32_t)0xFFFFFFFF - ui32Start) + ui32Now + 1);
    }

    // If we get here, we timed out so return a bad return code to let the
    // caller know. (That means that 497.1 days have passed .. wow
    return(false);
}


uint8_t Test_I2C_dir(uint32_t pos, uint8_t dir)
{
    uint8_t error=100;
    uint32_t I2C_Base=0;
    uint32_t *I2CMRIS;
    if(pos==1) I2C_Base=I2C0_BASE;
    if(pos==2) I2C_Base=I2C2_BASE;
    if(I2C_Base){
    I2CMasterSlaveAddrSet(I2C_Base, dir, 1);  //Modo LECTURA
    I2CMasterControl(I2C_Base, I2C_MASTER_CMD_SINGLE_RECEIVE); //Lect. Simple

    while(!(I2CMasterBusy(I2C_Base)));  //Espera que empiece
        while((I2CMasterBusy(I2C_Base)));   //Espera que acabe
        I2CMRIS= (uint32_t *)(I2C_Base+0x014);
        error=(uint8_t)((*I2CMRIS) & 0x10);
        if(error){
            I2CMRIS=(uint32_t *)(I2C_Base+0x01C);
            *I2CMRIS=0x00000010;
        }
    }
       return error;
}

int32_t medianValue(int32_t sensVal,int32_t buff[N]){

    int8_t i=0;
    int8_t j=0;

    // Desplazamos todo a la izquierda
    for(i=0;i<N-1;i++){
        buff[i] = buff[i+1];
    }

    // Introducimos el valor
    buff[N-1] = sensVal;

    // Ordenamos el vector
    // En orden ascendente
    for (i = 0; i < N; i++){
        // Iteramos por cada elemnto
        for (j = 0; j < N; j++){
            // Y comprobamos si hay alguno mayor que ese
            if (buff[j] > buff[i]){
                int tmp = buff[i];  // Usamos una variable temporal
                buff[i] = buff[j];  // Reemplazamos el valor
                buff[j] = tmp;      // Reemplazamos el valor
            }
        }
    }

    for(i = 0;i<N; i++){
        UARTprintf("%d ",buff[i]);
    }
    UARTprintf(": %d \n",buff[(N+1)/2]);
    return buff[(N+1)/2];
}


int main(void)
{
	bool bLastSuspend;
	uint32_t ui32SysClock;
	uint32_t ui32PLLRate;


	uint8_t xDistance = 0, yDistance = 0;

	// Run from the PLL at 120 MHz.
	ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
	SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
	SYSCTL_CFG_VCO_480),120000000);

	// Configuramos el boosterpack
	Conf_Boosterpack(BP, ui32SysClock);

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
	// USBStackModeSet(0, eUSBModeForceDevice, 0);
	USBStackModeSet(0, eUSBModeDevice,      0);

	// Tell the USB library the CPU clock and the PLL frequency.  This is a
	// new requirement for TM4C129 devices.
	SysCtlVCOGet(SYSCTL_XTAL_25MHZ,            &ui32PLLRate);
	USBDCDFeatureSet(0, USBLIB_FEATURE_CPUCLK, &ui32SysClock);
	USBDCDFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

	// Pass our device information to the USB HID device class driver,
	// initialize the USB controller and connect the device to the bus.
	USBDHIDMouseInit(0, (tUSBDHIDMouseDevice *)&g_sMouseDevice);

	// Set the system tick to fire 100 times per second.
	ROM_SysTickPeriodSet(ui32SysClock / SYSTICKS_PER_SECOND);
	ROM_SysTickIntEnable();
	ROM_SysTickEnable();

	// Initial Message
	UARTprintf("\033[2J\033[H\n");
	UARTprintf("******************************\n");
	UARTprintf("*         usb-mouse	         *\n");
	UARTprintf("******************************\n");

	UARTprintf("\033[2J \033[1;1H Inicializando BMI160... ");
    cod_err = Test_I2C_dir(2, BMI160_I2C_ADDR2);
    if (cod_err)
    {
        UARTprintf("Error 0X%x en BMI160\n", cod_err);
        Bmi_OK = 0;
    }
    else
    {
        UARTprintf("Inicializando BMI160, modo NAVIGATION... ");
        bmi160_initialize_sensor();
        bmi160_config_running_mode(APPLICATION_NAVIGATION);
        UARTprintf("Hecho! \nLeyendo DevID... ");
        readI2C(BMI160_I2C_ADDR2, BMI160_USER_CHIP_ID_ADDR, &DevID, 1);
        UARTprintf("DevID= 0X%x \n", DevID);
        Bmi_OK = 1;
    }


	// The main loop starts here.  We begin by waiting for a host connection
	// then drop into the main keyboard handling section.  If the host
	// disconnects, we return to the top and wait for a new connection.
	while (1)
	{
		uint8_t ui8Buttons;
		uint8_t ui8ButtonsChanged;

		// Tell the user what we are doing and provide some basic instructions.
		UARTprintf("\nWaiting For Host...\n");

		// Indica que esta listo para enchufar
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);

		// Nos quedamos esperado si no esta conectado al host (PC)
		while (!g_bConnected)
		{
		}

		// Indica que esta listo para enchufar
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 1);
		// Una vez connectada informamos por UART
		UARTprintf("\nHost Connected...\n");

		// Marcamos el estado de espera
		g_iMouseState = STATE_IDLE;

		// Declaramos variable de botones
		uint8_t currStateDo,prevStateDo = 0;
		uint8_t currStateUp,prevStateUp = 0;
		uint8_t butStat = 0;

		// En principio marcamos como bus no suspenso (Ya que nos acabamos de conectar)
		bLastSuspend = false;

		// USBDHIDMouseStateChange((void *) &g_sMouseDevice, 0, 0, 0);

		// Continuamos con nuestra logica de programa (Funcionnalidad del raton)
		// mientras estamos conectados. Esta variable es manejada por el MouseHandler
		// en funcion de los eventos que ocurran
		while (g_bConnected)
		{
			// Comprobamos si el estado de suspenso ha cambiado
			if (bLastSuspend != g_bSuspended)
			{
				// En caso de que si informamos por UART
				bLastSuspend = g_bSuspended;
				if (bLastSuspend)
				{
					UARTprintf("\nBus Suspended ... \n");
				}
				else
				{
					UARTprintf("\nHost Connected ... \n");
				}
			}
			// Si estamos en el estado de espera podemos realizar las funcionalidades normales
			if (g_iMouseState == STATE_IDLE)
			{


                if (g_ui32SysTickCount - g_ui32PrevSysTickCount > 1){ // Si ha pasado mas de 10 ms actualizamos
                    g_ui32PrevSysTickCount = g_ui32SysTickCount;
                    if (Bmi_OK){
                        bmi160_read_gyro_xyz(&s_gyroXYZ);

                        // Filtramos los datos
                        xdata = medianValue(s_gyroXYZ.x-gyro_off_x,xfilterBuff);
                        ydata = medianValue(s_gyroXYZ.z-gyro_off_z,yfilterBuff);

                        // QUE RANGO TOMA s_gyroXYX ----> 16 bits!!!
                        // ESCALARLO desde -32768 a 32767
                        // Lo hacemos por casting

                        int32_t scaling = 1; // Rango [1,Inf] A mas valor mas atenuacion
                        int32_t thresh  = 3; // Rangp [1,Inf] A mas valor menos sensible

                        if(xdata/scaling < thresh && xdata/scaling > -thresh){
                            yDistance = 0;
                        }else{
                            yDistance = -(int8_t)xdata/scaling;
                        }

                        if(ydata/scaling < thresh && ydata/scaling > -thresh){
                            xDistance = 0;
                        }else{
                            xDistance = -(int8_t)ydata/scaling;
                        }
                        movChange = 1;

                        sprintf(string, "  GYRO: X:%6d\t Z:%6d\t\n",
                                                        (int8_t)xDistance,
                                                        (int8_t)yDistance);
                        // UARTprintf(string);
                    }
                }

				// Comprobamos si los botones han sido pulsados
				ButtonsPoll(&ui8ButtonsChanged, &ui8Buttons);

				currStateDo = (ui8Buttons & LEFT_BUTTON);
				currStateUp = (ui8Buttons & RIGHT_BUTTON);

				butChange = 0;

				// Detectamos flancos de subida o bajada
				if (currStateDo && !prevStateDo){
				    // UARTprintf("Button DOWN (LEFT  CLICK) ON\n");
					prevStateDo = 1;
					butChange   = 1;
					butStat = MOUSE_REPORT_BUTTON_2;
				}else if (!currStateDo && prevStateDo){
					// UARTprintf("Button DOWN (LEFT  CLICK) OFF\n");
					prevStateDo = 0;
					butChange   = 0;
					butStat = MOUSE_REPORT_BUTTON_RELEASE;
				}

				// Detectamos flancos de subida o bajada
				if (currStateUp && !prevStateUp){
					// UARTprintf("Button UP   (RIGHT CLICK) ON\n");
					prevStateUp = 1;
					butChange   = 1;
					butStat = MOUSE_REPORT_BUTTON_1;
				}else if (!currStateUp && prevStateUp){
					// UARTprintf("Button UP   (RIGHT CLICK) OFF\n");
					prevStateUp = 0;
					butChange   = 1;
					butStat = MOUSE_REPORT_BUTTON_RELEASE;
				}

				if(butChange || movChange){ // Solo cuando pulsamos oo dejamos de pulsar
					// UARTprintf("Button change detected\n");
					// Mandamos el reportaje al host.
					g_iMouseState = STATE_SENDING;

					uint32_t ui32Retcode = 0;
					uint8_t  bSuccess = 0;
					uint32_t numAtemp = 0;

					// UARTprintf("Button stat is: %d\n",butStat);

					while(!bSuccess && numAtemp < 60000){
						numAtemp++;
						ui32Retcode = USBDHIDMouseStateChange((void *) &g_sMouseDevice,xDistance,yDistance,butStat);


						if (ui32Retcode == MOUSE_SUCCESS){
							// Esperamos a que el host reciba el reportaje si ha ido bien
							bSuccess = WaitForSendIdle(MAX_SEND_DELAY);

							// Se ha acabado el tiempo y no se ha puesto en IDLE?
							if (!bSuccess){
								// Asumimos que el host se ha desconectado
								// UARTprintf("Timout de envio\n");
								g_bConnected = 0;
							}
							// UARTprintf("Reporte enviado %d,%d\n",numAtemp,butStat);
						}else{
							// Error al mandar reporte ignoramos petcion e informamos
							// UARTprintf("No ha sido posible enviar reporte.\n");
							bSuccess = false;
						}
					}
					butChange = 0;
					movChange = 0;
				}
			}
		}
	}
}



