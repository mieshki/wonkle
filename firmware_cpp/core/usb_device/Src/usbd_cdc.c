#include "usbd_cdc.h"
#include "usbd_ctlreq.h"
#include "stm32f4xx_hal.h"

static uint8_t USBD_CDC_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CDC_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CDC_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t USBD_CDC_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CDC_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CDC_EP0_RxReady(USBD_HandleTypeDef *pdev);
#ifndef USE_USBD_COMPOSITE
static uint8_t *USBD_CDC_GetFSCfgDesc(uint16_t *length);
static uint8_t *USBD_CDC_GetHSCfgDesc(uint16_t *length);
static uint8_t *USBD_CDC_GetOtherSpeedCfgDesc(uint16_t *length);
static uint8_t *USBD_CDC_GetDeviceQualifierDescriptor(uint16_t *length);
#endif

#ifndef USE_USBD_COMPOSITE
__ALIGN_BEGIN static uint8_t USBD_CDC_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
    USB_LEN_DEV_QUALIFIER_DESC,
    USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
};
#endif

USBD_ClassTypeDef USBD_CDC =
{
    USBD_CDC_Init,
    USBD_CDC_DeInit,
    USBD_CDC_Setup,
    NULL,
    USBD_CDC_EP0_RxReady,
    USBD_CDC_DataIn,
    USBD_CDC_DataOut,
    NULL,
    NULL,
    NULL,
#ifdef USE_USBD_COMPOSITE
    NULL,
    NULL,
    NULL,
    NULL,
#else
    USBD_CDC_GetHSCfgDesc,
    USBD_CDC_GetFSCfgDesc,
    USBD_CDC_GetOtherSpeedCfgDesc,
    USBD_CDC_GetDeviceQualifierDescriptor,
#endif
#if (USBD_SUPPORT_USER_STRING_DESC == 1U)
    NULL,
#endif
};

#ifndef USE_USBD_COMPOSITE
__ALIGN_BEGIN static uint8_t USBD_CDC_CfgDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIGN_END =
{
    0x09, USB_DESC_TYPE_CONFIGURATION,
    LOBYTE(USB_CDC_CONFIG_DESC_SIZ), HIBYTE(USB_CDC_CONFIG_DESC_SIZ),
    0x02, 0x01, 0x00,
#if (USBD_SELF_POWERED == 1U)
    0xC0,
#else
    0x80,
#endif
    USBD_MAX_POWER,

    0x09, USB_DESC_TYPE_INTERFACE, 0x00, 0x00, 0x01,
    0x02, 0x02, 0x01, 0x00,

    0x05, 0x24, 0x00, 0x10, 0x01,
    0x05, 0x24, 0x01, 0x00, 0x01,
    0x04, 0x24, 0x02, 0x02,
    0x05, 0x24, 0x06, 0x00, 0x01,

    0x07, USB_DESC_TYPE_ENDPOINT, CDC_CMD_EP, 0x03,
    LOBYTE(CDC_CMD_PACKET_SIZE), HIBYTE(CDC_CMD_PACKET_SIZE),
    CDC_FS_BINTERVAL,

    0x09, USB_DESC_TYPE_INTERFACE, 0x01, 0x00, 0x02,
    0x0A, 0x00, 0x00, 0x00,

    0x07, USB_DESC_TYPE_ENDPOINT, CDC_OUT_EP, 0x02,
    LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
    0x00,

    0x07, USB_DESC_TYPE_ENDPOINT, CDC_IN_EP, 0x02,
    LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE), HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
    0x00
};
#endif

static uint8_t CDCInEpAdd = CDC_IN_EP;
static uint8_t CDCOutEpAdd = CDC_OUT_EP;
static uint8_t CDCCmdEpAdd = CDC_CMD_EP;

static uint8_t USBD_CDC_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USBD_CDC_HandleTypeDef *hcdc;

    hcdc = (USBD_CDC_HandleTypeDef *)USBD_malloc(sizeof(USBD_CDC_HandleTypeDef));
    if (hcdc == NULL)
    {
        pdev->pClassDataCmsit[pdev->classId] = NULL;
        return (uint8_t)USBD_EMEM;
    }

    (void)USBD_memset(hcdc, 0, sizeof(USBD_CDC_HandleTypeDef));
    pdev->pClassDataCmsit[pdev->classId] = (void *)hcdc;
    pdev->pClassData = pdev->pClassDataCmsit[pdev->classId];

#ifdef USE_USBD_COMPOSITE
    CDCInEpAdd  = USBD_CoreGetEPAdd(pdev, USBD_EP_IN,  USBD_EP_TYPE_BULK, (uint8_t)pdev->classId);
    CDCOutEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_OUT, USBD_EP_TYPE_BULK, (uint8_t)pdev->classId);
    CDCCmdEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_IN,  USBD_EP_TYPE_INTR, (uint8_t)pdev->classId);
#endif

    (void)USBD_LL_OpenEP(pdev, CDCInEpAdd, USBD_EP_TYPE_BULK, CDC_DATA_FS_IN_PACKET_SIZE);
    pdev->ep_in[CDCInEpAdd & 0xFU].is_used = 1U;

    (void)USBD_LL_OpenEP(pdev, CDCOutEpAdd, USBD_EP_TYPE_BULK, CDC_DATA_FS_OUT_PACKET_SIZE);
    pdev->ep_out[CDCOutEpAdd & 0xFU].is_used = 1U;

    pdev->ep_in[CDCCmdEpAdd & 0xFU].bInterval = CDC_FS_BINTERVAL;
    (void)USBD_LL_OpenEP(pdev, CDCCmdEpAdd, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);
    pdev->ep_in[CDCCmdEpAdd & 0xFU].is_used = 1U;

    hcdc->RxBuffer = NULL;

    ((USBD_CDC_ItfTypeDef *)pdev->pUserData[pdev->classId])->Init();

    hcdc->TxState = 0U;
    hcdc->RxState = 0U;

    if (hcdc->RxBuffer == NULL)
    {
        return (uint8_t)USBD_EMEM;
    }

    (void)USBD_LL_PrepareReceive(pdev, CDCOutEpAdd, hcdc->RxBuffer, CDC_DATA_FS_OUT_PACKET_SIZE);

    return (uint8_t)USBD_OK;
}

static uint8_t USBD_CDC_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;

#ifdef USE_USBD_COMPOSITE
    CDCInEpAdd  = USBD_CoreGetEPAdd(pdev, USBD_EP_IN,  USBD_EP_TYPE_BULK, (uint8_t)pdev->classId);
    CDCOutEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_OUT, USBD_EP_TYPE_BULK, (uint8_t)pdev->classId);
    CDCCmdEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_IN,  USBD_EP_TYPE_INTR, (uint8_t)pdev->classId);
#endif

    (void)USBD_LL_CloseEP(pdev, CDCInEpAdd);
    pdev->ep_in[CDCInEpAdd & 0xFU].is_used = 0U;

    (void)USBD_LL_CloseEP(pdev, CDCOutEpAdd);
    pdev->ep_out[CDCOutEpAdd & 0xFU].is_used = 0U;

    (void)USBD_LL_CloseEP(pdev, CDCCmdEpAdd);
    pdev->ep_in[CDCCmdEpAdd & 0xFU].is_used = 0U;
    pdev->ep_in[CDCCmdEpAdd & 0xFU].bInterval = 0U;

    if (pdev->pClassDataCmsit[pdev->classId] != NULL)
    {
        ((USBD_CDC_ItfTypeDef *)pdev->pUserData[pdev->classId])->DeInit();
        (void)USBD_free(pdev->pClassDataCmsit[pdev->classId]);
        pdev->pClassDataCmsit[pdev->classId] = NULL;
        pdev->pClassData = NULL;
    }

    return (uint8_t)USBD_OK;
}

static uint8_t USBD_CDC_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];
    uint16_t len;
    uint8_t ifalt = 0U;
    uint16_t status_info = 0U;
    USBD_StatusTypeDef ret = USBD_OK;

    if (hcdc == NULL)
    {
        return (uint8_t)USBD_FAIL;
    }

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
    case USB_REQ_TYPE_CLASS:
        if (req->wLength != 0U)
        {
            if ((req->bmRequest & 0x80U) != 0U)
            {
                ((USBD_CDC_ItfTypeDef *)pdev->pUserData[pdev->classId])->Control(
                    req->bRequest, (uint8_t *)hcdc->data, req->wLength);
                len = MIN(CDC_REQ_MAX_DATA_SIZE, req->wLength);
                (void)USBD_CtlSendData(pdev, (uint8_t *)hcdc->data, len);
            }
            else
            {
                hcdc->CmdOpCode = req->bRequest;
                hcdc->CmdLength = (uint8_t)MIN(req->wLength, USB_MAX_EP0_SIZE);
                (void)USBD_CtlPrepareRx(pdev, (uint8_t *)hcdc->data, hcdc->CmdLength);
            }
        }
        else
        {
            ((USBD_CDC_ItfTypeDef *)pdev->pUserData[pdev->classId])->Control(
                req->bRequest, (uint8_t *)req, 0U);
        }
        break;

    case USB_REQ_TYPE_STANDARD:
        switch (req->bRequest)
        {
        case USB_REQ_GET_STATUS:
            if (pdev->dev_state == USBD_STATE_CONFIGURED)
            {
                (void)USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
            }
            else
            {
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
            }
            break;

        case USB_REQ_GET_INTERFACE:
            if (pdev->dev_state == USBD_STATE_CONFIGURED)
            {
                (void)USBD_CtlSendData(pdev, &ifalt, 1U);
            }
            else
            {
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
            }
            break;

        case USB_REQ_SET_INTERFACE:
            if (pdev->dev_state != USBD_STATE_CONFIGURED)
            {
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
            }
            break;

        case USB_REQ_CLEAR_FEATURE:
            break;

        default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
        }
        break;

    default:
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
        break;
    }

    return (uint8_t)ret;
}

static uint8_t USBD_CDC_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    USBD_CDC_HandleTypeDef *hcdc;
    PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef *)pdev->pData;

    if (pdev->pClassDataCmsit[pdev->classId] == NULL)
    {
        return (uint8_t)USBD_FAIL;
    }

    hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

    if ((pdev->ep_in[epnum & 0xFU].total_length > 0U) &&
        ((pdev->ep_in[epnum & 0xFU].total_length % hpcd->IN_ep[epnum & 0xFU].maxpacket) == 0U))
    {
        pdev->ep_in[epnum & 0xFU].total_length = 0U;
        (void)USBD_LL_Transmit(pdev, epnum, NULL, 0U);
    }
    else
    {
        hcdc->TxState = 0U;
        if (((USBD_CDC_ItfTypeDef *)pdev->pUserData[pdev->classId])->TransmitCplt != NULL)
        {
            ((USBD_CDC_ItfTypeDef *)pdev->pUserData[pdev->classId])->TransmitCplt(
                hcdc->TxBuffer, &hcdc->TxLength, epnum);
        }
    }

    return (uint8_t)USBD_OK;
}

static uint8_t USBD_CDC_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

    if (pdev->pClassDataCmsit[pdev->classId] == NULL)
    {
        return (uint8_t)USBD_FAIL;
    }

    hcdc->RxLength = USBD_LL_GetRxDataSize(pdev, epnum);
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData[pdev->classId])->Receive(hcdc->RxBuffer, &hcdc->RxLength);

    return (uint8_t)USBD_OK;
}

static uint8_t USBD_CDC_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

    if (hcdc == NULL)
    {
        return (uint8_t)USBD_FAIL;
    }

    if ((pdev->pUserData[pdev->classId] != NULL) && (hcdc->CmdOpCode != 0xFFU))
    {
        ((USBD_CDC_ItfTypeDef *)pdev->pUserData[pdev->classId])->Control(
            hcdc->CmdOpCode, (uint8_t *)hcdc->data, (uint16_t)hcdc->CmdLength);
        hcdc->CmdOpCode = 0xFFU;
    }

    return (uint8_t)USBD_OK;
}

#ifndef USE_USBD_COMPOSITE
static uint8_t *USBD_CDC_GetFSCfgDesc(uint16_t *length)
{
    *length = (uint16_t)sizeof(USBD_CDC_CfgDesc);
    return USBD_CDC_CfgDesc;
}

static uint8_t *USBD_CDC_GetHSCfgDesc(uint16_t *length)
{
    *length = (uint16_t)sizeof(USBD_CDC_CfgDesc);
    return USBD_CDC_CfgDesc;
}

static uint8_t *USBD_CDC_GetOtherSpeedCfgDesc(uint16_t *length)
{
    *length = (uint16_t)sizeof(USBD_CDC_CfgDesc);
    return USBD_CDC_CfgDesc;
}

static uint8_t *USBD_CDC_GetDeviceQualifierDescriptor(uint16_t *length)
{
    *length = (uint16_t)sizeof(USBD_CDC_DeviceQualifierDesc);
    return USBD_CDC_DeviceQualifierDesc;
}
#endif

uint8_t USBD_CDC_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_CDC_ItfTypeDef *fops)
{
    if (fops == NULL)
    {
        return (uint8_t)USBD_FAIL;
    }
    pdev->pUserData[pdev->classId] = fops;
    return (uint8_t)USBD_OK;
}

#ifdef USE_USBD_COMPOSITE
uint8_t USBD_CDC_SetTxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff, uint32_t length, uint8_t ClassId)
{
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassDataCmsit[ClassId];
#else
uint8_t USBD_CDC_SetTxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff, uint32_t length)
{
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];
#endif
    if (hcdc == NULL)
    {
        return (uint8_t)USBD_FAIL;
    }
    hcdc->TxBuffer = pbuff;
    hcdc->TxLength = length;
    return (uint8_t)USBD_OK;
}

uint8_t USBD_CDC_SetRxBuffer(USBD_HandleTypeDef *pdev, uint8_t *pbuff)
{
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];
    if (hcdc == NULL)
    {
        return (uint8_t)USBD_FAIL;
    }
    hcdc->RxBuffer = pbuff;
    return (uint8_t)USBD_OK;
}

#ifdef USE_USBD_COMPOSITE
uint8_t USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev, uint8_t ClassId)
{
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassDataCmsit[ClassId];
#else
uint8_t USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev)
{
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];
#endif
    USBD_StatusTypeDef ret = USBD_BUSY;

#ifdef USE_USBD_COMPOSITE
    CDCInEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_IN, USBD_EP_TYPE_BULK, ClassId);
#endif

    if (hcdc == NULL)
    {
        return (uint8_t)USBD_FAIL;
    }

    if (hcdc->TxState == 0U)
    {
        hcdc->TxState = 1U;
        pdev->ep_in[CDCInEpAdd & 0xFU].total_length = hcdc->TxLength;
        (void)USBD_LL_Transmit(pdev, CDCInEpAdd, hcdc->TxBuffer, hcdc->TxLength);
        ret = USBD_OK;
    }

    return (uint8_t)ret;
}

uint8_t USBD_CDC_ReceivePacket(USBD_HandleTypeDef *pdev)
{
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

#ifdef USE_USBD_COMPOSITE
    CDCOutEpAdd = USBD_CoreGetEPAdd(pdev, USBD_EP_OUT, USBD_EP_TYPE_BULK, (uint8_t)pdev->classId);
#endif

    if (pdev->pClassDataCmsit[pdev->classId] == NULL)
    {
        return (uint8_t)USBD_FAIL;
    }

    (void)USBD_LL_PrepareReceive(pdev, CDCOutEpAdd, hcdc->RxBuffer, CDC_DATA_FS_OUT_PACKET_SIZE);
    return (uint8_t)USBD_OK;
}
