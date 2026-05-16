extern "C" {
#include "stm32f4xx_hal.h"
#include "usbd_core.h"
#include "usbd_hid.h"
#include "usbd_desc.h"
}

#include "usb.hpp"

static USBD_HandleTypeDef g_usbDevice;

void USB::init() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &gpio);

    USBD_Init(&g_usbDevice, &Class_Desc, 0);
    USBD_RegisterClass(&g_usbDevice, USBD_HID_CLASS);
    USBD_Start(&g_usbDevice);
}

bool USB::send_report(uint16_t x, uint16_t y, bool in_range, bool tip) {
    // Build 8-byte HID report matching digitizer descriptor:
    // Byte 0: padding (unused)
    // Byte 1: buttons (bit 1 = in_range, bit 2 = tip)
    // Bytes 2-3: X (uint16_t LE, 0-10000)
    // Bytes 4-5: Y (uint16_t LE, 0-10000)
    // Bytes 6-7: Tip Pressure (uint16_t LE, 0-2047)
    uint8_t report[8] = {0};
    report[1] = (in_range ? 0x02 : 0x00) | (tip ? 0x04 : 0x00);
    report[2] = static_cast<uint8_t>(x & 0xFF);
    report[3] = static_cast<uint8_t>((x >> 8) & 0xFF);
    report[4] = static_cast<uint8_t>(y & 0xFF);
    report[5] = static_cast<uint8_t>((y >> 8) & 0xFF);
    if (tip) {
        report[6] = 0x00;
        report[7] = 0x04;
    }

    if (g_usbDevice.dev_state == USBD_STATE_CONFIGURED) {
        USBD_HID_SendReport(&g_usbDevice, report, sizeof(report));
        return true;
    }
    return false;
}
