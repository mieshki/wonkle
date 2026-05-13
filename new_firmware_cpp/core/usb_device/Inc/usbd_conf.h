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

#include "stm32f4xx_hal.h"

/* Memory management — static pool for bare-metal (no heap) */
static uint32_t usbd_mem_pool[4]; /* 16 bytes, enough for HID handle */
#define USBD_malloc  static_malloc
#define USBD_free    static_free

static inline void* static_malloc(uint32_t size) {
    (void)size;
    return usbd_mem_pool;
}

static inline void static_free(void *ptr) {
    (void)ptr;
}

/*---------- -----------*/
#define USBD_CFG_MAX_NUM             1U
#define USBD_MAX_NUM_INTERFACES      1U
#define USBD_MAX_SUPPORTED_CLASS     1U
#define USBD_MAX_CLASS_ENDPOINTS     5U
#define USBD_MAX_CLASS_INTERFACES    5U

/* Debug level */
#define USBD_DEBUG_LEVEL             0U

/* User callback */
#define USBD_USER_REGISTER_CALLBACK  0U

/* LPM and BOS support */
#define USBD_LPM_ENABLED             0U
#define USBD_CLASS_BOS_ENABLED       0U

/* Support for user-defined string descriptors */
#define USBD_SUPPORT_USER_STRING_DESC 0U
#define USBD_CLASS_USER_STRING_DESC   0U

/* Assert */
#define USBD_ASSERT_PARAM(expr)      ((void)0U)

/* Debug macros */
#if (USBD_DEBUG_LEVEL > 0U)
#include <stdio.h>
#define USBD_ErrLog(...)
#else
#define USBD_ErrLog(...)
#endif /* USBD_DEBUG_LEVEL */

#endif /* __USBD_CONF_H */