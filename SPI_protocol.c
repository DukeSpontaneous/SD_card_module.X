#include "SPI_protocol.h"

#include "pins.h"
#include "SPI_DMA_interrupt.h"

void SPI_initial(void) //инициализация SPI
{
    cfgDma0SpiSend();
    cfgDma1SpiReceive();
    
    RPINR20bits.SDI1R = 3; //RB3
    RPOR1bits.RP2R = 8; //sck1
    RPOR0bits.RP1R = 7; //sdo1
    SPI1CON1bits.DISSCK = 0;
    SPI1CON1bits.DISSDO = 0;
    SPI1CON1bits.MODE16 = 0;
    SPI1CON1bits.SMP = 0;
    SPI1CON1bits.CKE = 0;
    // SPI1CON1bits.SSEN = 0;
    SPI1CON1bits.CKP = 1;
    SPI1CON1bits.MSTEN = 1;
    SPI1CON1bits.SPRE = 0b101; //13MHZ 101
    SPI1CON1bits.PPRE = 0b11; //11

    SPI1CON2bits.FRMEN = 0;
    SPI1CON2bits.SPIFSD = 0;
    SPI1CON2bits.FRMPOL = 0;
    SPI1CON2bits.FRMDLY = 0;

    SPI1STATbits.SPIEN = 1;
    SPI1STATbits.SPISIDL = 0;
    SPI1STATbits.SPIROV = 0;


    SPI1STATbits.SPIEN = 1;
    SD_CS_DEASSERT;
}
//------------------------------------------------------------------------------

uint8_t SPI1_rw(const uint8_t data)//запись/чтение байта SPI1
{
    IFS0bits.SPI1IF = 0;
    SPI1STATbits.SPIROV = 0;
    SPI1BUF = data;
    while (IFS0bits.SPI1IF == 0)
    {
    }
    IFS0bits.SPI1IF = 0;
    SPI1STATbits.SPIROV = 0;
    return SPI1BUF;
}
//------------------------------------------------------------------------------

void flash_read_block(uint32_t address_read, uint16_t bytes_read, uint8_t buffer)
{
//    //читаем
//    DMACS1bits.PPST0 = DMACS1bits.PPST1 = buffer;
//
//    if (zapis_fleshki.buff_1_2 == 1)// если заполняется буффер 2, использовать буффер 1 для чтения
//    {
//        Spi1TxBuff_1[0] = 0x03;
//        Spi1TxBuff_1[1] = adress_read >> 16;
//        Spi1TxBuff_1[2] = adress_read >> 8;
//        Spi1TxBuff_1[3] = adress_read;
//        //        zapis_fleshki.write_buffer_not_empty_1 = 1;
//        DMA0STA = __builtin_dmaoffset(Spi1TxBuff_1);
//        DMA0STB = __builtin_dmaoffset(Spi1TxBuff_1);
//    }
//    else
//    {
//        Spi1TxBuff_2[0] = 0x03;
//        Spi1TxBuff_2[1] = adress_read >> 16;
//        Spi1TxBuff_2[2] = adress_read >> 8;
//        Spi1TxBuff_2[3] = adress_read;
//
//        DMA0STA = __builtin_dmaoffset(Spi1TxBuff_2);
//        DMA0STB = __builtin_dmaoffset(Spi1TxBuff_2);
//    }
//    if (buffer == 0)
//    {
//        DMA1STA = __builtin_dmaoffset(Spi1RxBuff);
//        DMA1STB = __builtin_dmaoffset(Spi1RxBuff);
//    }
//    else
//    {
//        DMA1STA = __builtin_dmaoffset(Spi1_gurnal_rx_Buff);
//        DMA1STB = __builtin_dmaoffset(Spi1_gurnal_rx_Buff);
//    }
//
//    bytes_read += 3;
//    DMA1CNT = DMA0CNT = bytes_read;
//    IFS0bits.DMA0IF = IFS0bits.DMA1IF = 0; // Clear DMA interrupt
//    IEC0bits.DMA0IE = IEC0bits.DMA1IE = 1; // Enable DMA interrupt
//    DMA1CONbits.CHEN = DMA0CONbits.CHEN = 1; // Enable DMA Channel
//    CS_flash = 0;
//    DMA0REQbits.FORCE = 1;
//    zapis_fleshki.read_command = 1;
    //читаем
}
//flash read block
//------------------------------------------------------------------------------
//запись блока данных во внутренний флеш

void flash_write_block(uint32_t address_write, uint16_t bytes_write)
{
//    DMACS1bits.PPST0 = DMACS1bits.PPST1 = 0;
//    if (adress_write + bytes_write >= zapis_fleshki.adres_4096)
//    {
//        if (adress_write + bytes_write >= zapis_fleshki.max_adres)
//        {
//            zapis_fleshki.adres_4096 = 0;
//            adress_write = 0;
//            zapis_fleshki.adres = bytes_write;
//        }
//
//        flash_erase(zapis_fleshki.adres_4096);
//        zapis_fleshki.adres_4096 += 4096;
//        __delay_ms(50);
//        flash_erase(zapis_fleshki.adres_4096);
//        __delay_ms(50);
//    }
//    if (zapis_fleshki.buff_1_2 == 0)
//    {
//        Spi1TxBuff_1[0] = 0x02;
//        Spi1TxBuff_1[1] = adress_write >> 16;
//        Spi1TxBuff_1[2] = adress_write >> 8;
//        Spi1TxBuff_1[3] = adress_write;
//        DMA0STA = __builtin_dmaoffset(Spi1TxBuff_1);
//        DMA0STB = __builtin_dmaoffset(Spi1TxBuff_1);
//    }
//    else
//    {
//        Spi1TxBuff_2[0] = 0x02;
//        Spi1TxBuff_2[1] = adress_write >> 16;
//        Spi1TxBuff_2[2] = adress_write >> 8;
//        Spi1TxBuff_2[3] = adress_write;
//        DMA0STA = __builtin_dmaoffset(Spi1TxBuff_2);
//        DMA0STB = __builtin_dmaoffset(Spi1TxBuff_2);
//    }
//    DMA1STA = __builtin_dmaoffset(Spi1RxBuff);
//    DMA1STB = __builtin_dmaoffset(Spi1RxBuff);
//    bytes_write += 3;
//    DMA1CNT = DMA0CNT = bytes_write;
//    IFS0bits.DMA0IF = IFS0bits.DMA1IF = 0; // Clear DMA interrupt
//    IEC0bits.DMA0IE = IEC0bits.DMA1IE = 1; // Enable DMA interrupt
//    DMA1CONbits.CHEN = DMA0CONbits.CHEN = 1; // Enable DMA Channel
//    CS_flash = 0;
//    DMA0REQbits.FORCE = 1;
//    zapis_fleshki.write_command = 1;
}