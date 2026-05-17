#include "usbd_composite_builder.h"

#ifdef USE_USBD_COMPOSITE

static uint8_t *USBD_CMPSIT_GetFSCfgDesc(uint16_t *length);
static uint8_t *USBD_CMPSIT_GetOtherSpeedCfgDesc(uint16_t *length);
static uint8_t *USBD_CMPSIT_GetDeviceQualifierDescriptor(uint16_t *length);

USBD_ClassTypeDef USBD_CMPSIT =
{
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL,
    USBD_CMPSIT_GetFSCfgDesc,
    USBD_CMPSIT_GetOtherSpeedCfgDesc,
    USBD_CMPSIT_GetDeviceQualifierDescriptor,
#if (USBD_SUPPORT_USER_STRING_DESC == 1U)
    NULL,
#endif
};

__ALIGN_BEGIN static uint8_t USBD_CMPSIT_FSCfgDesc[USBD_CMPST_MAX_CONFDESC_SZ] __ALIGN_END = {0};
static uint8_t *pCmpstFSConfDesc = USBD_CMPSIT_FSCfgDesc;
static __IO uint32_t CurrFSConfDescSz = 0U;

__ALIGN_BEGIN static uint8_t USBD_CMPSIT_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
    USB_LEN_DEV_QUALIFIER_DESC, USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00, 0x02, 0xEF, 0x02, 0x01, 0x40, 0x01, 0x00,
};

uint8_t USBD_CMPSIT_AddClass(USBD_HandleTypeDef *pdev,
                              USBD_ClassTypeDef *pclass,
                              USBD_CompositeClassTypeDef class,
                              uint8_t cfgidx)
{
    if ((pdev->classId < USBD_MAX_SUPPORTED_CLASS) &&
        (pdev->tclasslist[pdev->classId].Active == 0U))
    {
        pdev->pClass[pdev->classId] = pclass;
        pdev->tclasslist[pdev->classId].ClassId = pdev->classId;
        pdev->tclasslist[pdev->classId].Active = 1U;
        pdev->tclasslist[pdev->classId].ClassType = class;

        if (USBD_CMPSIT_AddToConfDesc(pdev) != (uint8_t)USBD_OK)
            return (uint8_t)USBD_FAIL;
    }
    (void)cfgidx;
    return (uint8_t)USBD_OK;
}

static uint8_t USBD_CMPSIT_FindFreeIFNbr(USBD_HandleTypeDef *pdev)
{
    uint8_t idx = 0U;
    for (uint32_t i = 0U; i < pdev->NumClasses; i++)
        idx += (uint8_t)pdev->tclasslist[i].NumIf;
    return idx;
}

uint8_t USBD_CMPSIT_AddToConfDesc(USBD_HandleTypeDef *pdev)
{
    uint8_t *pConf = pCmpstFSConfDesc;
    uint32_t Sze;

    if (pdev->classId == 0U)
    {
        USBD_ConfigDescTypeDef *ptr = (USBD_ConfigDescTypeDef *)pConf;
        ptr->bLength = (uint8_t)sizeof(USBD_ConfigDescTypeDef);
        ptr->bDescriptorType = USB_DESC_TYPE_CONFIGURATION;
        ptr->wTotalLength = 0U;
        ptr->bNumInterfaces = 0U;
        ptr->bConfigurationValue = 1U;
        ptr->iConfiguration = USBD_CONFIG_STR_DESC_IDX;
#if (USBD_SELF_POWERED == 1U)
        ptr->bmAttributes = 0xC0U;
#else
        ptr->bmAttributes = 0x80U;
#endif
        ptr->bMaxPower = USBD_MAX_POWER;
        CurrFSConfDescSz = sizeof(USBD_ConfigDescTypeDef);
    }

    Sze = CurrFSConfDescSz;

    switch (pdev->tclasslist[pdev->classId].ClassType)
    {
#if USBD_CMPSIT_ACTIVATE_HID == 1
    case CLASS_TYPE_HID:
    {
        uint8_t idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);

        pdev->tclasslist[pdev->classId].CurrPcktSze = HID_EPIN_SIZE;
        pdev->tclasslist[pdev->classId].NumIf = 1U;
        pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;
        pdev->tclasslist[pdev->classId].NumEps = 1U;

        uint8_t ep = pdev->tclasslist[pdev->classId].EpAdd[0];
        pdev->tclasslist[pdev->classId].Eps[0].add = ep;
        pdev->tclasslist[pdev->classId].Eps[0].type = USBD_EP_TYPE_INTR;
        pdev->tclasslist[pdev->classId].Eps[0].size = HID_EPIN_SIZE;
        pdev->tclasslist[pdev->classId].Eps[0].is_used = 1U;

        USBD_IfDescTypeDef *pIfDesc = (USBD_IfDescTypeDef *)(pConf + Sze);
        pIfDesc->bLength = sizeof(USBD_IfDescTypeDef);
        pIfDesc->bDescriptorType = USB_DESC_TYPE_INTERFACE;
        pIfDesc->bInterfaceNumber = idxIf;
        pIfDesc->bAlternateSetting = 0U;
        pIfDesc->bNumEndpoints = 1U;
        pIfDesc->bInterfaceClass = 0x03U;
        pIfDesc->bInterfaceSubClass = 0x00U;
        pIfDesc->bInterfaceProtocol = 0x00U;
        pIfDesc->iInterface = 0U;
        Sze += sizeof(USBD_IfDescTypeDef);

        USBD_HIDDescTypeDef *pHidDesc = (USBD_HIDDescTypeDef *)(pConf + Sze);
        pHidDesc->bLength = sizeof(USBD_HIDDescTypeDef);
        pHidDesc->bDescriptorType = HID_DESCRIPTOR_TYPE;
        pHidDesc->bcdHID = 0x0111U;
        pHidDesc->bCountryCode = 0x00U;
        pHidDesc->bNumDescriptors = 0x01U;
        pHidDesc->bHIDDescriptorType = 0x22U;
        pHidDesc->wItemLength = HID_DIGITIZER_REPORT_DESC_SIZE;
        Sze += sizeof(USBD_HIDDescTypeDef);

        USBD_EpDescTypeDef *pEpDesc = (USBD_EpDescTypeDef *)(pConf + Sze);
        pEpDesc->bLength = sizeof(USBD_EpDescTypeDef);
        pEpDesc->bDescriptorType = USB_DESC_TYPE_ENDPOINT;
        pEpDesc->bEndpointAddress = ep;
        pEpDesc->bmAttributes = USBD_EP_TYPE_INTR;
        pEpDesc->wMaxPacketSize = HID_EPIN_SIZE;
        pEpDesc->bInterval = HID_FS_BINTERVAL;
        Sze += sizeof(USBD_EpDescTypeDef);

        ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 1U;
        ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)Sze;
        CurrFSConfDescSz = Sze;
        break;
    }
#endif

#if USBD_CMPSIT_ACTIVATE_CDC == 1
    case CLASS_TYPE_CDC:
    {
        uint8_t idxIf = USBD_CMPSIT_FindFreeIFNbr(pdev);

        pdev->tclasslist[pdev->classId].CurrPcktSze = CDC_DATA_FS_MAX_PACKET_SIZE;
        pdev->tclasslist[pdev->classId].NumIf = 2U;
        pdev->tclasslist[pdev->classId].Ifs[0] = idxIf;
        pdev->tclasslist[pdev->classId].Ifs[1] = (uint8_t)(idxIf + 1U);
        pdev->tclasslist[pdev->classId].NumEps = 3U;

        uint8_t epIn = pdev->tclasslist[pdev->classId].EpAdd[0];
        uint8_t epOut = pdev->tclasslist[pdev->classId].EpAdd[1];
        uint8_t epCmd = pdev->tclasslist[pdev->classId].EpAdd[2];

        pdev->tclasslist[pdev->classId].Eps[0].add = epIn;
        pdev->tclasslist[pdev->classId].Eps[0].type = USBD_EP_TYPE_BULK;
        pdev->tclasslist[pdev->classId].Eps[0].size = CDC_DATA_FS_MAX_PACKET_SIZE;
        pdev->tclasslist[pdev->classId].Eps[0].is_used = 1U;

        pdev->tclasslist[pdev->classId].Eps[1].add = epOut;
        pdev->tclasslist[pdev->classId].Eps[1].type = USBD_EP_TYPE_BULK;
        pdev->tclasslist[pdev->classId].Eps[1].size = CDC_DATA_FS_MAX_PACKET_SIZE;
        pdev->tclasslist[pdev->classId].Eps[1].is_used = 1U;

        pdev->tclasslist[pdev->classId].Eps[2].add = epCmd;
        pdev->tclasslist[pdev->classId].Eps[2].type = USBD_EP_TYPE_INTR;
        pdev->tclasslist[pdev->classId].Eps[2].size = CDC_CMD_PACKET_SIZE;
        pdev->tclasslist[pdev->classId].Eps[2].is_used = 1U;

#if USBD_COMPOSITE_USE_IAD == 1
        {
            USBD_IadDescTypeDef *pIadDesc = (USBD_IadDescTypeDef *)(pConf + Sze);
            pIadDesc->bLength = sizeof(USBD_IadDescTypeDef);
            pIadDesc->bDescriptorType = USB_DESC_TYPE_IAD;
            pIadDesc->bFirstInterface = idxIf;
            pIadDesc->bInterfaceCount = 2U;
            pIadDesc->bFunctionClass = 0x02U;
            pIadDesc->bFunctionSubClass = 0x02U;
            pIadDesc->bFunctionProtocol = 0x01U;
            pIadDesc->iFunction = 0U;
            Sze += sizeof(USBD_IadDescTypeDef);
        }
#endif

        {
            USBD_IfDescTypeDef *pIfDesc = (USBD_IfDescTypeDef *)(pConf + Sze);
            pIfDesc->bLength = sizeof(USBD_IfDescTypeDef);
            pIfDesc->bDescriptorType = USB_DESC_TYPE_INTERFACE;
            pIfDesc->bInterfaceNumber = idxIf;
            pIfDesc->bAlternateSetting = 0U;
            pIfDesc->bNumEndpoints = 1U;
            pIfDesc->bInterfaceClass = 0x02U;
            pIfDesc->bInterfaceSubClass = 0x02U;
            pIfDesc->bInterfaceProtocol = 0x01U;
            pIfDesc->iInterface = 0U;
            Sze += sizeof(USBD_IfDescTypeDef);
        }

        pConf[Sze++] = 0x05; pConf[Sze++] = 0x24; pConf[Sze++] = 0x00;
        pConf[Sze++] = 0x10; pConf[Sze++] = 0x01;

        pConf[Sze++] = 0x05; pConf[Sze++] = 0x24; pConf[Sze++] = 0x01;
        pConf[Sze++] = 0x00; pConf[Sze++] = idxIf + 1U;

        pConf[Sze++] = 0x04; pConf[Sze++] = 0x24; pConf[Sze++] = 0x02;
        pConf[Sze++] = 0x02;

        pConf[Sze++] = 0x05; pConf[Sze++] = 0x24; pConf[Sze++] = 0x06;
        pConf[Sze++] = idxIf; pConf[Sze++] = idxIf + 1U;

        {
            USBD_EpDescTypeDef *pEpDesc = (USBD_EpDescTypeDef *)(pConf + Sze);
            pEpDesc->bLength = sizeof(USBD_EpDescTypeDef);
            pEpDesc->bDescriptorType = USB_DESC_TYPE_ENDPOINT;
            pEpDesc->bEndpointAddress = epCmd;
            pEpDesc->bmAttributes = USBD_EP_TYPE_INTR;
            pEpDesc->wMaxPacketSize = CDC_CMD_PACKET_SIZE;
            pEpDesc->bInterval = CDC_FS_BINTERVAL;
            Sze += sizeof(USBD_EpDescTypeDef);
        }

        {
            USBD_IfDescTypeDef *pIfDesc = (USBD_IfDescTypeDef *)(pConf + Sze);
            pIfDesc->bLength = sizeof(USBD_IfDescTypeDef);
            pIfDesc->bDescriptorType = USB_DESC_TYPE_INTERFACE;
            pIfDesc->bInterfaceNumber = idxIf + 1U;
            pIfDesc->bAlternateSetting = 0U;
            pIfDesc->bNumEndpoints = 2U;
            pIfDesc->bInterfaceClass = 0x0AU;
            pIfDesc->bInterfaceSubClass = 0U;
            pIfDesc->bInterfaceProtocol = 0U;
            pIfDesc->iInterface = 0U;
            Sze += sizeof(USBD_IfDescTypeDef);
        }

        {
            USBD_EpDescTypeDef *pEpDesc = (USBD_EpDescTypeDef *)(pConf + Sze);
            pEpDesc->bLength = sizeof(USBD_EpDescTypeDef);
            pEpDesc->bDescriptorType = USB_DESC_TYPE_ENDPOINT;
            pEpDesc->bEndpointAddress = epIn;
            pEpDesc->bmAttributes = USBD_EP_TYPE_BULK;
            pEpDesc->wMaxPacketSize = CDC_DATA_FS_MAX_PACKET_SIZE;
            pEpDesc->bInterval = 0U;
            Sze += sizeof(USBD_EpDescTypeDef);
        }

        {
            USBD_EpDescTypeDef *pEpDesc = (USBD_EpDescTypeDef *)(pConf + Sze);
            pEpDesc->bLength = sizeof(USBD_EpDescTypeDef);
            pEpDesc->bDescriptorType = USB_DESC_TYPE_ENDPOINT;
            pEpDesc->bEndpointAddress = epOut;
            pEpDesc->bmAttributes = USBD_EP_TYPE_BULK;
            pEpDesc->wMaxPacketSize = CDC_DATA_FS_MAX_PACKET_SIZE;
            pEpDesc->bInterval = 0U;
            Sze += sizeof(USBD_EpDescTypeDef);
        }

        ((USBD_ConfigDescTypeDef *)pConf)->bNumInterfaces += 2U;
        ((USBD_ConfigDescTypeDef *)pConf)->wTotalLength = (uint16_t)Sze;
        CurrFSConfDescSz = Sze;
        break;
    }
#endif

    default:
        break;
    }

    return (uint8_t)USBD_OK;
}

static uint8_t *USBD_CMPSIT_GetFSCfgDesc(uint16_t *length)
{
    *length = (uint16_t)CurrFSConfDescSz;
    return USBD_CMPSIT_FSCfgDesc;
}

static uint8_t *USBD_CMPSIT_GetOtherSpeedCfgDesc(uint16_t *length)
{
    *length = (uint16_t)CurrFSConfDescSz;
    return USBD_CMPSIT_FSCfgDesc;
}

static uint8_t *USBD_CMPSIT_GetDeviceQualifierDescriptor(uint16_t *length)
{
    *length = (uint16_t)sizeof(USBD_CMPSIT_DeviceQualifierDesc);
    return USBD_CMPSIT_DeviceQualifierDesc;
}

uint8_t USBD_CMPST_ClearConfDesc(USBD_HandleTypeDef *pdev)
{
    (void)pdev;
    CurrFSConfDescSz = 0U;
    return USBD_OK;
}

uint32_t USBD_CMPSIT_SetClassID(USBD_HandleTypeDef *pdev,
                                 USBD_CompositeClassTypeDef Class,
                                 uint32_t Instance)
{
    (void)Instance;
    for (uint32_t i = 0U; i < USBD_MAX_SUPPORTED_CLASS; i++)
        if (pdev->tclasslist[i].ClassType == Class)
            return i;
    return 0xFFFFFFFFU;
}

uint32_t USBD_CMPSIT_GetClassID(USBD_HandleTypeDef *pdev,
                                 USBD_CompositeClassTypeDef Class,
                                 uint32_t Instance)
{
    (void)pdev; (void)Instance;
    return 0xFFFFFFFFU;
}

#endif
