/**
  ******************************************************************************
  * @file    usbd_conf.h
  * @author  MCD Application Team
  * @brief   General low level driver configuration
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef __USBD_CONF_H
#define __USBD_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USBD_MAX_NUM_INTERFACES     1U
#define USBD_MAX_NUM_CONFIGURATION  1U
#define USBD_MAX_STR_DESC_SIZ       0x100U
#define USBD_SELF_POWERED           1U
#define USBD_DEBUG_LEVEL            0U
#define USBD_SUPPORT_USER_STRING_DESC 0U

#define USBD_malloc         (void *)USBD_static_malloc
#define USBD_free           USBD_static_free
#define USBD_memset         memset
#define USBD_memcpy         memcpy
#define USBD_Delay          HAL_Delay

#if (USBD_DEBUG_LEVEL > 0U)
#define USBD_UsrLog(...)    do { printf(__VA_ARGS__); printf("\n"); } while(0)
#else
#define USBD_UsrLog(...)    do {} while(0)
#endif

#if (USBD_DEBUG_LEVEL > 1U)
#define USBD_ErrLog(...)    do { printf("ERROR: "); printf(__VA_ARGS__); printf("\n"); } while(0)
#else
#define USBD_ErrLog(...)    do {} while(0)
#endif

#if (USBD_DEBUG_LEVEL > 2U)
#define USBD_DbgLog(...)    do { printf("DEBUG: "); printf(__VA_ARGS__); printf("\n"); } while(0)
#else
#define USBD_DbgLog(...)    do {} while(0)
#endif

void *USBD_static_malloc(uint32_t size);
void USBD_static_free(void *p);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CONF_H */