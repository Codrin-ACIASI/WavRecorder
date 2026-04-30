#include "audio_player.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "i2s.pio.h"
#include "ff.h"
#include <stdio.h>

#define I2S_DATA_PIN 13
#define I2S_BCLK_PIN 14 // LRCLK va fi automat 15

#define AUDIO_BUF_SIZE 2048 // Dimensiunea unui buffer
int16_t audio_buf[2][AUDIO_BUF_SIZE];

static int dma_chan_a, dma_chan_b;
static FIL current_file;
static bool is_playing = false;
static bool is_paused = false;
static int current_buffer = 0;

// Configurare PIO
static PIO i2s_pio = pio0;
static uint sm = 0;

static void dma_handler() {
    // Ștergem flag-ul de întrerupere pentru canalul DMA care tocmai a terminat
    if (dma_hw->ints0 & (1u << dma_chan_a)) {
        dma_hw->ints0 = 1u << dma_chan_a;
        current_buffer = 0;
    } else if (dma_hw->ints0 & (1u << dma_chan_b)) {
        dma_hw->ints0 = 1u << dma_chan_b;
        current_buffer = 1;
    }

    if (!is_playing || is_paused) return;

    // Citim următoarea bucată din fișier în buffer-ul care tocmai s-a eliberat
    UINT bytes_read;
    f_read(&current_file, audio_buf[current_buffer], AUDIO_BUF_SIZE * sizeof(int16_t), &bytes_read);

    if (bytes_read == 0) {
        audio_player_stop(); // Am ajuns la finalul fișierului
    }
}

void audio_player_init(void) {
    uint offset = pio_add_program(i2s_pio, &i2s_program);
    
    // Configuram pinii
    pio_gpio_init(i2s_pio, I2S_DATA_PIN);
    pio_gpio_init(i2s_pio, I2S_BCLK_PIN);
    pio_gpio_init(i2s_pio, I2S_BCLK_PIN + 1); // LRCLK

    pio_sm_config c = i2s_program_get_default_config(offset);
    sm_config_set_out_pins(&c, I2S_DATA_PIN, 1);
    sm_config_set_sideset_pins(&c, I2S_BCLK_PIN);
    sm_config_set_out_shift(&c, false, true, 32); // Shift out pe stânga, auto pull
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    // Frecvența de bază (Ex: pt 44.1kHz * 32 biți pe frame * 8 cicluri per bit)
    // Vom ajusta frecvența dinamic la apelul de play.
    pio_sm_init(i2s_pio, sm, offset, &c);

    // Initializare DMA
    dma_chan_a = dma_claim_unused_channel(true);
    dma_chan_b = dma_claim_unused_channel(true);

    dma_channel_config dma_conf_a = dma_channel_get_default_config(dma_chan_a);
    channel_config_set_transfer_data_size(&dma_conf_a, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_conf_a, true);
    channel_config_set_write_increment(&dma_conf_a, false);
    channel_config_set_dreq(&dma_conf_a, pio_get_dreq(i2s_pio, sm, true));
    channel_config_set_chain_to(&dma_conf_a, dma_chan_b);

    dma_channel_config dma_conf_b = dma_channel_get_default_config(dma_chan_b);
    channel_config_set_transfer_data_size(&dma_conf_b, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_conf_b, true);
    channel_config_set_write_increment(&dma_conf_b, false);
    channel_config_set_dreq(&dma_conf_b, pio_get_dreq(i2s_pio, sm, true));
    channel_config_set_chain_to(&dma_conf_b, dma_chan_a);

    dma_channel_configure(dma_chan_a, &dma_conf_a, &i2s_pio->txf[sm], audio_buf[0], AUDIO_BUF_SIZE, false);
    dma_channel_configure(dma_chan_b, &dma_conf_b, &i2s_pio->txf[sm], audio_buf[1], AUDIO_BUF_SIZE, false);

    // Setup intreruperi DMA
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    dma_channel_set_irq0_enabled(dma_chan_a, true);
    dma_channel_set_irq0_enabled(dma_chan_b, true);
    irq_set_enabled(DMA_IRQ_0, true);
}

bool audio_player_play(const char* filename) {
    if (is_playing) audio_player_stop();

    if (f_open(&current_file, filename, FA_READ) != FR_OK) {
        return false;
    }

    // Aici sarim peste header-ul WAV pentru simplitate (primele 44 de bytes de obicei)
    // Vom citi esantionul la 44.1kHz pentru acest exemplu.
    f_lseek(&current_file, 44);

    // Setam frecventa de rulare PIO: 44100 Hz * 32 (biti per frame) * 8 (cicluri clock in PIO)
    float clk_div = (float)clock_get_hz(clk_sys) / (44100.0f * 32.0f * 8.0f);
    pio_sm_set_clkdiv(i2s_pio, sm, clk_div);

    UINT bytes_read;
    f_read(&current_file, audio_buf[0], AUDIO_BUF_SIZE * sizeof(int16_t), &bytes_read);
    f_read(&current_file, audio_buf[1], AUDIO_BUF_SIZE * sizeof(int16_t), &bytes_read);

    is_playing = true;
    is_paused = false;

    pio_sm_set_enabled(i2s_pio, sm, true);
    dma_channel_start(dma_chan_a);

    return true;
}

void audio_player_pause(void) {
    is_paused = true;
    pio_sm_set_enabled(i2s_pio, sm, false);
}

void audio_player_resume(void) {
    if(is_playing && is_paused) {
        is_paused = false;
        pio_sm_set_enabled(i2s_pio, sm, true);
    }
}

void audio_player_stop(void) {
    is_playing = false;
    is_paused = false;
    pio_sm_set_enabled(i2s_pio, sm, false);
    dma_channel_abort(dma_chan_a);
    dma_channel_abort(dma_chan_b);
    f_close(&current_file);
}

bool audio_player_is_playing(void) {
    return is_playing && !is_paused;
}