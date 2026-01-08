#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "Lcd.h"

int main(void) {

        SysCtlClockSet(
                        SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ
                        | SYSCTL_OSC_MAIN);

        Lcd_init();
        Lcd_Goto(1,1);
        Lcd_Puts("Evren Sampiyon");
        Lcd_Goto(2,4);
        Lcd_Puts("SAU");

        while (1) {

        }
}
