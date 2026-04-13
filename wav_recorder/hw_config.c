#include "hw_config.h"

// Configurare SPI
static spi_t spis[] = {
    {
        .hw_inst  = spi0,
        .miso_gpio = 16,
        .mosi_gpio = 19,
        .sck_gpio  = 18,
        .baud_rate = 12500000  
    }
};


static sd_spi_if_t spi_ifs[] = {
    {
        .spi     = &spis[0],
        .ss_gpio = 17         
    }
};

// SD Card
static sd_card_t sd_cards[] = {
    {
        .type     = SD_IF_SPI,
        .spi_if_p = &spi_ifs[0]
    }
};


size_t sd_get_num() {
    return count_of(sd_cards);
}

sd_card_t *sd_get_by_num(size_t num) {
    if (num < sd_get_num()) return &sd_cards[num];
    return NULL;
}