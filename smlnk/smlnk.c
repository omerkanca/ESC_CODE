#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

#define PWM_GPIO 2  // PWM çıkış pini (GPIO 2)

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int main() {
    adc_init();
    adc_gpio_init(26);

    // GPIO 2'yi PWM olarak ayarla
    gpio_set_function(PWM_GPIO, GPIO_FUNC_PWM);
    
    // PWM dilimini (slice) bul
    uint slice_num = pwm_gpio_to_slice_num(PWM_GPIO);

    // PWM frekansını 1 kHz yap
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 250.0f); // 125 MHz / 500 = 250 kHz
    pwm_config_set_wrap(&config, 10000);     // 250 kHz / 5000 = 50 Hz

    pwm_init(slice_num, &config, true); // PWM başlat

    while (true) {
        adc_select_input(0);
        uint adc_0 = adc_read();
        long speed = map(adc_0, 0, 4095, 500, 1000);
        pwm_set_gpio_level(PWM_GPIO, speed);
        sleep_ms(50);
    }
}
