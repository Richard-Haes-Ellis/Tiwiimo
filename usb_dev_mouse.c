/*******************************/
/*          Librerias          */
/*******************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Librerias genericas
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"

// Librerias del dirverlib
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

/*******************************/
/*          Defines            */
/*******************************/
#define SYSTICKS_PER_SECOND          100 // Ticks por segundo para contar cada 1 ms
#define MAX_SEND_DELAY                80 // Tiempo supuesto que tarda en mandar un reporte
#define MOUSE_REPORT_BUTTON_RELEASE 0x00 // Macro para definir boton sin pulsar
#define N 3         				     // Numero de muestras a filtrar
#define BP 2 							 // Posicion del booosterpack

/*******************************/
/*          Variables          */
/*******************************/
volatile bool g_bConnected = false; 	// Varibale que indica si esta conectado a PC
volatile bool g_bSuspended = false; 	// Variable que indica si se ha desconectado del bus USB
volatile uint32_t g_ui32SysTickCount;	// Contador del sistema (RELOJ)
uint32_t g_ui32PrevSysTickCount = 0;	// Almacena valor del contador para contar tiempo

// Buffer para la UART
char string[50];

// ID del sensor BMI160
int  DevID=0;

// Almacena variables del gyroscopo
struct bmi160_gyro_t s_gyroXYZ;

// DATOS DE CALIBRACION
int16_t gyro_off_x = 12;	// Offset del eje x
int16_t gyro_off_z = 12;	// Offset del eje z

// Variables de sensibilidad
int32_t scaling = 23; 	// Rango [1,Inf] A mas valor menos sensible
int32_t thresh  = 2; 	// Rangp [1,Inf] A mas valor menos responde a movimientos

// Buffers para almacenar muestras para el filtrado
int32_t xfilterBuff[N]; // Eje x
int32_t yfilterBuff[N]; // Eje y

// Codigo de error
uint8_t cod_err=0;
uint8_t Bme_OK = 0, Bmi_OK;

// Datos filtrados
int32_t xdata = 0; 		// Eje x
int32_t ydata = 0; 		// Eje y

// Datos procesados (Escalado y Umbral)
int8_t xDistance = 0; 	// Eje x
int8_t yDistance = 0; 	// Eje y

// Variables de estado
uint8_t movChange = 0;	// Indica cambio de movimiento
uint8_t butChange = 0;	// Indica cambio en el estado de los botones

// Varibles de estado del raton
volatile enum{
	STATE_UNCONFIGURED,	// Raton sin configurar
	STATE_IDLE,			// Nada que mandar y a la espera de datos
	STATE_SUSPEND,		// Estado de suspenso
	STATE_SENDING		// Esperando a los datos para enviar (No lo usamos)
}

// Inicialmente marcamos el dispositivo como no configurado
g_iMouseState = STATE_UNCONFIGURED;

/*******************************/
/*          Funciones          */
/*******************************/

// Runtina de manejo de eventos referidos al puerto USB
uint32_t HIDMouseHandler(void *pvCBData, uint32_t ui32Event,uint32_t ui32MsgData, void *pvMsgData){
	switch (ui32Event)
	{
		// Si se conecta al bus, ui32Event se pondra a USB_EVENT_CONNECTED
		case USB_EVENT_CONNECTED:
		{
			g_iMouseState = STATE_IDLE;			// Estado de espera
			g_bConnected = true;				// Inidcamos conexion
			g_bSuspended = false;				// Y no suspenso
			break;
		}

		// Si se desconecta al bus ui32Event se pondra a USB_EVENT_DISCONNECTED.
		case USB_EVENT_DISCONNECTED:
		{
			g_iMouseState = STATE_UNCONFIGURED;	// Estado desconfigurado
			g_bConnected = false;				// Indicamos desconexion
			break;
		}

		// Nos vamos al estado de espera despues de haber enviado informacion
		case USB_EVENT_TX_COMPLETE:
		{
			g_iMouseState = STATE_IDLE;			// Estado de espera
			break;
		}

		// Si se ha suspendido el bus USB ui32Event saltara al estado USB_EVENT_SUSPEND
		case USB_EVENT_SUSPEND:
		{
			g_iMouseState = STATE_SUSPEND;		// Estado de suspension
			g_bSuspended = true;				// Indicamos suspension
			break;
		}

		// Si el bus se recupera volvemos al estado de IDLE
		case USB_EVENT_RESUME:
		{
			g_iMouseState = STATE_IDLE;			// Estado de espera
			g_bSuspended = false;				// Indicamos no suspension
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

// Interrupcion de cuenta de reloj del sistema
void SysTickIntHandler(void){
	g_ui32SysTickCount++;
}

// Funcion de espera hasta envio o timout
bool WaitForSendIdle(uint32_t ui32TimeoutTicks){

	uint32_t ui32Start, ui32Now, ui32Elapsed;
	ui32Start = g_ui32SysTickCount; // Medimos el tiempo actual
	ui32Elapsed = 0;

	// Mientras no haya timeout
	while(ui32Elapsed < ui32TimeoutTicks)
	{
		// Si esta el raton en estado de espera o no confugurado retornamos inmediatamete .
		if((g_iMouseState == STATE_IDLE) || (g_iMouseState == STATE_UNCONFIGURED))
		{
			return(true);
		}
		// Determinamos cuanto tiempo ha trascurrido desde que hemos esperado
		// deberia funcionar para una  vuelta  entera  de  g_ui32SysTickCount.
		ui32Now = g_ui32SysTickCount; // Medimos el tiempo actual

		// En el caso de que haya buffer overflow y de la vuelta (FF -> 00)
		// medimos la diferencia correspondiente
		ui32Elapsed = (ui32Start < ui32Now) ? (ui32Now - ui32Start) :
		(((uint32_t)0xFFFFFFFF - ui32Start) + ui32Now + 1);
	}
	// Si hemos llegado aqui esque ha pasado una vuelta entera, es decir 2³² ticks
	// de g_ui32SysTickCount, que se traduce a (0.001ms/tick)*(2³²tick) = 49.71 dias.. osea...
	return(false);
}

// Funcion para testear el sensor del sensorpack
uint8_t Test_I2C_dir(uint32_t pos, uint8_t dir)
{
	uint8_t  error = 100;
	uint32_t I2C_Base = 0;
	uint32_t *I2CMRIS;

	// En funcion de donde tengamos el sensorpack
	if(pos==1) I2C_Base=I2C0_BASE;
	if(pos==2) I2C_Base=I2C2_BASE;

	if(I2C_Base)
	{
		I2CMasterSlaveAddrSet(I2C_Base, dir, 1);  //Modo LECTURA
		I2CMasterControl(I2C_Base, I2C_MASTER_CMD_SINGLE_RECEIVE); //Lect. Simple

		while(!(I2CMasterBusy(I2C_Base)));  //Espera que empiece
		while((I2CMasterBusy(I2C_Base)));   //Espera que acabe

		I2CMRIS= (uint32_t *)(I2C_Base+0x014);
		error=(uint8_t)((*I2CMRIS) & 0x10);
		if(error)
		{
			I2CMRIS=(uint32_t *)(I2C_Base+0x01C);
			*I2CMRIS=0x00000010;
		}
	}
	return error;
}

// Este es el filtro que se aplica a las señales de gyro
int32_t filter(int32_t sensVal, int32_t values[N])
{
	int8_t i = 0;
	int8_t j = 0;
	int32_t avg;

	// Desplazamos todos los elementos a la izquierda
	for (i = 0; i < N - 1; i++)
	{
		values[i] = values[i + 1];
	}

	// Introducimos la medida al ultimo elemento del vector
	values[N - 1] = sensVal;

	// Calculamos el valor medio de todos los elementos del vector
	avg = 0;
	for (i = 0; i < N; i++)
	{
		avg = avg + values[i];
	}
	return avg / N; // Dividimos para la media
}

int main(void)
{
	bool bLastSuspend;
	uint32_t ui32SysClock;
	uint32_t ui32PLLRate;

	// Run from the PLL at 120 MHz.
	ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),120000000);

	// Configuramos el boosterpack
	Conf_Boosterpack(BP, ui32SysClock);

	// Configuramos los pines de la uart (ETHERNET|UART)
	PinoutSet(false, true);

	// Inicializamos los botones de la placa
	ButtonsInit();

	// Habilitamos el periferico UART0
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

	// Inicializamos la UART para la consola .
	UARTStdioConfig(0, 115200, ui32SysClock);

	// Inicialmente el raton estara desconfigurado
	g_bConnected = false;
	g_bSuspended = false;
	bLastSuspend = false;

	// Inicializamos el stack del USB para el modo dispositivo
	USBStackModeSet(0, eUSBModeDevice,      0);

	// Le decimos a la libreria USB el clock de la CPU y la frecuncia de la PLL
	// Es requerido para las placas TM4C129.
	SysCtlVCOGet(SYSCTL_XTAL_25MHZ,            &ui32PLLRate);
	USBDCDFeatureSet(0, USBLIB_FEATURE_CPUCLK, &ui32SysClock);
	USBDCDFeatureSet(0, USBLIB_FEATURE_USBPLL, &ui32PLLRate);

	// Pasamos informacion de nuestro dispositivo al dirver USB HID
	// Inicializamos el controlador USB y conectamos el dispositvo al bus
	USBDHIDMouseInit(0, (tUSBDHIDMouseDevice *)&g_sMouseDevice);

	// Configuramos el reloj del sistema para contar 100 veces por segudo
	ROM_SysTickPeriodSet(ui32SysClock / SYSTICKS_PER_SECOND);
	ROM_SysTickIntEnable();
	ROM_SysTickEnable();

	// Mensaje Inicial
	UARTprintf("\033[2J\033[H\n");
	UARTprintf("******************************\n");
	UARTprintf("*         usb-mouse	         *\n");
	UARTprintf("******************************\n");

	// Comprobamos el funcionamiento del sensor
	UARTprintf("\033[2J \033[1;1H Inicializando BMI160... ");
	cod_err = Test_I2C_dir(2, BMI160_I2C_ADDR2);
	if (cod_err)
	{
		// Fallo del sensor
		UARTprintf("Error 0X%x en BMI160\n", cod_err);
		Bmi_OK = 0;
	}
	else
	{
		// Exito
		UARTprintf("Inicializando BMI160, modo NAVIGATION... ");
		bmi160_initialize_sensor();
		bmi160_config_running_mode(APPLICATION_NAVIGATION);
		UARTprintf("Hecho! \nLeyendo DevID... ");
		readI2C(BMI160_I2C_ADDR2, BMI160_USER_CHIP_ID_ADDR, &DevID, 1);
		UARTprintf("DevID= 0X%x \n", DevID);
		Bmi_OK = 1;
	}

	/* BUCLE PRINCIPAL ****************************************************/
	/* Empezamos esperando a que se conecte el micro a algun host , luego */
	/* Luego entramos en el manejador de botones y movimiento del sensor  */
	/* Si por lo que sea nos desconectamos del host volvemos a la espera  */
	/**********************************************************************/
	while (1)
	{
		uint8_t ui8Buttons;
		uint8_t ui8ButtonsChanged;

		UARTprintf("\nEsperamos al host...\n");

		// Indica que el micro aun no esta listo para usar
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);

		// Nos quedamos esperado si no esta conectado al host (PC)
		while (!g_bConnected){}

		// Indica que el micro esta listo para usar
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 1);

		// Una vez connectada informamos por UART
		UARTprintf("\nHost conectado...\n");

		// Marcamos el estado de espera
		g_iMouseState = STATE_IDLE;

		// Declaramos variable de botones
		uint8_t currB1State,prevB1State = 0;
		uint8_t currB2State,prevB2State = 0;
		uint8_t butReport = 0;

		// En principio marcamos como bus no suspenso (Ya que nos acabamos de conectar)
		bLastSuspend = false;

		// Continuamos con nuestra logica de programa (Funcionnalidad del raton)
		// mientras estamos conectados. Esta variable es manejada por el MouseHandler
		// en funcion de los eventos que ocurran (Conexion/Desconecion/Suspension..)
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
				// Si ha pasado mas de 10 ms actualizamos
				if (g_ui32SysTickCount - g_ui32PrevSysTickCount > 1)
				{
					// Reseteamos el cotador
					g_ui32PrevSysTickCount = g_ui32SysTickCount;
					// Si el sensor esta bien
					if (Bmi_OK)
					{
						// Leemos los datos por I2C
						bmi160_read_gyro_xyz(&s_gyroXYZ);

						// Filtramos los datos
						xdata = filter(s_gyroXYZ.x+gyro_off_x,xfilterBuff);
						ydata = filter(s_gyroXYZ.z+gyro_off_z,yfilterBuff);

						// QUE RANGO TOMA s_gyroXYX ? ----> 16 bits!!!
						// Se DEBE escalar desde -32768 a 32767. Lo hacemos por casting
						if((xdata/scaling) < thresh && (xdata/scaling) > -thresh)
						{	// Si esta dentro del umbral [-thresh,thres] rechazamos
							yDistance = 0;
						}
						else
						{	// En cualquier otro caso lo aceptamosy escalamos el dato
							yDistance = -(int8_t)(xdata/scaling);
						}

						// Lo mismo para el eje x
						if((ydata/scaling) < thresh && (ydata/scaling) > -thresh)
						{
							xDistance = 0;
						}
						else
						{
							xDistance = -(int8_t)(ydata/scaling);
						}

						// Para graficar en Matlab
						// UARTprintf("%d\t %d\t %d;\n",s_gyroXYZ.x,xdata,xDistance);

						// Indicamos entonces que el raton se ha movido
						movChange = 1;
					}
				}

				// Comprobamos si los botones han sido pulsados
				ButtonsPoll(&ui8ButtonsChanged, &ui8Buttons);

				// Actualizamos las variables de estado de los botones
				currB1State = (ui8Buttons & LEFT_BUTTON);
				currB2State = (ui8Buttons & RIGHT_BUTTON);

				butChange = 0;

				// Detectamos flancos de subida o bajada
				if (currB1State && !prevB1State) 	// SUBIDA (0->1)
				{
					prevB1State = 1; // Actualizamos el valor posterior
					butChange   = 1; // Indicamos cambio de estado
					butReport = MOUSE_REPORT_BUTTON_2;
				}
				else if (!currB1State && prevB1State) // BAJADA (1->0)
				{
					prevB1State = 0; // Actualizamos el valor posterior
					butChange   = 1; // Indicamos cambio de estado
					butReport = MOUSE_REPORT_BUTTON_RELEASE;
				}

				// Detectamos flancos de subida o bajada
				if (currB2State && !prevB2State)	// SUBIDA (0->1)
				{
					prevB2State = 1; // Actualizamos el valor posterior
					butChange   = 1; // Indicamos cambio de estado
					butReport = MOUSE_REPORT_BUTTON_1;
				}
				else if (!currB2State && prevB2State) // BAJADA (1->0)
				{
					prevB2State = 0; // Actualizamos el valor posterior
					butChange   = 1; // Indicamos cambio de estado
					butReport = MOUSE_REPORT_BUTTON_RELEASE;
				}

				// Solo mandamos reportes al host si ha habido cambios
				if(butChange || movChange)
				{
					// Indicamos estado de envio
					g_iMouseState = STATE_SENDING;

					uint32_t ui32Retcode = 0;
					uint8_t  bSuccess = 0;
					uint32_t numAtemp = 0;

					// Mandamos el reportaje continuamente si falla
					while(!bSuccess && numAtemp < 60000)
					{
						numAtemp++; // Numero de intentos
						// Mandamos el reporte
						ui32Retcode = USBDHIDMouseStateChange((void *) &g_sMouseDevice,
						xDistance,	// Desplazamiento de pixeles en el eje x
						yDistance,	// Desplazamiento de pixeles en el eje y
						butReport); // Estado de los botones

						// Si ha habido exito enviando el reporte
						if (ui32Retcode == MOUSE_SUCCESS)
						{
							// Esperamos a que el host reciba el reportaje si ha ido bien
							bSuccess = WaitForSendIdle(MAX_SEND_DELAY);

							// Se ha acabado el tiempo y no se ha puesto en IDLE?
							if (!bSuccess)
							{
								// Asumimos que el host se ha desconectado
								g_bConnected = false;
							}
						}
						else
						{
							// Error al mandar reporte ignoramos petcion e informamos
							// UARTprintf("No ha sido posible enviar reporte.\n");
							bSuccess = false;
						}
					}
					// Reseteamos las variables de cambios de estado
					butChange = 0;
					movChange = 0;
				}
			}
		}
	}
}
