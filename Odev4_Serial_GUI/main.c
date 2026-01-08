#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>             // String işlemleri için
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"        // GPIO kilit işlemleri için
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "Lcd.h"

// Global Değişkenler
volatile int sa = 0, dk = 0, sn = 0;
volatile uint32_t adc_degeri = 0;   // Ham ADC verisi (0-4095)
volatile bool buton_basildi = false; // PF4 Durumu
volatile bool ekran_guncelle = false;

// PC'den gelen 3 karakterlik metin
char lcd_metin[4] = "---";

// UART Tamponları
char gelen_veri[20];
int index_uart = 0;
char gonderilecek_veri[50]; // PC'ye gidecek paket için

//Donanım Başlatma Fonksiyonlari

void ADC_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END); //adc0 secildi-temperature sensor kullanildi-interrupt enable
    ADCSequenceEnable(ADC0_BASE, 3);
}

void Buton_Init(void) {
    // Port F'yi aktif et -SW1 icin-
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // PF4 Tiva üzerindeki SW1 butonudur.
    // Pull-Up direncini aktif etmeliyiz (Basılınca 0 olur)
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

void UART_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, 50000000, 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}

// UART'tan String Göndermeye Yardımcı Fonksiyon
void UART_Gonder_String(char* str) {
    while(*str) {
        UARTCharPut(UART0_BASE, *str++);
    }
}

//Veri Okuma ve İşleme

void Verileri_Oku(void) {
    // ADC verisi okuma
    uint32_t adc_buffer[1];
    ADCProcessorTrigger(ADC0_BASE, 3);

    while(!ADCIntStatus(ADC0_BASE, 3, false)) {

    }

    ADCIntClear(ADC0_BASE, 3);
    ADCSequenceDataGet(ADC0_BASE, 3, adc_buffer);
    adc_degeri = adc_buffer[0];

    // 2. Buton Okuma (PF4)
    // Basılı ise 0 (Low), değilse 1 (High) okunur.
    // Biz "Basıldı mı?" diye baktığımız için tersini alalım.
    if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0) {
        buton_basildi = true; // Basılı
    } else {
        buton_basildi = false; // Basılı Değil
    }
}

//Timer Kesmesi -Saniyede 1 Kez-
// Görev: Saati ilerlet + PC'ye Rapor Ver + Ekranı Güncelle
void Timer0IntHandler(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // Saati İlerlet
    sn++;
    if (sn >= 60) { sn = 0; dk++; if (dk >= 60) { dk = 0; sa++; if (sa >= 24) sa = 0; } }

    // ADC sensor verilerini okuma
    Verileri_Oku();

    // --- PC'ye Veri Gönder (Protokol: D:HH:MM:SS,ADC,BTN) ---
    // Örnek: D:12:34:56
               //4095
              //1
    // \r\n ekliyoruz ki SharpDevelop satırın bittiğini anlasın.
    sprintf(gonderilecek_veri, "D:%02d:%02d:%02d,%04d,%d\r\n",
            sa, dk, sn, (int)adc_degeri, buton_basildi ? 1 : 0);

    UART_Gonder_String(gonderilecek_veri);

    ekran_guncelle = true;
}

void Timer_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 50000000 - 1);
    TimerIntRegister(TIMER0_BASE, TIMER_A, Timer0IntHandler);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    TimerEnable(TIMER0_BASE, TIMER_A);
}

// Gelen Veriyi Ayrıştırma -Parsing-
void Komut_Isle(char* veri) {
    // Protokol 1: Saat Ayarı -> "S12:34:56"
    if (veri[0] == 'S') {
        int y_sa = (veri[1] - '0') * 10 + (veri[2] - '0');
        int y_dk = (veri[4] - '0') * 10 + (veri[5] - '0');
        int y_sn = (veri[7] - '0') * 10 + (veri[8] - '0');

        if (y_sa < 24 && y_dk < 60 && y_sn < 60) {
            sa = y_sa; dk = y_dk; sn = y_sn;
            ekran_guncelle = true;
        }
    }
    // Protokol 2: Metin Ayarı -> "TABC"
    else if (veri[0] == 'T') {
        // En fazla 3 karakter alacağız
        lcd_metin[0] = veri[1];
        lcd_metin[1] = veri[2];
        lcd_metin[2] = veri[3];
        lcd_metin[3] = '\0'; // String sonlandırıcı
        ekran_guncelle = true;
    }
}

int main(void) {
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    Lcd_init();
    UART_Init();
    ADC_Init();
    Buton_Init();
    Timer_Init(); // Sistem başlar

    Lcd_Goto(1,1);
    Lcd_Puts("Tiva Kontrol PC");

    char karakter;

    while (1) {
        // --- UART Veri Alma ---
        if (UARTCharsAvail(UART0_BASE)) {
            karakter = UARTCharGet(UART0_BASE);

            // Satır sonu karakteri geldi mi? (\n veya \r)
            if (karakter == '\r' || karakter == '\n') {
                gelen_veri[index_uart] = '\0';
                if(index_uart > 0) {
                    Komut_Isle(gelen_veri);
                }
                index_uart = 0; // Tamponu sıfırla
            }
            else {
                if (index_uart < 18) {
                    gelen_veri[index_uart] = karakter;
                    index_uart++;
                }
            }
        }

        // --- LCD Güncelleme ---
        if (ekran_guncelle) {
            ekran_guncelle = false;

            // 1. Satırın sonuna 3 karakterlik metni yaz (Sağa yaslı)
            Lcd_Goto(1, 13);
            Lcd_Puts(lcd_metin);

            // 2. Satıra Saati Yaz
            char tampon[17];
            sprintf(tampon, "%02d:%02d:%02d   ", sa, dk, sn);
            Lcd_Goto(2, 1);
            Lcd_Puts(tampon);
        }
    }
}
