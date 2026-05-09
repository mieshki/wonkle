extern "C" {
#include "stm32f4xx_hal.h"
}

#include "rtt.hpp"
#include "sensor.hpp"
#include "usb.hpp"

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
float g_cursor_x = 0.0f;
float g_cursor_y = 0.0f;
uint8_t g_cursor_valid = 0;
uint32_t g_frame = 0;

static constexpr bool kDebugMode = false; //true;

int main() {
    RTT::init();
    RTT::printf("Hello from Wonkle C++!\n");

    HAL_Init();
    SystemClock_Config();
    SystemCoreClockUpdate();
    RTT::printf("HCLK: %u Hz\n", static_cast<unsigned>(SystemCoreClock));

    Sensor::init();
    RTT::printf("Sensor init done\n");

    USB::init();
    RTT::printf("USB init done\n");

    while (true) {
        Sensor::scan(g_grid);

        if (kDebugMode) {
            RTT::printf("===GRID===\n");
            for (uint8_t r = 0; r < Sensor::ROW_LEN; r++) {
                RTT::printf("R%02u:", r);
                for (uint8_t c = 0; c < Sensor::COL_LEN; c++) {
                    RTT::printf(" %u", g_grid[r * Sensor::COL_LEN + c]);
                }
                RTT::printf("\n");
            }
            RTT::printf("===END===\n");
        } else {
            auto cursor = Sensor::find_center(g_grid);
            g_cursor_x = cursor.x;
            g_cursor_y = cursor.y;
            g_cursor_valid = cursor.valid ? 1 : 0;

            if (cursor.valid) {
                uint16_t grid_max = 0;
                uint16_t grid_min = 65535U;
                uint16_t active_pixels = 0;
                for (uint16_t i = 0; i < Sensor::ROW_LEN * Sensor::COL_LEN; i++) {
                    uint16_t v = g_grid[i];
                    if (v > grid_max) grid_max = v;
                    if (v < grid_min) grid_min = v;
                    if (v > 2200U) active_pixels++;
                }

                uint16_t x_usb = static_cast<uint16_t>(cursor.x * 10000.0f / 18.0f);
                uint16_t y_usb = static_cast<uint16_t>(cursor.y * 10000.0f / 10.0f);
                if (x_usb > 10000) x_usb = 10000;
                if (y_usb > 10000) y_usb = 10000;

                USB::send_report(x_usb, y_usb, true, false); //true);

                RTT::printf("N F=%u X=%u.%02u Y=%u.%02u AP=%u MX=%u MN=%u\n",
                    static_cast<unsigned>(g_frame),
                    static_cast<unsigned>(cursor.x),
                    static_cast<unsigned>((cursor.x - static_cast<int>(cursor.x)) * 100.0f) % 100U,
                    static_cast<unsigned>(cursor.y),
                    static_cast<unsigned>((cursor.y - static_cast<int>(cursor.y)) * 100.0f) % 100U,
                    static_cast<unsigned>(active_pixels),
                    static_cast<unsigned>(grid_max),
                    static_cast<unsigned>(grid_min));
            } else {
                USB::send_report(0, 0, false, false);
            }
        }
        g_frame++;
    }
}
