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
