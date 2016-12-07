/********************************************************************/
/*                 Definitions to enable SPI channels               */
/********************************************************************/
#define DRV_SPI_CONFIG_CHANNEL_1_ENABLE
//#define DRV_SPI_CONFIG_CHANNEL_2_ENABLE

// Disable SPI FIFO mode in parts where it is not supported
// Enabling the FIFO mode will improve library performance.
// In this demo this definition is sometimes disabled because early versions of the PIC24FJ128GA010s have an errata preventing this feature from being used.
#if defined (__XC8__) || defined (__PIC24HJ128GP502__)
#define DRV_SPI_CONFIG_ENHANCED_BUFFER_DISABLE
#endif