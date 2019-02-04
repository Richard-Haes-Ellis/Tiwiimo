#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>


#include "HAL_I2C.h"
#include "sensorlib2.h"
#include "driverlib2.h"

#include "utils/uartstdio.h"

volatile uint32_t g_ui32SysTickCount;
uint32_t g_ui32PrevSysTickCount = 0;
#define SYSTICKS_PER_SECOND     1000

// =======================================================================
// Function Declarations
// =======================================================================


void SysTickIntHandler(void)
{
    g_ui32SysTickCount++; // Actualiza cada 1 ms
}

int RELOJ;


void Timer0IntHandler(void);
uint8_t Test_I2C_dir(uint32_t I2C_Base, uint8_t dir);


char updateData=0;

char string[50];
int  DevID=0;


 // BMI160/BMM150
 int8_t returnValue;
 struct bmi160_gyro_t s_gyroXYZ;

 //Calibration off-sets
 int16_t gyro_off_x = 6;
 int16_t gyro_off_y = -19;
 int16_t gyro_off_z = -19;

long int inicio;

volatile long int ticks=0;

void IntTick(void){
    ticks++;
}
uint8_t cod_err=0;
#define BP 2
uint8_t Bme_OK = 0, Bmi_OK;
int main(void) {


    uint32_t ui32SysClock;

    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);



    Conf_Boosterpack(BP, RELOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 10*120000-1); // Cada 1ms * 10
    TimerIntRegister(TIMER0_BASE, TIMER_A,Timer0IntHandler);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    TimerEnable(TIMER0_BASE, TIMER_A);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, RELOJ);

    // Set the system tick to fire 1000 times per second.
        ROM_SysTickPeriodSet(ui32SysClock / SYSTICKS_PER_SECOND);
        ROM_SysTickIntEnable();
        ROM_SysTickEnable();


    UARTprintf("\033[2J \033[1;1H Inicializando BMI160... ");
    cod_err=Test_I2C_dir(2, BMI160_I2C_ADDR2);
    if(cod_err)
    {
        UARTprintf("Error 0X%x en BMI160\n", cod_err);
        Bmi_OK=0;
    }
    else
    {
        UARTprintf("Inicializando BMI160, modo NAVIGATION... ");
        bmi160_initialize_sensor();
        bmi160_config_running_mode(APPLICATION_NAVIGATION);
        UARTprintf("Hecho! \nLeyendo DevID... ");
        readI2C(BMI160_I2C_ADDR2,BMI160_USER_CHIP_ID_ADDR, &DevID, 1);
        UARTprintf("DevID= 0X%x \n", DevID);
        Bmi_OK=1;
    }


    while(1)
    {
        if(g_ui32SysTickCount-g_ui32PrevSysTickCount > 10){ // Si ha pasado mas de 10 ms actualizamos
            g_ui32PrevSysTickCount = g_ui32SysTickCount;
            if(Bmi_OK)
            {
                bmi160_read_gyro_xyz(&s_gyroXYZ);

                UARTprintf("\033[4;1H\n");
                sprintf(string, "  GYRO: X:%6d\033[21;22HY:%6d\033[21;35HZ:%6d  \n",s_gyroXYZ.x,s_gyroXYZ.y,s_gyroXYZ.z);
                sprintf(string, "  GYRO: X:%6d\t Y:%6d\t Z:%6d  \n",
                        s_gyroXYZ.x-gyro_off_x, s_gyroXYZ.y-gyro_off_y, s_gyroXYZ.z-gyro_off_z);

                UARTprintf(string);
            }
        }
            // Por alguna razon si lo quito no funciona :)
            if(Bme_OK)
            {
               UARTprintf(string);
            }


    }



    return 0;
}


void Timer0IntHandler(void)
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT); //Borra flag
}

uint8_t Test_I2C_dir(uint32_t pos, uint8_t dir)
{
    uint8_t error=100;
    uint32_t I2C_Base=0;
    uint32_t *I2CMRIS;
    if(pos==1) I2C_Base=I2C0_BASE;
    if(pos==2) I2C_Base=I2C2_BASE;
    if(I2C_Base){
    I2CMasterSlaveAddrSet(I2C_Base, dir, 1);  //Modo LECTURA
    I2CMasterControl(I2C_Base, I2C_MASTER_CMD_SINGLE_RECEIVE); //Lect. Simple

    while(!(I2CMasterBusy(I2C_Base)));  //Espera que empiece
        while((I2CMasterBusy(I2C_Base)));   //Espera que acabe
        I2CMRIS= (uint32_t *)(I2C_Base+0x014);
        error=(uint8_t)((*I2CMRIS) & 0x10);
        if(error){
            I2CMRIS=(uint32_t *)(I2C_Base+0x01C);
            *I2CMRIS=0x00000010;
        }
    }
       return error;
}
