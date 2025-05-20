#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

#define SAMPLE_COUNT 100  // Kaç örnek okunacağını belirle

uint16_t adc_buffer[SAMPLE_COUNT];  // ADC verileri için tampon

void adc_dma_init() {
    adc_init();  
    adc_gpio_init(26);  // ADC0 (GP26)
    adc_gpio_init(27);  // ADC1 (GP27)
    adc_gpio_init(28);  // ADC2 (GP28)
    adc_select_input(0); // ADC0'ı seç

    int dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_ADC);

    dma_channel_configure(
        dma_chan,
        &c,
        adc_buffer,
        &adc_hw->fifo,
        SAMPLE_COUNT,
        true
    );

    adc_fifo_setup(true, false, 1, false, false);
    adc_run(true);
}

int main() {
    stdio_init_all();
    adc_dma_init();

    while (1) {
        sleep_ms(500);

        printf("ADC DMA Readings: ");
        for (int i = 0; i < SAMPLE_COUNT; i++) {
            printf("%d ", adc_buffer[i]);
        }
        printf("\n");
    }
}
