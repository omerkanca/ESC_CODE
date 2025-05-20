
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"

int main() {
    stdio_init_all();
    
    uint16_t start = (uint16_t)timer_hw->timerawl;

    busy_wait_us(1500);  // 1.5 ms gecikme

    uint16_t end = (uint16_t)timer_hw->timerawl;

    uint16_t elapsed = end - start;
    
    while (true) {


    printf("Geçen süre: %u us\n", elapsed);

        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
