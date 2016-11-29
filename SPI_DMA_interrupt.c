#include <xc.h>
#include <stdint.h>

#include "SPI_DMA_interrupt.h"

uint8_t Spi1BuffReceive[512] __attribute__((space(dma)));
uint8_t Spi1BuffSend_1[512] __attribute__((space(dma)));
uint8_t Spi1BuffSend_2[512] __attribute__((space(dma)));

void cfgDma0SpiSend(void) //настройка DMA0 на передачу SPI1
{
    DMA0CON = 0b0110000000000001;
    DMA0CNT = 511;
    DMA0REQ = 0x00A;
    IPC3bits.DMA1IP = 6;
    IPC1bits.DMA0IP = 6;
    DMA0PAD = (volatile uint16_t) &SPI1BUF;
    DMA0STA = __builtin_dmaoffset(Spi1BuffSend_1);
    DMA0STB = __builtin_dmaoffset(Spi1BuffSend_1);
}
//--------------------------------------------------------------------------------

void cfgDma1SpiReceive(void) //настройка DMA1 на прием SPI1
{
    DMA1CON = 0b0100000000000001;
    DMA1CNT = 511;
    DMA1REQ = 0x00A;

    DMA1PAD = (volatile uint16_t) &SPI1BUF;
    DMA1STA = __builtin_dmaoffset(Spi1BuffReceive);
    DMA1STB = __builtin_dmaoffset(Spi1BuffReceive);
}
//------------------------------------------------------------------------------

//void __attribute__((interrupt, auto_psv)) _DMA0Interrupt(void)
//{
//    IFS0bits.DMA0IF = 0; // Clear the DMA0 Interrupt Flag;
//}
////------------------------------------------------------------------------------
//void __attribute__((interrupt, auto_psv)) _DMA1Interrupt(void)
//{
//    DMA1CONbits.CHEN = DMA0CONbits.CHEN = 0; // Disable DMA Channel
//    IEC0bits.DMA0IE = IEC0bits.DMA1IE = 0; // Disable DMA interrupt
//    SD_CS_DEASSERT;
//    if (zapis_fleshki.write_command == 1)
//    {
//        zapis_fleshki.write_command = 0;
//    }
//    else
//    {
//        zapis_fleshki.read_command = 0;
//    }
//    IFS0bits.DMA1IF = 0; // Clear the DMA1 Interrupt Flag
//}
