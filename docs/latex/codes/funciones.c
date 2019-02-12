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
			g_iMouseState = STATE_UNCONFIGURED;		// Estado desconfigurado
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
