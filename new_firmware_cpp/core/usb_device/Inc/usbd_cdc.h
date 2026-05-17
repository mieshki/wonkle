#ifndef __USB_CDC_H
#define __USB_CDC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_ioreq.h"

#ifndef CDC_IN_EP
#define CDC_IN_EP                                   0x82U
#endif
#ifndef CDC_OUT_EP
#define CDC_OUT_EP                                  0x01U
#endif
#ifndef CDC_CMD_EP
#define CDC_CMD_EP                                  0x83U
#endif

#ifndef CDC_CMD_PACKET_SIZE
#define CDC_CMD_PACKET_SIZE                         8U
#endif

#ifndef CDC_DATA_HS_MAX_PACKET_SIZE
#define CDC_DATA_HS_MAX_PACKET_SIZE                 512U
#endif
#ifndef CDC_DATA_FS_MAX_PACKET_SIZE
#define CDC_DATA_FS_MAX_PACKET_SIZE                 64U
#endif

#define CDC_DATA_FS_IN_PACKET_SIZE                  CDC_DATA_FS_MAX_PACKET_SIZE
#define CDC_DATA_FS_OUT_PACKET_SIZE                 CDC_DATA_FS_MAX_PACKET_SIZE

#define CDC_REQ_MAX_DATA_SIZE                       0x7U

#define CDC_SEND_ENCAPSULATED_COMMAND               0x00U
#define CDC_GET_ENCAPSULATED_RESPONSE               0x01U
#define CDC_SET_COMM_FEATURE                        0x02U
#define CDC_GET_COMM_FEATURE                        0x03U
#define CDC_CLEAR_COMM_FEATURE                      0x04U
#define CDC_SET_LINE_CODING                         0x20U
#define CDC_GET_LINE_CODING                         0x21U
#define CDC_SET_CONTROL_LINE_STATE                  0x22U
#define CDC_SEND_BREAK                              0x23U

typedef struct
{
    uint32_t bitrate;
    uint8_t  format;
    uint8_t  paritytype;
    uint8_t  datatype;
} USBD_CDC_LineCodingTypeDef;

typedef struct _USBD_CDC_Itf
{
    int8_t (* Init)(void);
    int8_t (* DeInit)(void);
    int8_t (* Control)(uint8_t cmd, uint8_t *pbuf, uint16_t length);
    int8_t (* Receive)(uint8_t *Buf, uint32_t *Len);
    int8_t (* TransmitCplt)(uint8_t *Buf, uint32_t *Len, uint8_t epnum);
} USBD_CDC_ItfTypeDef;

typedef struct
{
    uint32_t data[CDC_DATA_HS_MAX_PACKET_SIZE / 4U];
    uint8_t  CmdOpCode;
    uint8_t  CmdLength;
    uint8_t  *RxBuffer;
    uint8_t  *TxBuffer;
    uint32_t RxLength;
    uint32_t TxLength;

    __IO uint32_t TxState;
    __IO uint32_t RxState;
} USBD_CDC_HandleTypeDef;

extern USBD_ClassTypeDef USBD_CDC;
#define USBD_CDC_CLASS &USBD_CDC

uint8_t USBD_CDC_RegisterInterface(USBD_HandleTypeDef *pdev,
                                   USBD_CDC_ItfTypeDef *fops);

#ifdef USE_USBD_COMPOSITE
uint8_t USBD_CDC_SetTxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff,
                             uint32_t length, uint8_t ClassId);
uint8_t USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev, uint8_t ClassId);
#else
uint8_t USBD_CDC_SetTxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff,
                             uint32_t length);
uint8_t USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev);
#endif

uint8_t USBD_CDC_SetRxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff);
uint8_t USBD_CDC_ReceivePacket(USBD_HandleTypeDef *pdev);

#ifdef __cplusplus
}
#endif

#endif
