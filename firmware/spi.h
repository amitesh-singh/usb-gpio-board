/*
 *  GPIO-12 board firmware
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

#ifndef __GPIO12_SPI_H_
#define __GPIO12_SPI_H_

#define MOSI PB3
#define MISO PB4
#define SCK PB5
#define SS PB3

void spi_init()
{
   DDRB |= (1 << MOSI) | (1 << SCK) | (1 << SS);
   DDRB &= ~(1 << MISO); //set MISO at input
   SPCR |= (1 << SPE) | (1 << MSTR) | (1 << SPR0); //f_ck/16 = 1Mhz
}

unsigned char spi_send(unsigned char data)
{
   SPDR = data;
   while (!(SPSR & (1 << SPIF)))
     {
        ;
     }

   return SPDR;
}

void spi_end()
{
   SPCR = 0;
}

#endif /* __GPIO12_SPI_H_ */
