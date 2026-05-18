#include "tablet.hpp"
#include "pins.hpp"
#include "usb/usb.hpp"
#include "logger/rtt.hpp"

extern "C" {
#include "core_cm4.h"
#include "stm32f4xx_hal.h"
}

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

void Tablet::init() {
    HAL_Init();
    SystemClock_Config();
    SystemCoreClockUpdate();
    RTT::printf("HCLK: %u Hz\n", static_cast<unsigned>(SystemCoreClock));

    RTT::printf("Waiting for 3 seconds...\n");
    HAL_Delay(3000);

    RTT::printf("Initializing...\n");
    USB::init();
    sensor_grid_.init();
    telemetry_.init(sensor_grid_);
    RTT::printf("Init done\n");
    RTT::printf("Config: mux_settling=%u, adc_sampling=%s, adc_dummy_reads=%u\n",
                static_cast<unsigned>(sensor_grid_.getMuxSettling()),
                adcSamplingLabel(sensor_grid_.getAdcSampling()),
                static_cast<unsigned>(sensor_grid_.getAdcDummyReads()));

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    tick_start_ = HAL_GetTick();
}

void Tablet::tick(bool measure) {
    telemetry_.service();

    uint32_t t0 = 0, t1 = 0, t2 = 0, t3 = 0;
    if (measure) {
        t0 = DWT->CYCCNT;
    }

    sensor_grid_.scan_grid(grid_);

    if (measure) {
        t1 = DWT->CYCCNT;
    }

    auto cursor = find_centroid(grid_);

    if (measure) {
        t2 = DWT->CYCCNT;
    }

    update_cursor(cursor);

    telemetry_.feed_grid(grid_, cursor.x, cursor.y, cursor.valid);

    if (measure) {
        t3 = DWT->CYCCNT;
        sum_scan_ += t1 - t0;
        sum_centroid_ += t2 - t1;
        sum_usb_ += t3 - t2;
        sum_cycles_ += t3 - t0;
        iterations_++;

        uint32_t elapsed = HAL_GetTick() - tick_start_;
        if (elapsed >= 2000) {
            uint32_t hz = (iterations_ * 1000) / elapsed;
            uint64_t cycles_per_iter = sum_cycles_ / iterations_;
            uint32_t us_total = static_cast<uint32_t>((static_cast<uint64_t>(cycles_per_iter) * 1000000ULL) / SystemCoreClock);
            uint32_t us_scan = static_cast<uint32_t>((sum_scan_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
            uint32_t us_centroid = static_cast<uint32_t>((sum_centroid_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
            uint32_t us_usb = static_cast<uint32_t>((sum_usb_ * 1000000ULL) / (static_cast<uint64_t>(iterations_) * SystemCoreClock));
            RTT::printf("%u Hz | scan=%uus centroid=%uus usb=%uus total=%uus\n",
                        static_cast<unsigned>(hz),
                        static_cast<unsigned>(us_scan),
                        static_cast<unsigned>(us_centroid),
                        static_cast<unsigned>(us_usb),
                        static_cast<unsigned>(us_total));
            tick_start_ = HAL_GetTick();
            iterations_ = 0;
            sum_scan_ = 0;
            sum_centroid_ = 0;
            sum_usb_ = 0;
            sum_cycles_ = 0;
        }
    }
}

void Tablet::update_cursor(const Cursor& cursor) {
    uint8_t report[8] = {0};
    if (cursor.valid) {
        uint16_t x_usb = static_cast<uint16_t>(cursor.x * 10000.0f / 18.0f);
        uint16_t y_usb = static_cast<uint16_t>(cursor.y * 10000.0f / 10.0f);
        report[1] = 0x02;
        report[2] = static_cast<uint8_t>(x_usb & 0xFF);
        report[3] = static_cast<uint8_t>((x_usb >> 8) & 0xFF);
        report[4] = static_cast<uint8_t>(y_usb & 0xFF);
        report[5] = static_cast<uint8_t>((y_usb >> 8) & 0xFF);
    }
    USB::send_hid_report(report, sizeof(report));
}

Tablet::Cursor Tablet::find_centroid(const uint16_t* grid) {
    float sum_x = 0.0f;
    float sum_y = 0.0f;
    float total = 0.0f;
    uint8_t clustered_count = 0;

    for (uint8_t r = 0; r < Pins::ROWS; r++) {
        for (uint8_t c = 0; c < Pins::COLS; c++) {
            uint16_t raw = grid[r * Pins::COLS + c];
            if (raw > threshold_) {
                bool has_neighbor =
                    (r > 0     && grid[(r - 1) * Pins::COLS + c] > threshold_) ||
                    (r < Pins::ROWS - 1 && grid[(r + 1) * Pins::COLS + c] > threshold_) ||
                    (c > 0     && grid[r * Pins::COLS + (c - 1)] > threshold_) ||
                    (c < Pins::COLS - 1  && grid[r * Pins::COLS + (c + 1)] > threshold_);

                if (has_neighbor) {
                    clustered_count++;
                    float val = static_cast<float>(raw - threshold_);
                    sum_x += val * static_cast<float>(c);
                    sum_y += val * static_cast<float>(r);
                    total += val;
                }
            }
        }
    }

    if (clustered_count >= 3 && total > 0.0f) {
        return { sum_x / total, sum_y / total, true };
    }
    return { 0.0f, 0.0f, false };
}
