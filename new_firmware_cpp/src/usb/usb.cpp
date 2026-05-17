extern "C" {
#include "stm32f4xx_hal.h"
#include "usbd_core.h"
#include "usbd_hid.h"
#include "usbd_cdc.h"
#include "usbd_desc.h"
#include "crc16.h"
}

#include "usb.hpp"
#include "usb_config.h"
#include "logger/rtt.hpp"

static USBD_HandleTypeDef g_usbDevice;
static uint8_t cdc_rx_buffer[CDC_DATA_FS_MAX_PACKET_SIZE];
static uint16_t g_frame_counter = 0;
static volatile uint8_t g_cdc_subscriptions = 0;

/* ── CDC command parser state (multi-byte commands) ────────────────── */
static uint8_t  cdc_cmd_pending = 0;
static uint8_t  cdc_cmd_buf[4];
static uint8_t  cdc_cmd_idx = 0;

static USBD_CDC_LineCodingTypeDef g_line_coding = {
    115200,
    0,
    0,
    8
};

static int8_t cdc_init(void) {
    USBD_CDC_SetRxBuffer(&g_usbDevice, cdc_rx_buffer);
    uint8_t ret = USBD_CDC_ReceivePacket(&g_usbDevice);
    RTT::printf("CDC init: RxBuffer=%p ret=%d classId=%d\n",
                (void *)cdc_rx_buffer, ret, g_usbDevice.classId);
    return 0;
}
static int8_t cdc_deinit(void) {
    g_cdc_subscriptions = 0;
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
    RTT::printf("CDC rx: len=%u\n", static_cast<unsigned>(*len));
    for (uint32_t i = 0; i < *len; i++) {
        if (cdc_cmd_idx == 0) {
            cdc_cmd_pending = buf[i];
            cdc_cmd_idx = 1;
        } else if (cdc_cmd_idx < sizeof(cdc_cmd_buf) + 1) {
            cdc_cmd_buf[cdc_cmd_idx - 1] = buf[i];
            cdc_cmd_idx++;
        }
    }
    if (cdc_cmd_idx > 0) {
        USB::on_cdc_command(cdc_cmd_pending, cdc_cmd_buf, cdc_cmd_idx - 1);
        cdc_cmd_pending = 0;
        cdc_cmd_idx = 0;
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

bool USB::send_report(uint16_t x, uint16_t y, bool in_range) {
    uint8_t report[8] = {0};
    report[1] = in_range ? 0x02 : 0x00;
    report[2] = static_cast<uint8_t>(x & 0xFF);
    report[3] = static_cast<uint8_t>((x >> 8) & 0xFF);
    report[4] = static_cast<uint8_t>(y & 0xFF);
    report[5] = static_cast<uint8_t>((y >> 8) & 0xFF);

    if (g_usbDevice.dev_state == USBD_STATE_CONFIGURED) {
        USBD_HID_SendReport(&g_usbDevice, report, sizeof(report), 0);
        return true;
    }
    return false;
}

static bool cdc_tx_ready(void) {
    if (g_usbDevice.dev_state != USBD_STATE_CONFIGURED) return false;
    USBD_CDC_HandleTypeDef *hcdc =
        (USBD_CDC_HandleTypeDef *)g_usbDevice.pClassDataCmsit[USB::CDC_CLASS_ID];
    return hcdc != nullptr && hcdc->TxState == 0U;
}

static void cdc_send_frame(const uint8_t *data, uint16_t len) {
    if (!cdc_tx_ready()) return;
    USBD_CDC_SetTxBuffer(&g_usbDevice, const_cast<uint8_t*>(data), len, USB::CDC_CLASS_ID);
    USBD_CDC_TransmitPacket(&g_usbDevice, USB::CDC_CLASS_ID);
}

void USB::send_cdc_grid(const uint16_t *grid, float cx, float cy, bool cvalid) {
    static CdcGridFrame frame;

    frame.header.sync_lo  = 0xAA;
    frame.header.sync_hi  = 0x55;
    frame.header.version  = PROTOCOL_VERSION;
    frame.header.msg_type = MSG_GRID;
    frame.header.seq      = g_frame_counter;

    for (int i = 0; i < 209; i++) {
        frame.payload.values[i] = grid[i];
    }
    frame.payload.cursor_x     = static_cast<int16_t>(cx * 100.0f);
    frame.payload.cursor_y     = static_cast<int16_t>(cy * 100.0f);
    frame.payload.cursor_valid = cvalid ? 1 : 0;

    uint8_t *payload = reinterpret_cast<uint8_t*>(&frame);
    uint16_t payload_len = static_cast<uint16_t>(offsetof(CdcGridFrame, crc));
    frame.crc = crc16_ccitt(payload, payload_len);

    if (g_frame_counter < 3) {
        bool ready = cdc_tx_ready();
        RTT::printf("GRID#%u ready=%d subs=0x%02x\n",
                    static_cast<unsigned>(g_frame_counter),
                    ready ? 1 : 0,
                    static_cast<unsigned>(g_cdc_subscriptions));
        if (!ready) return;
    }
    cdc_send_frame(reinterpret_cast<const uint8_t*>(&frame), sizeof(frame));
    g_frame_counter++;
}

bool USB::is_subscribed(uint8_t flag) {
    return (g_cdc_subscriptions & flag) != 0;
}

void USB::on_cdc_command(uint8_t cmd, const uint8_t *data, uint8_t len) {
    switch (cmd) {
    case CMD_SUBSCRIBE:
        if (len >= 1) {
            g_cdc_subscriptions |= data[0];
        } else {
            g_cdc_subscriptions |= SUB_GRID;
        }
        RTT::printf("CDC: subscribe 0x%02x (now=0x%02x)\n",
                    static_cast<unsigned>(len >= 1 ? data[0] : SUB_GRID),
                    static_cast<unsigned>(g_cdc_subscriptions));
        break;
    case CMD_UNSUBSCRIBE:
        g_cdc_subscriptions = 0;
        RTT::printf("CDC: unsubscribe all\n");
        break;
    case CMD_UNSUBSCRIBE_TO:
        if (len >= 1) {
            g_cdc_subscriptions &= ~data[0];
            RTT::printf("CDC: unsubscribe 0x%02x (now=0x%02x)\n",
                        static_cast<unsigned>(data[0]),
                        static_cast<unsigned>(g_cdc_subscriptions));
        }
        break;
    default:
        break;
    }
}
