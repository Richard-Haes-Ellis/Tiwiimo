#ifndef _USB_MOUSE_STRUCTS_H_
#define _USB_MOUSE_STRUCTS_H_

extern uint32_t HIDMouseHandler(void *pvCBData, uint32_t ui32Event,
		uint32_t ui32MsgData, void *pvMsgData);

extern tUSBDHIDMouseDevice g_sMouseDevice;

#endif