#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "Lcd.h"



void Lcd_Gonder_Nibble(unsigned char data, unsigned char control) {
    // Önce veri yollarını temizle (D4-D7) ve yeni veriyi yaz
    // control değişkeni RS pinini belirler (0: Komut, 1: Veri)

    // Veriyi yaz (Maskeleme yaparak sadece ilgili bitleri değiştiriyoruz)
    GPIOPinWrite(LCDPORT, D4 | D5 | D6 | D7, (data & 0xF0));
    GPIOPinWrite(LCDPORT, RS, control ? RS : 0x00);

    // Enable Pulse (Tetikleme)
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(800); // En az 450ns bekle (garanti olsun diye artırdık)
    GPIOPinWrite(LCDPORT, E, 0x00);
    SysCtlDelay(800); // Komutun işlenmesi için bekle
}

void Lcd_Komut(unsigned char c) {
    // Önce Yüksek 4 bit
    Lcd_Gonder_Nibble(c, 0);
    // Sonra Düşük 4 bit (Veriyi sola kaydırarak yüksek konuma getiriyoruz)
    Lcd_Gonder_Nibble(c << 4, 0);

    // Komutların çoğu 40us sürer, ama 'Temizle' ve 'Başa Dön' 2ms sürer.
    if(c < 4)
        SysCtlDelay(50000); // ~2ms bekle
    else
        SysCtlDelay(2000);  // ~50us bekle
}

void Lcd_Putch(unsigned char d) {
    // Önce Yüksek 4 bit (RS=1 çünkü Veri gönderiyoruz)
    Lcd_Gonder_Nibble(d, 1);
    // Sonra Düşük 4 bit
    Lcd_Gonder_Nibble(d << 4, 1);

    SysCtlDelay(2000); // ~50us bekle
}

void Lcd_init() {
    // Port Ayarları
    SysCtlPeripheralEnable(LCDPORTENABLE);
    // Portun hazır olmasını bekle
    while(!SysCtlPeripheralReady(LCDPORTENABLE));

    GPIOPinTypeGPIOOutput(LCDPORT, 0xFF);

    // LCD'nin elektriksel olarak kendine gelmesi için bekle (>15ms)
    SysCtlDelay(500000);

    // --- 4-Bit Moda Geçiş Ritüeli (Datasheet'e göre) ---

    // Adım 1: 0x03 gönder
    GPIOPinWrite(LCDPORT, RS, 0x00);
    GPIOPinWrite(LCDPORT, D4 | D5 | D6 | D7, 0x30);
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(800);
    GPIOPinWrite(LCDPORT, E, 0x00);
    SysCtlDelay(150000); // >4.1ms bekle

    // Adım 2: Tekrar 0x03 gönder
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(800);
    GPIOPinWrite(LCDPORT, E, 0x00);
    SysCtlDelay(5000); // >100us bekle

    // Adım 3: Tekrar 0x03 gönder
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(800);
    GPIOPinWrite(LCDPORT, E, 0x00);
    SysCtlDelay(5000);

    // Adım 4: 0x02 gönder (4-bit moda geçiş komutu)
    GPIOPinWrite(LCDPORT, D4 | D5 | D6 | D7, 0x20);
    GPIOPinWrite(LCDPORT, E, E);
    SysCtlDelay(800);
    GPIOPinWrite(LCDPORT, E, 0x00);
    SysCtlDelay(5000);

    // --- Artık 4-bit moddayız, normal komut fonksiyonunu kullanabiliriz ---

    Lcd_Komut(0x28); // Fonksiyon Seti: 4-bit, 2 satır, 5x7 font
    Lcd_Komut(0x0C); // Ekran Aç, İmleç Kapalı, Yanıp Sönme Kapalı
    Lcd_Komut(0x06); // Giriş Modu: Yazdıkça imleci sağa kaydır
    Lcd_Temizle();   // Ekranı temizle
}

void Lcd_Goto(char x, char y){
    // x: Satır (1 veya 2), y: Sütun (1-16)
    if(x==1)
        Lcd_Komut(0x80 + ((y-1) % 16));
    else
        Lcd_Komut(0xC0 + ((y-1) % 16));
}

void Lcd_Temizle(void){
    Lcd_Komut(0x01);
    SysCtlDelay(50000); // Temizleme uzun sürer
}

void Lcd_Puts(char* s){
    while(*s)
        Lcd_Putch(*s++);
}
