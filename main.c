#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "hdmi.pio.h"
#include "hdmi_clk.pio.h"
#include "oled.h"

// Pinos HDMI (conforme README.md)
#define HDMI_D0   16
#define HDMI_D1   17
#define HDMI_D2   18
#define HDMI_CLK  19

// Botões
#define BUTTON_A  5 // GP28
#define BUTTON_B  6 // GP9

// Estados dos padrões
typedef enum {
    PATTERN_COLOR_BARS = 0,
    PATTERN_IMAGE_TEST,
    PATTERN_FRACTAL,
    PATTERN_SMPTE_OVERRIDE
} pattern_state_t;

volatile pattern_state_t current_pattern = PATTERN_COLOR_BARS;
volatile pattern_state_t previous_pattern = PATTERN_COLOR_BARS;
volatile bool fractal_active = false;
volatile bool button_a_pressed = false;
volatile bool button_b_pressed = false;
volatile bool smpte_override_active = false;

const char *pattern_names[] = {
    "Faixas Coloridas",
    "SMPTE",
    "Fractal",
    "SMPTE (Override)"
};

void oled_show_pattern(pattern_state_t pattern) {
    oled_clear();
    oled_set_text_line(0, "Padrão:", OLED_ALIGN_LEFT);
    oled_set_text_line(1, pattern_names[pattern], OLED_ALIGN_LEFT);
    oled_render_text();
}

void check_both_buttons() {
    // Leitura direta dos estados dos botões
    bool a = !gpio_get(BUTTON_A); // ativo baixo
    bool b = !gpio_get(BUTTON_B);
    if (a && b) {
        if (!smpte_override_active) {
            previous_pattern = current_pattern;
            current_pattern = PATTERN_SMPTE_OVERRIDE;
            smpte_override_active = true;
            oled_show_pattern(PATTERN_SMPTE_OVERRIDE);
        } else {
            current_pattern = previous_pattern;
            smpte_override_active = false;
            oled_show_pattern(current_pattern);
        }
    }
}

#define DEBOUNCE_MS 150
static uint32_t last_a_time = 0;
static uint32_t last_b_time = 0;

void gpio_irq_callback(uint gpio, uint32_t events) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (gpio == BUTTON_A) {
        if (now - last_a_time < DEBOUNCE_MS) return;
        last_a_time = now;
        button_a_pressed = true;
        check_both_buttons();
        if (smpte_override_active) return;
        if (fractal_active) return; // Ignorar se fractal
        if (current_pattern == PATTERN_COLOR_BARS)
            current_pattern = PATTERN_IMAGE_TEST;
        else
            current_pattern = PATTERN_COLOR_BARS;
        oled_show_pattern(current_pattern);
    } else if (gpio == BUTTON_B) {
        if (now - last_b_time < DEBOUNCE_MS) return;
        last_b_time = now;
        button_b_pressed = true;
        check_both_buttons();
        if (smpte_override_active) return;
        if (fractal_active) {
            fractal_active = false;
            current_pattern = PATTERN_COLOR_BARS;
        } else {
            fractal_active = true;
            current_pattern = PATTERN_FRACTAL;
        }
        oled_show_pattern(current_pattern);
    }
}

// Configuração do PIO para HDMI (3 bits RGB)
PIO pio_hdmi = NULL;
PIO pio_hdmi_clk = NULL;
uint sm_hdmi = 0;
uint hdmi_offset = 0;
uint sm_hdmi_clk = 1;
uint hdmi_clk_offset = 0;

void setup_hdmi_pio() {
    pio_hdmi = pio1;
    sm_hdmi = 0;
    hdmi_offset = pio_add_program(pio_hdmi, &hdmi_rgb3_program);
    hdmi_rgb3_program_init(pio_hdmi, sm_hdmi, hdmi_offset, HDMI_D0);
    pio_sm_set_enabled(pio_hdmi, sm_hdmi, true);
}

void setup_hdmi_clk_pio() {
    pio_hdmi_clk = pio1;
    sm_hdmi_clk = 1;
    hdmi_clk_offset = pio_add_program(pio_hdmi_clk, &hdmi_clk_program);
    hdmi_clk_program_init(pio_hdmi_clk, sm_hdmi_clk, hdmi_clk_offset, HDMI_CLK);
    pio_sm_set_enabled(pio_hdmi_clk, sm_hdmi_clk, true);
}

void setup_buttons() {
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_callback);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);
}

// Geração de faixas coloridas verticais (test pattern simples)
#define HDMI_WIDTH  720
#define HDMI_HEIGHT 480

void hdmi_draw_color_bars() {
    // 8 faixas verticais, 3 bits RGB (0-7)
    for (int y = 0; y < HDMI_HEIGHT; y++) {
        for (int x = 0; x < HDMI_WIDTH; x++) {
            uint8_t bar = (x * 8) / HDMI_WIDTH;
            while (pio_sm_is_tx_fifo_full(pio_hdmi, sm_hdmi));
            pio_sm_put_blocking(pio_hdmi, sm_hdmi, bar & 0x7);
        }
    }
}

// SMPTE color bars simplificado (8 barras horizontais)
void hdmi_draw_smpte() {
    // Cores SMPTE (simplificadas para 3 bits RGB):
    // 0: Preto   (000)
    // 1: Azul    (001)
    // 2: Vermelho(100)
    // 3: Magenta (101)
    // 4: Verde   (010)
    // 5: Ciano   (011)
    // 6: Amarelo (110)
    // 7: Branco  (111)
    uint8_t smpte_colors[8] = {7, 6, 5, 4, 3, 2, 1, 0};
    int bar_height = HDMI_HEIGHT / 8;
    for (int y = 0; y < HDMI_HEIGHT; y++) {
        int bar = y / bar_height;
        if (bar > 7) bar = 7;
        for (int x = 0; x < HDMI_WIDTH; x++) {
            while (pio_sm_is_tx_fifo_full(pio_hdmi, sm_hdmi));
            pio_sm_put_blocking(pio_hdmi, sm_hdmi, smpte_colors[bar]);
        }
    }
}

int main() {
    stdio_init_all();

    sleep_ms(6000);
    oled_init();
    oled_show_pattern(current_pattern);
    setup_hdmi_clk_pio();
    setup_hdmi_pio();
    setup_buttons();

    while (true) {
        switch (current_pattern) {
            case PATTERN_COLOR_BARS:
                hdmi_draw_color_bars();
                break;
            case PATTERN_IMAGE_TEST:
                hdmi_draw_smpte();
                break;
            case PATTERN_FRACTAL:
                // TODO: gerar fractal animado
                break;
            case PATTERN_SMPTE_OVERRIDE:
                hdmi_draw_smpte();
                break;
        }
        sleep_ms(10); // Ajuste conforme necessário
    }
}

