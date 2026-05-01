extern "C" {
#include "stm32f4xx_hal.h"
}

#include "rtt.hpp"
#include "sensor.hpp"

static void Error_Handler(void) {
    while (1) { __NOP(); }
}

static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) { Error_Handler(); }
}

uint16_t g_grid[Sensor::ROW_LEN * Sensor::COL_LEN];

// looks like best is weighted centroid with 2200 threshold
int main() {
    RTT::init();
    RTT::printf("Hello from Wonkle C++!\n");

    HAL_Init();
    SystemClock_Config();
    SystemCoreClockUpdate();
    RTT::printf("HCLK: %u Hz\n", static_cast<unsigned>(SystemCoreClock));

    Sensor::init();
    RTT::printf("Sensor init done\n");

    while (true) {
        Sensor::scan(g_grid);
        RTT::printf("===GRID===\n");
        for (uint8_t r = 0; r < Sensor::ROW_LEN; r++) {
            RTT::printf("R%02u:", r);
            for (uint8_t c = 0; c < Sensor::COL_LEN; c++) {
                RTT::printf(" %u", g_grid[r * Sensor::COL_LEN + c]);
            }
            RTT::printf("\n");
        }
        RTT::printf("===END===\n");
    }
}
