#include "audio_player.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "i2s_tx.pio.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>

#define I2S_DATA_PIN 13
#define I2S_BCLK_PIN 14
#define I2S_LRCLK_PIN 15
#define I2S_AMP_SD   16

#define AUDIO_FRAMES 2048

uint32_t audio_buffer_0[AUDIO_FRAMES];
uint32_t audio_buffer_1[AUDIO_FRAMES];

static int dma_ch_0;
static int dma_ch_1;
static uint pio_sm;
static PIO pio_inst = pio0;
static uint pio_offset;

volatile bool buffer_0_needs_fill = false;
volatile bool buffer_1_needs_fill = false;
volatile bool is_playing = false;

#define MAX_RAM_WAV_BYTES 120000
static int16_t ram_wav_buffer[MAX_RAM_WAV_BYTES / 2];
static uint32_t ram_wav_samples_total = 0;
static uint32_t ram_wav_current_sample_index = 0;

static uint16_t wav_channels = 1;
static uint32_t wav_sample_rate = 16000;

static FIL current_wav;

/* ================= WAV PARSER ================= */

bool parse_wav_header(FIL *file) {
    uint8_t header[44];
    UINT br;

    f_lseek(file, 0);
    if (f_read(file, header, 44, &br) != FR_OK || br != 44)
        return false;

    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0)
        return false;

    wav_channels = header[22] | (header[23] << 8);
    wav_sample_rate = header[24] | (header[25] << 8) |
                      (header[26] << 16) | (header[27] << 24);

    uint16_t bits = header[34] | (header[35] << 8);
    if (bits != 16) return false;

    uint32_t offset = 12;
    uint32_t data_size = 0;

    while (offset < 2048) {
        f_lseek(file, offset);
        if (f_read(file, header, 8, &br) != FR_OK || br != 8)
            break;

        uint32_t chunk_size = header[4] | (header[5] << 8) |
                              (header[6] << 16) | (header[7] << 24);

        if (memcmp(header, "data", 4) == 0) {
            data_size = chunk_size;
            break;
        }

        offset += 8 + chunk_size;
    }

    if (data_size == 0)
        return false;

    if (data_size > MAX_RAM_WAV_BYTES)
        data_size = MAX_RAM_WAV_BYTES;

    f_lseek(file, offset + 8);

    UINT br2;
    if (f_read(file, ram_wav_buffer, data_size, &br2) != FR_OK)
        return false;

    ram_wav_samples_total = br2 / 2;
    ram_wav_current_sample_index = 0;

    f_close(file);
    return true;
}

/* ================= BUFFER ================= */

void fill_buffer(uint32_t *dest) {
    if (!is_playing) {
        memset(dest, 0, AUDIO_FRAMES * 4);
        return;
    }

    if (ram_wav_current_sample_index >= ram_wav_samples_total) {
        memset(dest, 0, AUDIO_FRAMES * 4);
        audio_stop();
        return;
    }

    for (int i = 0; i < AUDIO_FRAMES; i++) {
        int16_t sample = 0;

        if (ram_wav_current_sample_index < ram_wav_samples_total) {
            sample = ram_wav_buffer[ram_wav_current_sample_index++];
        }

        dest[i] = ((uint32_t)(uint16_t)sample << 16) |
                   (uint16_t)sample;
    }
}

/* ================= DMA IRQ ================= */

void dma_irq_handler() {
    if (dma_hw->ints0 & (1u << dma_ch_0)) {
        dma_hw->ints0 = (1u << dma_ch_0);
        dma_channel_set_read_addr(dma_ch_0, audio_buffer_0, false);
        dma_channel_set_trans_count(dma_ch_0, AUDIO_FRAMES, false);
        buffer_0_needs_fill = true;
    }

    if (dma_hw->ints0 & (1u << dma_ch_1)) {
        dma_hw->ints0 = (1u << dma_ch_1);
        dma_channel_set_read_addr(dma_ch_1, audio_buffer_1, false);
        dma_channel_set_trans_count(dma_ch_1, AUDIO_FRAMES, false);
        buffer_1_needs_fill = true;
    }
}

/* ================= INIT ================= */

void audio_init() {
    gpio_init(I2S_AMP_SD);
    gpio_set_dir(I2S_AMP_SD, GPIO_OUT);
    gpio_put(I2S_AMP_SD, 0);

    pio_offset = pio_add_program(pio_inst, &i2s_tx_program);
    pio_sm = pio_claim_unused_sm(pio_inst, true);

    pio_sm_config c = i2s_tx_program_get_default_config(pio_offset);

    sm_config_set_out_pins(&c, I2S_DATA_PIN, 1);
    sm_config_set_sideset_pins(&c, I2S_BCLK_PIN);
    sm_config_set_out_shift(&c, false, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    pio_gpio_init(pio_inst, I2S_DATA_PIN);
    pio_gpio_init(pio_inst, I2S_BCLK_PIN);
    pio_gpio_init(pio_inst, I2S_LRCLK_PIN);

    pio_sm_set_consecutive_pindirs(pio_inst, pio_sm, I2S_DATA_PIN, 1, true);
    pio_sm_set_consecutive_pindirs(pio_inst, pio_sm, I2S_BCLK_PIN, 3, true);

    pio_sm_init(pio_inst, pio_sm, pio_offset, &c);

    dma_ch_0 = dma_claim_unused_channel(true);
    dma_ch_1 = dma_claim_unused_channel(true);

    dma_channel_config c0 = dma_channel_get_default_config(dma_ch_0);
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_32);
    channel_config_set_read_increment(&c0, true);
    channel_config_set_write_increment(&c0, false);
    channel_config_set_dreq(&c0, pio_get_dreq(pio_inst, pio_sm, true));
    channel_config_set_chain_to(&c0, dma_ch_1);

    dma_channel_configure(dma_ch_0, &c0,
        &pio_inst->txf[pio_sm],
        audio_buffer_0,
        AUDIO_FRAMES,
        false);

    dma_channel_config c1 = dma_channel_get_default_config(dma_ch_1);
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);
    channel_config_set_read_increment(&c1, true);
    channel_config_set_write_increment(&c1, false);
    channel_config_set_dreq(&c1, pio_get_dreq(pio_inst, pio_sm, true));
    channel_config_set_chain_to(&c1, dma_ch_0);

    dma_channel_configure(dma_ch_1, &c1,
        &pio_inst->txf[pio_sm],
        audio_buffer_1,
        AUDIO_FRAMES,
        false);

    dma_channel_set_irq0_enabled(dma_ch_0, true);
    dma_channel_set_irq0_enabled(dma_ch_1, true);

    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

/* ================= PLAY / STOP ================= */

bool audio_play_wav(const char *filename) {
    if (is_playing) audio_stop();

    if (f_open(&current_wav, filename, FA_READ) != FR_OK)
        return false;

    if (!parse_wav_header(&current_wav))
        return false;

    uint32_t sys_clk = clock_get_hz(clk_sys);
    uint32_t bclk = wav_sample_rate * 2 * 16;
    float div = (float)sys_clk / (bclk * 2.0f);
    pio_sm_set_clkdiv(pio_inst, pio_sm, div);

    gpio_put(I2S_AMP_SD, 1);
    is_playing = true;

    fill_buffer(audio_buffer_0);
    fill_buffer(audio_buffer_1);

    dma_channel_set_read_addr(dma_ch_0, audio_buffer_0, false);
    dma_channel_set_trans_count(dma_ch_0, AUDIO_FRAMES, false);
    dma_channel_set_read_addr(dma_ch_1, audio_buffer_1, false);
    dma_channel_set_trans_count(dma_ch_1, AUDIO_FRAMES, false);

    pio_sm_set_enabled(pio_inst, pio_sm, true);
    dma_channel_start(dma_ch_0);

    return true;
}

void audio_stop() {
    is_playing = false;
    ram_wav_current_sample_index = 0;

    dma_channel_abort(dma_ch_0);
    dma_channel_abort(dma_ch_1);

    pio_sm_set_enabled(pio_inst, pio_sm, false);
    pio_sm_clear_fifos(pio_inst, pio_sm);

    gpio_put(I2S_AMP_SD, 0);

    f_close(&current_wav);
}

void audio_pause(bool pause) {
    if (!is_playing) return;

    if (pause) {
        pio_sm_set_enabled(pio_inst, pio_sm, false);
        gpio_put(I2S_AMP_SD, 0);
    } else {
        gpio_put(I2S_AMP_SD, 1);
        pio_sm_set_enabled(pio_inst, pio_sm, true);
    }
}

void audio_task() {
    if (buffer_0_needs_fill) {
        fill_buffer(audio_buffer_0);
        buffer_0_needs_fill = false;
    }

    if (buffer_1_needs_fill) {
        fill_buffer(audio_buffer_1);
        buffer_1_needs_fill = false;
    }
}

bool audio_is_playing() {
    return is_playing;
}