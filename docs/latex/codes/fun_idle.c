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
