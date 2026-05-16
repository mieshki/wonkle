extern "C" {
#include "stm32f4xx_hal.h"
}

#include "tablet.hpp"
#include "benchmarks/grid_performance.hpp"
#include "logger/rtt.hpp"

static Tablet g_tablet;

int main() {
    RTT::init();
    RTT::printf("Hello from Wonkle!\n");

    g_tablet.init();

    SensorGridBenchmark bench(g_tablet.get_sensor_grid());

    while (1) {
        // bench.run();
        // HAL_Delay(1000);
        g_tablet.tick();
    }
}
