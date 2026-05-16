#pragma once

/* ── Device descriptors ────────────────────────────────────────────── */
#define USBD_VID                      0x1209
#define USBD_PID                      0x02D7
#define USBD_LANGID_STRING            0x0409
#define USBD_MANUFACTURER_STRING      "Wonkleboard"
#define USBD_PRODUCT_HS_STRING        "Wonkleboard Prototype"
#define USBD_PRODUCT_FS_STRING        "Wonkleboard Prototype"
#define USBD_CONFIGURATION_HS_STRING  "HID Config"
#define USBD_CONFIGURATION_FS_STRING  "HID Config"
#define USBD_INTERFACE_HS_STRING      "HID Interface"
#define USBD_INTERFACE_FS_STRING      "HID Interface"

/* ── Power ─────────────────────────────────────────────────────────── */
#define USBD_SELF_POWERED           1U
#define USBD_MAX_POWER              0x32U  /* 100 mA */

/* ── HID endpoint ──────────────────────────────────────────────────── */
#define HID_EPIN_ADDR               0x81U
#define HID_EPIN_SIZE               0x08U

/* ── HID polling interval (10 ms FS, 8 ms HS) ──────────────────────── */
#define HID_FS_BINTERVAL            0x0AU
#define HID_HS_BINTERVAL            0x07U

/* ── Report descriptor ─────────────────────────────────────────────── */
#define HID_DIGITIZER_REPORT_DESC_SIZE  76U
