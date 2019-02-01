#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_types.h>
#include <usblib/usblib.h>
#include <usblib/usbhid.h>
#include <usblib/usb-ids.h>
#include <usblib/device/usbdevice.h>
#include <usblib/device/usbdcomp.h>
#include <usblib/device/usbdhid.h>
#include <usblib/device/usbdhidmouse.h>

#include "usb_mouse_structs.h"

const uint8_t g_pui8LangDescriptor[] = { 4,
USB_DTYPE_STRING, USBShort(USB_LANG_EN_US) };

const uint8_t g_pui8ManufacturerString[] = { (17 + 1) * 2,
USB_DTYPE_STRING, 'T', 0, 'h', 0, 'i', 0, 'a', 0, 'g', 0, 'o', 0, ' ', 0, 'I',
		0, 'n', 0, 'c', 0, '.', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0, ' ',
		0, };

const uint8_t g_pui8ProductString[] = { (18 + 1) * 2,
USB_DTYPE_STRING, 'M', 0, 'o', 0, 'u', 0, 's', 0, 'e', 0, ' ', 0, 'd', 0, 'a',
		0, ' ', 0, 'U', 0, 'N', 0, 'E', 0, 'S', 0, 'P', 0, '-', 0, 'F', 0, 'E',
		0, 'G', 0, };

const uint8_t g_pui8SerialNumberString[] =
		{ (8 + 1) * 2,
		USB_DTYPE_STRING, '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7',
				0, '8', 0 };

const uint8_t g_pui8HIDInterfaceString[] = { (21 + 1) * 2,
USB_DTYPE_STRING, 'H', 0, 'I', 0, 'D', 0, ' ', 0, 'M', 0, 'o', 0, 'u', 0, 's',
		0, 'e', 0, ' ', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a',
		0, 'c', 0, 'e', 0, ' ', 0, ' ', 0, };

const uint8_t g_pui8ConfigString[] = { (25 + 1) * 2,
USB_DTYPE_STRING, 'H', 0, 'I', 0, 'D', 0, ' ', 0, 'M', 0, 'o', 0, 'u', 0, 's',
		0, 'e', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 'f', 0, 'i', 0, 'g', 0, 'u',
		0, 'r', 0, 'a', 0, 't', 0, 'i', 0, 'o', 0, 'n', 0, ' ', 0, ' ', 0 };

const uint8_t * const g_ppui8StringDescriptors[] = { g_pui8LangDescriptor,
		g_pui8ManufacturerString, g_pui8ProductString, g_pui8SerialNumberString,
		g_pui8HIDInterfaceString, g_pui8ConfigString };

#define NUM_STRING_DESCRIPTORS (sizeof(g_ppui8StringDescriptors) /            \
                                sizeof(uint8_t *))

tUSBDHIDMouseDevice g_sMouseDevice = {
USB_VID_TI_1CBE,
USB_PID_MOUSE, 0,
USB_CONF_ATTR_SELF_PWR, HIDMouseHandler, (void *) &g_sMouseDevice,
		g_ppui8StringDescriptors,
		NUM_STRING_DESCRIPTORS, 0, 0 };
