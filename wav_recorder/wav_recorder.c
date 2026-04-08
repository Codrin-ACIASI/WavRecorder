#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "lvgl.h" // Includem biblioteca gigant!

// ---- Pini display ----
#define PIN_MISO  16
#define PIN_CLK   18
#define PIN_MOSI  19
#define PIN_RST   17
#define PIN_DC    21

#define LCD_W 240
#define LCD_H 320

// --- Butoane ---

#define BUTON1 20
#define BUTON2 22
#define BUTON3 26
#define BUTON4 27

// Functii ajutatoare SPI
static inline void dc_cmd()  { gpio_put(PIN_DC, 0); }
static inline void dc_data() { gpio_put(PIN_DC, 1); }
static void spi_write_byte(uint8_t b) { spi_write_blocking(spi0, &b, 1); }
static void spi_write_word(uint16_t w) {
    uint8_t buf[2] = { w >> 8, w & 0xFF };
    spi_write_blocking(spi0, buf, 2);
}

// Functii LCD
static void lcd_cmd(uint8_t cmd)   { dc_cmd();  spi_write_byte(cmd); }
static void lcd_data8(uint8_t d)   { dc_data(); spi_write_byte(d);   }
static void lcd_data16(uint16_t d) { dc_data(); spi_write_word(d);   }

static void lcd_init_ili9341() {
    gpio_put(PIN_RST, 1); sleep_ms(10);
    gpio_put(PIN_RST, 0); sleep_ms(50);
    gpio_put(PIN_RST, 1); sleep_ms(120);

    lcd_cmd(0x01); sleep_ms(5); // Reset
    lcd_cmd(0x28); // Display OFF
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

// ============================================================
// MAGIA LVGL: Functia care deseneaza
// ============================================================
void my_disp_flush(lv_disp_drv_t *disp_drv,
                   const lv_area_t *area,
                   lv_color_t *color_p)
{
    lcd_set_window(area->x1, area->y1, area->x2, area->y2);

    uint32_t size = (area->x2 - area->x1 + 1) *
                    (area->y2 - area->y1 + 1);

    dc_data();

    for(uint32_t i = 0; i < size; i++) {
        uint16_t c = color_p[i].full; 
        spi_write_word(c);
    }

    lv_disp_flush_ready(disp_drv);
}

// O functie standard pe care LVGL o cere ca sa stie cat timp a trecut
uint32_t custom_tick_get(void) {
    return to_ms_since_boot(get_absolute_time());
}

int main() {
    stdio_init_all();

    gpio_init(BUTON1);
    gpio_set_dir(BUTON1, GPIO_IN);
    gpio_pull_up(BUTON1);

    // 1. Setup SPI si Pini (10 MHz)
    spi_init(spi0, 10 * 1000 * 1000); 
    gpio_set_function(PIN_CLK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_init(PIN_DC);  gpio_set_dir(PIN_DC,  GPIO_OUT);
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT);

    // 2. Pornim fizic ecranul
    lcd_init_ili9341();

    // 3. Initializam biblioteca LVGL
    lv_init();

    // 4. Setam un "buffer" (Memorie temporara unde LVGL deseneaza formele inainte sa le afiseze)
    // Vom face un buffer mare de 40 de randuri pentru o animatie super fluida
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[LCD_W * 40]; 
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LCD_W * 40);

    // 5. Inregistram ecranul nostru la LVGL
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_W;
    disp_drv.ver_res = LCD_H;
    disp_drv.flush_cb = my_disp_flush; // Legam functia noastra de afisare!
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // ============================================================
    // CREARE INTERFATA (Gata cu desenele manuale de pixeli!)
    // ============================================================
    
    // fundal titlu
    lv_obj_t * my_rect = lv_obj_create(lv_scr_act());
    lv_obj_set_size(my_rect , 240, 120);
    lv_obj_align(my_rect, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(my_rect , lv_color_hex(0xFFFFFF), 0);
    
    // titlu
    lv_obj_t *symbol = lv_label_create(my_rect);
    lv_obj_set_style_text_font(symbol, &lv_font_montserrat_32, 0);
    lv_label_set_text(symbol, "\xEF\x80\x81");
    lv_obj_set_style_text_color(symbol, lv_color_hex(0x000000), 0);
    lv_obj_align(symbol, LV_ALIGN_TOP_LEFT, 0, 35);

    lv_obj_t *label = lv_label_create(my_rect);
    lv_label_set_text(label, "Echo Note");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 25, 35);

    // optiuni 
    // Ca sa activezi fontul, intri pe lv_conf.h si unde e definit fontul pui valoarea 1 (ex: #define LV_FONT_MONTSERRAT_20 1)
    // dupa in functia de font scrii &lv_font_montserrat_20, ca ex
    lv_obj_t *opt1 = lv_label_create(lv_scr_act());
    lv_label_set_text(opt1, "1. Inregistrare voce");
    lv_obj_set_style_text_font(opt1, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(opt1, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(opt1, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *opt2 = lv_label_create(lv_scr_act());
    lv_label_set_text(opt2, "2. Ascultare inregistrari");
    lv_obj_set_style_text_font(opt2, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(opt2, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(opt2, LV_ALIGN_CENTER, 0, 30);

    lv_obj_t *opt3 = lv_label_create(lv_scr_act());
    lv_label_set_text(opt3, "3. Oprire");
    lv_obj_set_style_text_font(opt3, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(opt3, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(opt3, LV_ALIGN_CENTER, 0, 60);

    // bar
    static lv_style_t style_bg;
    static lv_style_t style_indic;

    lv_style_init(&style_bg);
    lv_style_set_border_color(&style_bg, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_border_width(&style_bg, 2);
    lv_style_set_pad_all(&style_bg, 6); 
    lv_style_set_radius(&style_bg, 6);
    lv_style_set_anim_time(&style_bg, 1000);

    lv_style_init(&style_indic);
    lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_radius(&style_indic, 3);

    lv_obj_t * bar = lv_bar_create(lv_scr_act());
    lv_obj_remove_style_all(bar);  /*To have a clean start*/
    lv_obj_add_style(bar, &style_bg, 0);
    lv_obj_add_style(bar, &style_indic, LV_PART_INDICATOR);

    lv_obj_set_size(bar, 200, 20);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_bar_set_value(bar, 100, LV_ANIM_ON);


    // ============================================================

    while (true) {
        // Punem biblioteca LVGL sa faca magia (sa animeze butoanele, etc.)
        lv_timer_handler(); 
        lv_tick_inc(5);
        sleep_ms(5); // Asteptam 5 milisecunde pentru a nu suprasolicita procesorul
        
        //schimba culoarea textului
        if(!gpio_get(BUTON1)) {
            lv_obj_set_style_text_color(opt1, lv_color_hex(0xFFCCAA), 0);
            lv_obj_invalidate(opt1); // forțează redraw
        } else {
            lv_obj_set_style_text_color(opt1, lv_color_hex(0xFFFFFF), 0);
            lv_obj_invalidate(opt1);
        }
    }

    return 0;
}