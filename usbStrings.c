/*
 * This file is part of the m032HidBootloader distribution (https://github.com/nimo-labs/m032HidBootloader).
 * Copyright (c) 2021 Nimolabs Ltd. www.nimo.uk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>

#define DESC_STRING         0x03ul

/*!<USB Vendor String Descriptor */
uint8_t gu8VendorStringDesc[] =
{
    26,
    DESC_STRING,
    'N', 0, 'i', 0, 'm', 0, 'o', 0, 'L', 0, 'a', 0, 'b', 0, 's', 0, ' ', 0, 'L', 0, 't', 0, 'd', 0
};

/*!<USB Product String Descriptor */
uint8_t gu8ProductStringDesc[] =
{
    32,             /* bLength          */
    DESC_STRING,    /* bDescriptorType  */
    'H', 0, 'I', 0, 'D', 0, ' ', 0, 'B', 0, 'o', 0, 'o', 0, 't', 0, ' ', 0, 'L', 0, 'o', 0, 'a', 0, 'd', 0, 'e', 0, 'r', 0
};