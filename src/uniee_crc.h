/*
 *
 * Copyright (c) 2021  Faster CZ, ondra@faster.cz
 *
 * SPDX-License-Identifier: LGPL-2.1+
 *
 */

#ifndef UNIEE_CRC_H_
#define UNIEE_CRC_H_

#include <stdint.h>

uint16_t prv_compute_checksum(uint8_t *buff, size_t filesize);

#endif /* UNIEE_CRC_H_*/
