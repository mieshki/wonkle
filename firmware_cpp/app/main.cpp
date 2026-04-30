extern "C" {
#include "stm32f4xx_hal.h"
}

#include "rtt.hpp"

volatile uint32_t g_counter = 0;

int main() {
    RTT::init();
    RTT::printf("Hello from Wonkle C++!\n");

    HAL_Init();
    RTT::printf("HAL_Init done\n");

    for (int i = 0; i < 100; i++) {
        g_counter++;
        RTT::printf("Tick\n");
    }

    RTT::printf("Done!\n");

    while (true) {
        g_counter++;
    }
}
