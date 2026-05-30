extern "C" {
#include "stm32f4xx_hal.h"
#include "usbd_core.h"
#include "usbd_hid.h"
#include "usbd_cdc.h"
#include "usbd_desc.h"
}

#include "usb.hpp"
#include "usb_config.h"
#include "logger/rtt.hpp"

static USBD_HandleTypeDef g_usbDevice;
static uint8_t cdc_rx_buffer[CDC_DATA_FS_MAX_PACKET_SIZE];

#define RX_PACKET_MAX  64
#define RX_PACKET_RING 8

static struct { uint8_t data[RX_PACKET_MAX]; uint16_t len; } g_rx_packets[RX_PACKET_RING];
static volatile uint8_t g_rx_pkt_head = 0;
static uint8_t g_rx_pkt_tail = 0;
static volatile bool g_rx_overflow_occurred = false;

static USBD_CDC_LineCodingTypeDef g_line_coding = {
    115200,
    0,
    0,
    8
};

static int8_t cdc_init(void) {
    USBD_CDC_SetRxBuffer(&g_usbDevice, cdc_rx_buffer);
    uint8_t ret = USBD_CDC_ReceivePacket(&g_usbDevice);
    RTT::printf("CDC init: RxBuffer=%p ret=%d\n", (void*)cdc_rx_buffer, ret);
    return 0;
}

static int8_t cdc_deinit(void) {
    return 0;
}

static int8_t cdc_control(uint8_t cmd, uint8_t *pbuf, uint16_t length) {
    (void)length;
    switch (cmd) {
    case CDC_SET_LINE_CODING:
        g_line_coding.bitrate     = (uint32_t)pbuf[0] | ((uint32_t)pbuf[1] << 8)
                                  | ((uint32_t)pbuf[2] << 16) | ((uint32_t)pbuf[3] << 24);
        g_line_coding.format      = pbuf[4];
        g_line_coding.paritytype  = pbuf[5];
        g_line_coding.datatype    = pbuf[6];
        break;
    case CDC_GET_LINE_CODING:
        pbuf[0] = (uint8_t)(g_line_coding.bitrate & 0xFF);
        pbuf[1] = (uint8_t)((g_line_coding.bitrate >> 8) & 0xFF);
        pbuf[2] = (uint8_t)((g_line_coding.bitrate >> 16) & 0xFF);
        pbuf[3] = (uint8_t)((g_line_coding.bitrate >> 24) & 0xFF);
        pbuf[4] = g_line_coding.format;
        pbuf[5] = g_line_coding.paritytype;
        pbuf[6] = g_line_coding.datatype;
        break;
    case CDC_SET_CONTROL_LINE_STATE:
        break;
    default:
        break;
    }
    return 0;
}

static int8_t cdc_receive(uint8_t *buf, uint32_t *len) {
    uint8_t next = (g_rx_pkt_head + 1) % RX_PACKET_RING;
    if (next == g_rx_pkt_tail) {
        g_rx_overflow_occurred = true;
    } else {
        uint16_t n = *len > RX_PACKET_MAX ? RX_PACKET_MAX : static_cast<uint16_t>(*len);
        for (uint16_t i = 0; i < n; i++) {
            g_rx_packets[g_rx_pkt_head].data[i] = buf[i];
        }
        g_rx_packets[g_rx_pkt_head].len = n;
        g_rx_pkt_head = next;
    }
    USBD_CDC_SetRxBuffer(&g_usbDevice, cdc_rx_buffer);
    USBD_CDC_ReceivePacket(&g_usbDevice);
    return 0;
}

static int8_t cdc_transmit_cplt(uint8_t *buf, uint32_t *len, uint8_t epnum) {
    (void)buf; (void)len; (void)epnum;
    return 0;
}

static USBD_CDC_ItfTypeDef cdc_interface_fops = {
    cdc_init,
    cdc_deinit,
    cdc_control,
    cdc_receive,
    cdc_transmit_cplt
};

static void check_rx_overflow() {
    if (g_rx_overflow_occurred) {
        g_rx_overflow_occurred = false;
        RTT::printf("WARNING: CDC RX ring buffer overflow — packet(s) dropped\n");
    }
}

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

    static uint8_t hidEpAddr[] = { HID_EPIN_ADDR };
    static uint8_t cdcEpAddr[] = { CDC_IN_EP, CDC_OUT_EP, CDC_CMD_EP };

    USBD_RegisterClassComposite(&g_usbDevice, &USBD_HID, CLASS_TYPE_HID, hidEpAddr);
    USBD_RegisterClassComposite(&g_usbDevice, &USBD_CDC, CLASS_TYPE_CDC, cdcEpAddr);

    g_usbDevice.classId = CDC_CLASS_ID;
    USBD_CDC_RegisterInterface(&g_usbDevice, &cdc_interface_fops);

    USBD_Start(&g_usbDevice);

    RTT::printf("USB composite started (HID+CDC)\n");
}

bool USB::send_hid_report(const uint8_t* data, uint16_t len) {
    if (g_usbDevice.dev_state != USBD_STATE_CONFIGURED) return false;
    USBD_HID_SendReport(&g_usbDevice, const_cast<uint8_t*>(data), len, 0);
    return true;
}

static bool cdc_tx_ready(void) {
    if (g_usbDevice.dev_state != USBD_STATE_CONFIGURED) return false;
    USBD_CDC_HandleTypeDef *hcdc =
        (USBD_CDC_HandleTypeDef *)g_usbDevice.pClassDataCmsit[USB::CDC_CLASS_ID];
    return hcdc != nullptr && hcdc->TxState == 0U;
}

bool USB::cdc_send_frame(const uint8_t *data, uint16_t len) {
    if (!cdc_tx_ready()) return false;
    USBD_CDC_SetTxBuffer(&g_usbDevice, const_cast<uint8_t*>(data), len, USB::CDC_CLASS_ID);
    USBD_CDC_TransmitPacket(&g_usbDevice, USB::CDC_CLASS_ID);
    return true;
}

uint16_t USB::cdc_rx_pop(uint8_t* buf, uint16_t maxlen) {
    check_rx_overflow();
    if (g_rx_pkt_tail == g_rx_pkt_head) return 0;
    uint16_t n = g_rx_packets[g_rx_pkt_tail].len;
    if (n > maxlen) n = maxlen;
    for (uint16_t i = 0; i < n; i++) {
        buf[i] = g_rx_packets[g_rx_pkt_tail].data[i];
    }
    g_rx_pkt_tail = (g_rx_pkt_tail + 1) % RX_PACKET_RING;
    return n;
}
