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
