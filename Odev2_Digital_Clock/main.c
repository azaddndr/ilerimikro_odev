#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"

#include "utils/ustdlib.h"

#include "lcd.h"

int sn, dk, sa;

void initmikro(void);
void timerkesme(void);
void lcdyaz_saat(void);

///////////////////////////////////////////////////////////////////////////////
uint32_t gelenveri[4]; // ADC'den gelen 4 örneði tutacak dizi
    uint32_t ortdeger;     // Hesaplanan ortalama deðer

    char lcd_buffer[20];


//////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
    initmikro();



    while (1)
    {



    }


}

void initmikro()
{
    sn = 0; dk = 0; sa = 0;

    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN); // 40 MHz
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);

    Lcd_init();
    Lcd_Goto(1,1);
    Lcd_Puts("Saat");
    Lcd_Goto(2,1);
    Lcd_Puts("00:00:00");

    // Timer0 periyodik mod
    TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() - 1); // 1 saniye periyot

    // Interrupt ayarlarý
    TimerIntRegister(TIMER0_BASE, TIMER_A, timerkesme);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();

    TimerEnable(TIMER0_BASE, TIMER_A);


}

void timerkesme(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT); // Kesme bayraðýný temizle

    sn++;
    if (sn == 60)
    {
        sn = 0;
        dk++;
        if (dk == 60)
        {
            dk = 0;
            sa++;
            if (sa == 24)
                sa = 0;
        }
    }

    // LED'i toggle yap (debug)
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1,
        ~GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1));
        lcdyaz_saat();  // LCD'de sürekli zamaný güncelle
        

}

void lcdyaz_saat(void)
{
    char saatStr[9];
    sprintf(saatStr, "%02d:%02d:%02d", sa, dk, sn);
    Lcd_Goto(2,1);
    Lcd_Puts(saatStr);
}

