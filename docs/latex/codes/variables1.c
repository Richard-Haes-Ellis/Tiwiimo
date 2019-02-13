volatile bool g_bConnected = false; // Varibale que indica si esta conectado a PC
volatile bool g_bSuspended = false; // Variable que indica si se ha desconectado del bus USB
volatile uint32_t g_ui32SysTickCount;	// Contador del sistema (RELOJ)
uint32_t g_ui32PrevSysTickCount = 0; // Almacena valor del contador para contar tiempo

// Buffer para la UART
char string[50];

// ID del sensor BMI160
int  DevID=0;

// Almacena variables del gyroscopo
struct bmi160_gyro_t s_gyroXYZ;

// DATOS DE CALIBRACION
int16_t gyro_off_x = 6;	 	// Offset del eje x
int16_t gyro_off_y = -19;	// Offset del eje y
int16_t gyro_off_z = -19;	// Offset del eje z

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
