#ifndef __USBD_COMPOSITE_BUILDER_H__
#define __USBD_COMPOSITE_BUILDER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"

#if USBD_CMPSIT_ACTIVATE_HID == 1U
#include "usbd_hid.h"
#endif
#if USBD_CMPSIT_ACTIVATE_CDC == 1U
#include "usbd_cdc.h"
#endif

#ifndef USBD_CMPSIT_ACTIVATE_HID
#define USBD_CMPSIT_ACTIVATE_HID                           0U
#endif
#ifndef USBD_CMPSIT_ACTIVATE_MSC
#define USBD_CMPSIT_ACTIVATE_MSC                           0U
#endif
#ifndef USBD_CMPSIT_ACTIVATE_DFU
#define USBD_CMPSIT_ACTIVATE_DFU                           0U
#endif
#ifndef USBD_CMPSIT_ACTIVATE_CDC
#define USBD_CMPSIT_ACTIVATE_CDC                           0U
#endif
#ifndef USBD_CMPSIT_ACTIVATE_CDC_ECM
#define USBD_CMPSIT_ACTIVATE_CDC_ECM                       0U
#endif
#ifndef USBD_CMPSIT_ACTIVATE_RNDIS
#define USBD_CMPSIT_ACTIVATE_RNDIS                         0U
#endif
#ifndef USBD_CMPSIT_ACTIVATE_AUDIO
#define USBD_CMPSIT_ACTIVATE_AUDIO                         0U
#endif
#ifndef USBD_CMPSIT_ACTIVATE_CUSTOMHID
#define USBD_CMPSIT_ACTIVATE_CUSTOMHID                     0U
#endif
#ifndef USBD_CMPSIT_ACTIVATE_VIDEO
#define USBD_CMPSIT_ACTIVATE_VIDEO                         0U
#endif
#ifndef USBD_CMPSIT_ACTIVATE_PRINTER
#define USBD_CMPSIT_ACTIVATE_PRINTER                       0U
#endif
#ifndef USBD_CMPSIT_ACTIVATE_CCID
#define USBD_CMPSIT_ACTIVATE_CCID                          0U
#endif
#ifndef USBD_CMPSIT_ACTIVATE_MTP
#define USBD_CMPSIT_ACTIVATE_MTP                           0U
#endif

#ifndef USBD_CMPST_MAX_CONFDESC_SZ
#define USBD_CMPST_MAX_CONFDESC_SZ                         300U
#endif

#ifndef USBD_CONFIG_STR_DESC_IDX
#define USBD_CONFIG_STR_DESC_IDX                           4U
#endif

#ifndef USBD_COMPOSITE_USE_IAD
#define USBD_COMPOSITE_USE_IAD                             0U
#endif

typedef struct
{
    uint8_t           bLength;
    uint8_t           bDescriptorType;
    uint8_t           bFirstInterface;
    uint8_t           bInterfaceCount;
    uint8_t           bFunctionClass;
    uint8_t           bFunctionSubClass;
    uint8_t           bFunctionProtocol;
    uint8_t           iFunction;
} USBD_IadDescTypeDef;

typedef struct
{
    uint8_t           bLength;
    uint8_t           bDescriptorType;
    uint8_t           bInterfaceNumber;
    uint8_t           bAlternateSetting;
    uint8_t           bNumEndpoints;
    uint8_t           bInterfaceClass;
    uint8_t           bInterfaceSubClass;
    uint8_t           bInterfaceProtocol;
    uint8_t           iInterface;
} USBD_IfDescTypeDef;

#if (USBD_CMPSIT_ACTIVATE_CDC == 1)
typedef struct
{
    uint8_t           bLength;
    uint8_t           bDescriptorType;
    uint8_t           bDescriptorSubtype;
    uint16_t          bcdCDC;
} __PACKED USBD_CDCHeaderFuncDescTypeDef;

typedef struct
{
    uint8_t           bLength;
    uint8_t           bDescriptorType;
    uint8_t           bDescriptorSubtype;
    uint8_t           bmCapabilities;
    uint8_t           bDataInterface;
} USBD_CDCCallMgmFuncDescTypeDef;

typedef struct
{
    uint8_t           bLength;
    uint8_t           bDescriptorType;
    uint8_t           bDescriptorSubtype;
    uint8_t           bmCapabilities;
} USBD_CDCACMFuncDescTypeDef;

typedef struct
{
    uint8_t           bLength;
    uint8_t           bDescriptorType;
    uint8_t           bDescriptorSubtype;
    uint8_t           bMasterInterface;
    uint8_t           bSlaveInterface;
} USBD_CDCUnionFuncDescTypeDef;
#endif

extern USBD_ClassTypeDef USBD_CMPSIT;

uint8_t  USBD_CMPSIT_AddToConfDesc(USBD_HandleTypeDef *pdev);

#ifdef USE_USBD_COMPOSITE
uint8_t  USBD_CMPSIT_AddClass(USBD_HandleTypeDef *pdev,
                              USBD_ClassTypeDef *pclass,
                              USBD_CompositeClassTypeDef class,
                              uint8_t cfgidx);

uint32_t USBD_CMPSIT_SetClassID(USBD_HandleTypeDef *pdev,
                                USBD_CompositeClassTypeDef Class,
                                uint32_t Instance);

uint32_t USBD_CMPSIT_GetClassID(USBD_HandleTypeDef *pdev,
                                USBD_CompositeClassTypeDef Class,
                                uint32_t Instance);
#endif

uint8_t USBD_CMPST_ClearConfDesc(USBD_HandleTypeDef *pdev);

#define __USBD_CMPSIT_SET_EP(epadd, eptype, epsize, HSinterval, FSinterval) \
  do { \
    pEpDesc = ((USBD_EpDescTypeDef*)((uint32_t)pConf + *Sze)); \
    pEpDesc->bLength            = (uint8_t)sizeof(USBD_EpDescTypeDef); \
    pEpDesc->bDescriptorType    = USB_DESC_TYPE_ENDPOINT; \
    pEpDesc->bEndpointAddress   = (epadd); \
    pEpDesc->bmAttributes       = (eptype); \
    pEpDesc->wMaxPacketSize     = (uint16_t)(epsize); \
    if(speed == (uint8_t)USBD_SPEED_HIGH) \
      pEpDesc->bInterval        = HSinterval; \
    else \
      pEpDesc->bInterval        = FSinterval; \
    *Sze += (uint32_t)sizeof(USBD_EpDescTypeDef); \
  } while(0)

#define __USBD_CMPSIT_SET_IF(ifnum, alt, eps, class, subclass, protocol, istring) \
  do { \
    pIfDesc = ((USBD_IfDescTypeDef*)((uint32_t)pConf + *Sze)); \
    pIfDesc->bLength = (uint8_t)sizeof(USBD_IfDescTypeDef); \
    pIfDesc->bDescriptorType = USB_DESC_TYPE_INTERFACE; \
    pIfDesc->bInterfaceNumber = ifnum; \
    pIfDesc->bAlternateSetting = alt; \
    pIfDesc->bNumEndpoints = eps; \
    pIfDesc->bInterfaceClass = class; \
    pIfDesc->bInterfaceSubClass = subclass; \
    pIfDesc->bInterfaceProtocol = protocol; \
    pIfDesc->iInterface = istring; \
    *Sze += (uint32_t)sizeof(USBD_IfDescTypeDef); \
  } while(0)

#ifdef __cplusplus
}
#endif

#endif
