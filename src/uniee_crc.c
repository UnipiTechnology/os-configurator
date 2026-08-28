/*
 *
 *
 * Copyright (c) 2021  Martin Triska triska@unipi.technology
 * Copyright (c) 2025  Frantisek Burian frantisek.burian@unipi.technology
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "uniee.h"
#include "uniee_crc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>



#define  CRC_INITIAL_VALUE 0xFFFF
#define  CRC_POLYNOM     0x1021

static uint16_t prv_crc_finish(uint16_t crc_value)
{
    uint16_t v;
    uint16_t xor_flag;

    for (int i = 0; i < 16; i++) {
      xor_flag= (crc_value & 0x8000) ? 1 : 0;
      crc_value = crc_value << 1;

      if (xor_flag)
        crc_value = crc_value ^ CRC_POLYNOM;
    }
    return crc_value;
}

static uint16_t prv_crc_step(uint16_t crc_value, unsigned short ch)
{
    //  Align test bit with leftmost bit of the message byte.
    uint16_t v = 0x80;

    for (int i=0; i<8; i++) {
      uint16_t xor_flag= (crc_value & 0x8000) ? 1 : 0;
      crc_value = crc_value << 1;

      if (ch & v) {
          // Append next bit of message to end of CRC if it is not zero.
          // The zero bit placed there by the shift above need not be
          // changed if the next bit of the message is zero.
          crc_value = crc_value + 1;
      }

      if (xor_flag)
        crc_value = crc_value ^ CRC_POLYNOM;

      // Align test bit with next bit of the message byte.
      v = v >> 1;
    }
    return crc_value;
}

uint16_t prv_compute_checksum(uint8_t *buff, size_t filesize)
{
   uint16_t crc_value = CRC_INITIAL_VALUE;

   for (int i=0; i < (filesize - sizeof(((uniee_bank_3_t*)0)->checksum)); i++)
     crc_value = prv_crc_step(crc_value, buff[i]);

   return prv_crc_finish(crc_value);
   /*
    * REFERENCE CRC TEST
   char* testdata = "123456789";

   while (*testdata != 0){
     prv_crc_step(*testdata);
     testdata++;
   }

  */
}

