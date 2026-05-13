#ifndef __USBD_DESC_H
#define __USBD_DESC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_def.h"

#define USB_SIZ_STRING_SERIAL       0x1A
#define DEVICE_ID1                  UID_BASE
#define DEVICE_ID2                  (UID_BASE + 4U)
#define DEVICE_ID3                  (UID_BASE + 8U)

extern USBD_DescriptorsTypeDef Class_Desc;

#ifdef __cplusplus
}
#endif

#endif