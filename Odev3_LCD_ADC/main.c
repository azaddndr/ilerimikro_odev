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
 ///////////////////////////////////////////////////////////////////////////////
        /*    // 1. ADC Çevrimini Baþlat (Tetikle)
        ADCProcessorTrigger(ADC0_BASE, 1);

        // 2. Çevrimin bitmesini bekle (Polling)
        // ADC meþgul olduðu sürece veya bayrak kalkana kadar bekle
        while(!ADCIntStatus(ADC0_BASE, 1, false));

        // 3. Veriyi ADC FIFO'sundan çek ve diziye yaz
        ADCSequenceDataGet(ADC0_BASE, 1, gelenveri);

        // 4. 4 okumanýn ortalamasýný al (Gürültüyü azaltmak için)
        ortdeger = (gelenveri[0] + gelenveri[1] + gelenveri[2] + gelenveri[3]) / 4;

        // 5. Kesme bayraðýný temizle (Bir sonraki tur için hazýr olsun)
        ADCIntClear(ADC0_BASE, 1);

        // (Ýsteðe baðlý) Ýþlemciyi yormamak için minik bir gecikme
        SysCtlDelay(SysCtlClockGet() / 100);


           //     usprintf(lcd_buffer, "%d", ortdeger);
                    usprintf(lcd_buffer, "%4d", ortdeger);

             //   Lcd_Goto(2,4);
             //   Lcd_Puts("    ");
                Lcd_Goto(2,13);
                Lcd_Puts(lcd_buffer);
   ///////////////////////////////////////////////////////////////////////////////  */


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

    ///////////////////////////////////////////////////////////////////////////////
    Lcd_Goto(1,13);
    Lcd_Puts("ADC:");
    ///////////////////////////////////////////////////////////////////////////////



    // Timer0 periyodik mod
    TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() - 1); // 1 saniye periyot

    // Interrupt ayarlarý
    TimerIntRegister(TIMER0_BASE, TIMER_A, timerkesme);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();

    TimerEnable(TIMER0_BASE, TIMER_A);

    //////////////////////////////////////////////////////////////////////////////////////

      // 2. ADC Modülünü Aktif Et
      SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

      // --- DEÐÝÞÝKLÝK 1: GPIO AYARLARI ---
      // Potansiyometreyi baðlayacaðýmýz Port E'yi aktif et (PE3 kullanacaðýz)
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

      // PE3 pinini Analog Giriþ (ADC) olarak ayarla
      // Tiva C'de PE3 pini, AIN0 (Analog Input 0) kanalýna baðlýdýr.
      GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);


      // 3. ADC Sequence (Sýralayýcý) Ayarý
      // ADC0, Sample Sequencer 1 (SS1), Ýþlemci Tetiklemeli, Öncelik 0
      ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);


      // --- DEÐÝÞÝKLÝK 2: ADIM (STEP) AYARLARI ---
      // Eski kodda ADC_CTL_TS (Sýcaklýk Sensörü) vardý.
      // Bunu ADC_CTL_CH0 (Kanal 0 -> PE3) ile deðiþtirdik.

      // Adým 0: PE3'ü oku
      ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_CH0);
      // Adým 1: PE3'ü tekrar oku
      ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_CH0);
      // Adým 2: PE3'ü tekrar oku
      ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_CH0);

      // Adým 3: PE3'ü son kez oku, Kesme üret (IE) ve Sýralamayý bitir (END)
      ADCSequenceStepConfigure(ADC0_BASE, 1, 3, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);


      // 4. ADC Sýralayýcýyý (Sequencer) Baþlat
      ADCSequenceEnable(ADC0_BASE, 1);
      //////////////////////////////////////////////////////////////////////////////////////


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
        yazadc();

}

void lcdyaz_saat(void)
{
    char saatStr[9];
    sprintf(saatStr, "%02d:%02d:%02d", sa, dk, sn);
    Lcd_Goto(2,1);
    Lcd_Puts(saatStr);
}

void yazadc(void){
    ///////////////////////////////////////////////////////////////////////////////
            // 1. ADC Çevrimini Baþlat (Tetikle)
            ADCProcessorTrigger(ADC0_BASE, 1);

            // 2. Çevrimin bitmesini bekle (Polling)
            // ADC meþgul olduðu sürece veya bayrak kalkana kadar bekle
            while(!ADCIntStatus(ADC0_BASE, 1, false));

            // 3. Veriyi ADC FIFO'sundan çek ve diziye yaz
            ADCSequenceDataGet(ADC0_BASE, 1, gelenveri);

            // 4. 4 okumanýn ortalamasýný al (Gürültüyü azaltmak için)
            ortdeger = (gelenveri[0] + gelenveri[1] + gelenveri[2] + gelenveri[3]) / 4;

            // 5. Kesme bayraðýný temizle (Bir sonraki tur için hazýr olsun)
            ADCIntClear(ADC0_BASE, 1);

            // (Ýsteðe baðlý) Ýþlemciyi yormamak için minik bir gecikme
            SysCtlDelay(SysCtlClockGet() / 100);


               //     usprintf(lcd_buffer, "%d", ortdeger);
                        usprintf(lcd_buffer, "%4d", ortdeger);

                 //   Lcd_Goto(2,4);
                 //   Lcd_Puts("    ");
                    Lcd_Goto(2,13);
                    Lcd_Puts(lcd_buffer);
       ///////////////////////////////////////////////////////////////////////////////
}