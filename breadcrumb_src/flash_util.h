/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef FLASH_UTIL_H__
#define FLASH_UTIL_H__

#include <stdint.h>

uint32_t readWordFlash(void);
void writeWordFlash(uint32_t word);

#endif /* FLASH_UTIL_H__ */