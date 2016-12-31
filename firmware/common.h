/*
 *  GPIO-12 driver
 *
 *  (C) Amitesh Singh <singh.amitesh@gmail.com>, 2016
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMMON_H_
#define COMMON_H_

#ifdef AVR_USB_FIRMWARE
#include <stdint.h>
#endif

typedef struct __attribute__((__packed__)) _gpio_info
{
   uint8_t no;
   uint8_t val;
} gpio_info;

typedef struct __attribute__((__packed__)) _pktheader
{
   uint8_t command;
   gpio_info gpio;
} pktheader;

typedef enum _command
{
   BOARD_INIT,
   BOARD_RESET,
   GPIO_INPUT,
   GPIO_OUTPUT,
   GPIO_READ,
   GPIO_WRITE,
//   SPI_INIT,
//   SPI_DATA,
//   SPI_END,
} command;


#endif /* COMMON_H_ */
