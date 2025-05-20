#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"    // Temel kütüphaneler
#include "hardware/pwm.h"   // PWM darbe genişliği için
#include "hardware/irq.h"   // Kesme işlemleri için
#include "hardware/clocks.h"    // Saat frekansı için 
#include <pico/multicore.h>    // İki çekirdekli işlemler için
#include "hardware/adc.h"   // ADC okuma işlemi için
#include "hardware/timer.h"
 
#define MEASURE_PIN 5   // PWM giriş pini ( Hız ayarı için )
#define A_HIGH 13   // Mosfet sürücü pinleri
#define B_HIGH 14
#define C_HIGH 15
#define A_LOW 16
#define B_LOW 17
#define C_LOW 18

#define St_speed 50

#define NUM 0
#define IRQ TIMER_IRQ_0
static volatile bool flag = true;

uint adc_0 = 0; // ADC değişkenleri
uint adc_1 = 0;
uint adc_2 = 0;

int step=1; // 6 step fonksiyonunda adım belirtmek için
int speed = 0;  // PWM çıkışların son halidir
uint StepTime = 0;
bool Drm = 0;
uint16_t elapsed = 0;
uint16_t last_time = 0;

volatile float duty_cycle = 0.0f;   // Duty değeri için ortak değişken ( volatile diğer çekirdekten değiştirilebilir )

// Kesme
void Kesme() {
    hw_clear_bits(&timer_hw->intr, 1u << NUM);
    flag = true;
}
// Alarmı başlatan fonksiyon
void start_min(uint32_t delay_us) {
    uint64_t now = timer_hw->timerawl;
    timer_hw->alarm[NUM] = (uint32_t)(now + delay_us);
}
void init_alarm_system() {
    hw_set_bits(&timer_hw->inte, 1u << NUM);
    irq_set_exclusive_handler(IRQ, Kesme);
    irq_set_enabled(IRQ, true);
}

// Faz anahtarlama
void switch_phase(int step, int speed) {
    switch (step) {
        case 1:
            gpio_put(A_LOW, 1);
            gpio_put(C_LOW, 1);
            gpio_put(B_LOW, 0);
            pwm_set_gpio_level(C_HIGH, 0);
            pwm_set_gpio_level(B_HIGH, 0);
            pwm_set_gpio_level(A_HIGH, speed);
            break;
        case 2:
            gpio_put(B_LOW, 1);
            gpio_put(A_LOW, 1);
            gpio_put(C_LOW, 0);
            pwm_set_gpio_level(B_HIGH, 0);
            pwm_set_gpio_level(C_HIGH, 0);
            pwm_set_gpio_level(A_HIGH, speed);
            break;
        case 3:
            gpio_put(A_LOW, 1);
            gpio_put(B_LOW, 1);
            gpio_put(C_LOW, 0);
            pwm_set_gpio_level(A_HIGH, 0);
            pwm_set_gpio_level(C_HIGH, 0);
            pwm_set_gpio_level(B_HIGH, speed);
            break;
        case 4:
            gpio_put(C_LOW, 1);
            gpio_put(B_LOW, 1);
            gpio_put(A_LOW, 0);
            pwm_set_gpio_level(A_HIGH, 0);
            pwm_set_gpio_level(C_HIGH, 0);
            pwm_set_gpio_level(B_HIGH, speed);
            break;
        case 5:
            gpio_put(B_LOW, 1);
            gpio_put(C_LOW, 1);
            gpio_put(A_LOW, 0);
            pwm_set_gpio_level(B_HIGH, 0);
            pwm_set_gpio_level(A_HIGH, 0);
            pwm_set_gpio_level(C_HIGH, speed);
            break;
        case 6:
            gpio_put(A_LOW, 1);
            gpio_put(C_LOW, 1);
            gpio_put(B_LOW, 0);
            pwm_set_gpio_level(A_HIGH, 0);
            pwm_set_gpio_level(B_HIGH, 0);
            pwm_set_gpio_level(C_HIGH, speed);
            break;
    }
}

// Duty cycle ölçme fonksiyonu
float measure_duty_cycle(uint gpio) {
    gpio_set_dir(gpio, GPIO_IN);
    while (!gpio_get(gpio)); // LOW → HIGH
    absolute_time_t t1 = get_absolute_time();
    while (gpio_get(gpio));  // HIGH → LOW
    absolute_time_t t2 = get_absolute_time();
    while (!gpio_get(gpio)); // LOW → HIGH (next)
    absolute_time_t t3 = get_absolute_time();

    float high_time = absolute_time_diff_us(t1, t2);
    float period = absolute_time_diff_us(t1, t3);
    return high_time / period;
}

// 2. çekirdekte çalışacak fonksiyon
void core1_entry() {
    while (1) {
        float duty = measure_duty_cycle(MEASURE_PIN);   // Duty ölçüm fonksiyonu kullanılır
        if (duty >= 0.0f) {
            duty_cycle = duty; // Global değişkene ölçülen değeri yazılır
        }
        if (duty <= 0.05f){ // Sınırlandırmalar yapılır
            duty_cycle= 0.05f;
        }
        if (duty >= 0.1f){
            duty_cycle= 0.1f;
        }
        sleep_ms(500);
    }
}

// PWM çıkış ayarları
void setup_pwm(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM); // GPIO'yu PWM olarak ayarla
    uint slice_num = pwm_gpio_to_slice_num(gpio); // PWM dilimini al

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 125.0f); // 125 MHz / 125 = 1 MHz
    pwm_config_set_wrap(&config, 1000);     // 1 MHz / 1000 = 1 kHz

    pwm_init(slice_num, &config, true); // PWM başlat
}

// Adım kontrol
int ortalama(int step){
    busy_wait_us(elapsed/3); 
    adc_select_input(0);    // Her farklı pinden okuma için yeniden yapılandırılır
    adc_0 = adc_read(); // Okunan değerler değişkenlere atanır
    adc_select_input(1);
    adc_1 = adc_read();
    adc_select_input(2);
    adc_2 = adc_read();
    int deg ;
    switch (step)
    {
    case 1:
    deg = (adc_0 + adc_1)/2 - adc_2;
        break;
    case 2:
    deg = (adc_0 + adc_2)/2 - adc_1;        
        break;
    case 3:
    deg = (adc_1 + adc_2)/2 - adc_0;       
        break;
    case 4:
    deg = (adc_0 + adc_1)/2 - adc_2;       
        break;
    case 5:
    deg = (adc_0 + adc_2)/2 - adc_1;         
        break;
    case 6:
    deg = (adc_1 + adc_2)/2 - adc_0;       
        break;
    default:
        break;
    }
    return(deg > -100 && deg < 100);    // back emf tolerans aralığı
}

//  Motor start fonk
void START() {
    StepTime = 15000;
    while (StepTime > 1500)
    {
        if (flag) {
            flag = false;
            switch_phase(step, St_speed);
            step ++;
            if (step > 6 ) step = 1; 
            StepTime = StepTime-(StepTime/50);
            start_min(StepTime);
        }
        speed = (duty_cycle-0.05f) * 10000.0f;   // speed değişkenini duty değerine göre sürekli günceller
        if(speed < St_speed) break;
    }
    
}

int main() {
    
    multicore_launch_core1(core1_entry);    // 2. çekirdeği başlatır ve core1_entry fonksiyonunu başlatır

    init_alarm_system();

    adc_init(); // ADC donanımını çalıştırır
    adc_gpio_init(26);  // ADC pinleri seçilir
    adc_gpio_init(27);
    adc_gpio_init(28);
    
    gpio_init(A_LOW);   // LOW pinleri çıkış olarak ayarlanır
    gpio_set_dir(A_LOW, GPIO_OUT);
    gpio_init(B_LOW);
    gpio_set_dir(B_LOW, GPIO_OUT);
    gpio_init(C_LOW);
    gpio_set_dir(C_LOW, GPIO_OUT);
    setup_pwm(A_HIGH);  // HIGH pinleri pwm çıkışı olarak ayarlanır
    setup_pwm(B_HIGH);
    setup_pwm(C_HIGH);
    
    while (true) {
        if(Drm == 0) switch_phase(step, 0);
        if(ortalama(step)){ // adım kontrol

            uint16_t now = (uint16_t)timer_hw->timerawl;    // Adımlar arası zaman ölçümü
            if (last_time != 0) {  // eğer daha önce bir zaman varsa
                elapsed = now - last_time;
            }
            last_time = now;

            step ++;
            if (step > 6 ) step = 1;   // Step değişkenini belli aralıkta döngüde tutar
        }
        speed = (duty_cycle-0.05f) * 10000.0f;   // speed değişkenini duty değerine göre sürekli günceller
        if (Drm == 1) switch_phase(step, speed);
        if (speed < St_speed) Drm = 0;
        if (Drm == 0 && speed > St_speed)
        {
            START();
            Drm = 1;
        }
        
    } 
}
