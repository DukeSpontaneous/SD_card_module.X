#include "slave_modul_sd.h"

void SLAVE_SD_MasterQueryProcessing(uint8_t *in_buf, uint8_t *out_buf) {
    SHUTTLE_SYSTEM_CMD cmd = in_buf[1];

    int x;
    switch (cmd) // Обработка полученных данных
    {
        case SHUTTLE_SYS_CMD_FLASH_INITIAL:

            break;
        case SHUTTLE_SYS_CMD_FLASH_WRITE:

            x = 0;
            break;
        default:

            break;
    }
}
