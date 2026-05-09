#include "usb.hpp"

extern "C" {
#include "usbd_core.h"
#include "usbd_hid.h"
#include "usbd_desc.h"
}

static USBD_HandleTypeDef hUsbDeviceFS;

struct __attribute__((packed)) HidReport {
    uint8_t unused;
    uint8_t buttons;
    uint16_t x;
    uint16_t y;
    uint16_t pressure;
};

static HidReport g_report = {};

void USB::init() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &gpio);

    USBD_Init(&hUsbDeviceFS, &Class_Desc, 0);
    USBD_RegisterClass(&hUsbDeviceFS, &USBD_HID);
    USBD_Start(&hUsbDeviceFS);
}

void USB::poll() {
}

bool USB::send_report(uint16_t x, uint16_t y, bool in_range, bool tip) {
    g_report.unused = 0;
    g_report.buttons = (in_range ? 0x02 : 0x00) | (tip ? 0x04 : 0x00);
    g_report.x = x;
    g_report.y = y;
    g_report.pressure = tip ? 1024 : 0;

    if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) {
        USBD_HID_SendReport(&hUsbDeviceFS, reinterpret_cast<uint8_t*>(&g_report), sizeof(g_report));
        return true;
    }
    return false;
}
