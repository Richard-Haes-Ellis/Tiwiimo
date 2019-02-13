uint32_t HIDMouseHandler(void *pvCBData, uint32_t ui32Event,uint32_t ui32MsgData, void *pvMsgData){
switch (ui32Event) {
	// Si se conecta al bus, ui32Event se pondra a USB_EVENT_CONNECTED
	case USB_EVENT_CONNECTED:
	{
		g_iMouseState = STATE_IDLE; // Estado de espera
		g_bConnected = true; // Inidcamos conexion
		g_bSuspended = false;	// Y no suspenso
		break;
	}
	// Si se desconecta al bus ui32Event se pondra a USB_EVENT_DISCONNECTED.
	case USB_EVENT_DISCONNECTED:
	{
		g_iMouseState = STATE_UNCONFIGURED; // Estado desconfigurado
		g_bConnected = false; // Indicamos desconexion
		break;
	}
	// Nos vamos al estado de espera despues de haber enviado informacion
	case USB_EVENT_TX_COMPLETE:
	{
		g_iMouseState = STATE_IDLE; // Estado de espera
		break;
	}
	// Si se ha suspendido el bus USB ui32Event saltara al estado USB_EVENT_SUSPEND
	case USB_EVENT_SUSPEND:
	{
		g_iMouseState = STATE_SUSPEND; // Estado de suspension
		g_bSuspended = true; // Indicamos suspension
		break;
	}
	// Si el bus se recupera volvemos al estado de IDLE
	case USB_EVENT_RESUME:
	{
		g_iMouseState = STATE_IDLE; // Estado de espera
		g_bSuspended = false; // Indicamos no suspension
		break;
	}

	// Cualquier otro evento la ignoramos
	default:{ break;}
}
	return (0);
}
