#include "display_port.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "lvgl.h"

//============================================================ 
// ---- Pini display ----
//============================================================ 

#define PIN_MISO  16
#define PIN_CLK   18
#define PIN_MOSI  19
#define PIN_RST   17
#define PIN_DC    21

#define LCD_W 240
#define LCD_H 320


//============================================================ 
// --- Funcții private ---
//============================================================ 

static inline void dc_cmd()  { gpio_put(PIN_DC, 0); }
static inline void dc_data() { gpio_put(PIN_DC, 1); }

static void spi_write_byte(uint8_t b) { spi_write_blocking(spi0, &b, 1); }
static void spi_write_word(uint16_t w) {
    uint8_t buf[2] = { w >> 8, w & 0xFF };
    spi_write_blocking(spi0, buf, 2);
}

static void lcd_cmd(uint8_t cmd)   { dc_cmd();  spi_write_byte(cmd); }
static void lcd_data8(uint8_t d)   { dc_data(); spi_write_byte(d);   }
static void lcd_data16(uint16_t d) { dc_data(); spi_write_word(d);   }

static void lcd_init_ili9341() {
    gpio_put(PIN_RST, 1); sleep_ms(10);
    gpio_put(PIN_RST, 0); sleep_ms(50);
    gpio_put(PIN_RST, 1); sleep_ms(120);

    lcd_cmd(0x01); sleep_ms(5); 
    lcd_cmd(0x28); 
    lcd_cmd(0x3A); lcd_data8(0x55); 
    lcd_cmd(0x36); lcd_data8(0x48); 
    lcd_cmd(0x11); sleep_ms(120);   
    lcd_cmd(0x29); 
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    lcd_cmd(0x2A); lcd_data16(x0); lcd_data16(x1);
    lcd_cmd(0x2B); lcd_data16(y0); lcd_data16(y1);
    lcd_cmd(0x2C);
}

static void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    lcd_set_window(area->x1, area->y1, area->x2, area->y2);
    uint32_t size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
    dc_data();
    for(uint32_t i = 0; i < size; i++) {
        uint16_t c = color_p[i].full; 
        spi_write_word(c);
    }
    lv_disp_flush_ready(disp_drv);
}


//============================================================ 
// --- Functii publice ---
//============================================================ 


uint32_t custom_tick_get(void) {
    return to_ms_since_boot(get_absolute_time());
}

void display_port_init(void) {
    // 1. Setup SPI
    spi_init(spi0, 20 * 1000 * 1000); 
    gpio_set_function(PIN_CLK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_init(PIN_DC);  gpio_set_dir(PIN_DC,  GPIO_OUT);
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT);

    // 2. Pornim fizic ecranul
    lcd_init_ili9341();

    // 3. LVGL Init & Buffer
    lv_init();
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[LCD_W * 40]; 
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LCD_W * 40);

    // 4. Inregistram driverul LVGL
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_W;
    disp_drv.ver_res = LCD_H;
    disp_drv.flush_cb = my_disp_flush; 
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // 5. Setăm fundalul negru general
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
}