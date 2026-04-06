#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "lvgl.h" // Includem biblioteca gigant!
#
// ---- Pini display ----
#define PIN_MISO  16
#define PIN_CLK   18
#define PIN_MOSI  19
#define PIN_RST   20
#define PIN_DC    21

#define LCD_W 240
#define LCD_H 320

// Funcții ajutătoare SPI
static inline void dc_cmd()  { gpio_put(PIN_DC, 0); }
static inline void dc_data() { gpio_put(PIN_DC, 1); }
static void spi_write_byte(uint8_t b) { spi_write_blocking(spi0, &b, 1); }
static void spi_write_word(uint16_t w) {
    uint8_t buf[2] = { w >> 8, w & 0xFF };
    spi_write_blocking(spi0, buf, 2);
}

// Funcții LCD
static void lcd_cmd(uint8_t cmd)   { dc_cmd();  spi_write_byte(cmd); }
static void lcd_data8(uint8_t d)   { dc_data(); spi_write_byte(d);   }
static void lcd_data16(uint16_t d) { dc_data(); spi_write_word(d);   }

static void lcd_init_ili9341() {
    gpio_put(PIN_RST, 1); sleep_ms(10);
    gpio_put(PIN_RST, 0); sleep_ms(50);
    gpio_put(PIN_RST, 1); sleep_ms(120);

    lcd_cmd(0x01); sleep_ms(5); // Reset
    lcd_cmd(0x28); // Display OFF
    lcd_cmd(0x3A); lcd_data8(0x55); // Pixel format RGB565
    lcd_cmd(0x36); lcd_data8(0x48); // Orientare Portret
    lcd_cmd(0x11); sleep_ms(120);   // Sleep out
    lcd_cmd(0x29); // Display ON
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    lcd_cmd(0x2A); lcd_data16(x0); lcd_data16(x1);
    lcd_cmd(0x2B); lcd_data16(y0); lcd_data16(y1);
    lcd_cmd(0x2C);
}

// ============================================================
// MAGIA LVGL: Funcția care desenează
// ============================================================
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    // 1. Spunem ecranului în ce zonă vrem să desenăm
    lcd_set_window(area->x1, area->y1, area->x2, area->y2);
    
    // 2. Calculăm câți pixeli ne-a trimis LVGL
    uint32_t size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
    
    // 3. Trimitem toți pixelii dintr-o lovitură prin SPI
    dc_data();
    spi_write_blocking(spi0, (const uint8_t *)color_p, size * 2); // *2 pentru că un pixel are 2 bytes

    // 4. Îi spunem lui LVGL: "Sefu', am terminat de afișat bucata asta!"
    lv_disp_flush_ready(disp_drv);
}

// O funcție standard pe care LVGL o cere ca să știe cât timp a trecut
uint32_t custom_tick_get(void) {
    return to_ms_since_boot(get_absolute_time());
}


int main() {
    stdio_init_all();

    // 1. Setup SPI și Pini (10 MHz)
    spi_init(spi0, 10 * 1000 * 1000); 
    gpio_set_function(PIN_CLK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_init(PIN_DC);  gpio_set_dir(PIN_DC,  GPIO_OUT);
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT);

    // 2. Pornim fizic ecranul
    lcd_init_ili9341();

    // 3. Inițializăm biblioteca LVGL
    lv_init();

    // 4. Setăm un "buffer" (Memorie temporară unde LVGL desenează formele înainte să le afișeze)
    // Vom face un buffer mare de 40 de rânduri pentru o animație super fluidă
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[LCD_W * 40]; 
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LCD_W * 40);

    // 5. Înregistrăm ecranul nostru la LVGL
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_W;
    disp_drv.ver_res = LCD_H;
    disp_drv.flush_cb = my_disp_flush; // Legăm funcția noastră de afișare!
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // ============================================================
    // CREARE INTERFAȚĂ (Gata cu desenele manuale de pixeli!)
    // ============================================================
    
    // Creăm un buton frumos
    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 120, 50);                  // Lățime, Înălțime
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -20);     // Aliniere pe centru
    
    // Adăugăm un text pe buton
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "Apasa-ma!");
    lv_obj_center(label);

    // Creăm un slider (bara de volum/progres)
    lv_obj_t * slider = lv_slider_create(lv_scr_act());
    lv_obj_set_width(slider, 150);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 50);

    // ============================================================

    while (true) {
        // Punem biblioteca LVGL să facă magia (să animeze butoanele, etc.)
        lv_timer_handler(); 
        sleep_ms(5); // Așteptăm 5 milisecunde pentru a nu suprasolicita procesorul
    }

    return 0;
}

