
#ifndef __UARTSEND_H__
#define __UARTSEND_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//       FUNCIONES EMPLEADAS PARA ENVIAR Y RECIBIR POR AMBAS UART
//*****************************************************************************
extern void UARTIntHandler(void);
extern void UART1IntHandler(void);
extern void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count);
extern void UART1Send(const uint8_t *pui8Buffer, uint32_t ui32Count);


//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __UARTSEND_H__
