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

/*
 *  D2(D+) and D4(D-) are used for vusb  and PB6 and PB6 are for oscillators
 *  then 12 GPIOs are only left for general purpose.
 *  PD0  - PD7 (excluding PD2/D+ and PD4/D - ) = 6
 *	PB0  - PB5 (excluding PB6 and PB7)  = 6
 */

#include <avr/io.h>
#include <avr/wdt.h>        /* Watch dog */
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

#define AVR_USB_FIRMWARE
#include "common.h"

static pktheader pkt_syn, pkt_ack;

static inline uint8_t _adjust_gpio(uint8_t no)
{
   if (no == 3) no = 4;
   else if (no == 4) no = 6;
   else if (no == 5) no = 7;
   else if (no == 6) no = 8;
   return no;
}

static void
_gpio_init(uint8_t no, uint8_t input)
{
   switch (no)
     {
      case 1 ... 6:
         //some adjustments since PD2 and PD4 are not available.
         if (no > 2)
           no = _adjust_gpio(no);

         if (!input)
           DDRD |= (1 << (no - 1));
         else
           DDRD &= ~(1 << (no - 1));
         break;
      case 7 ... 12:
         if (!input)
           DDRB |= (1 << (no - 7));
         else
           DDRB &= ~(1 << (no - 7));
         break;
      default:
         break;
     }
}

static void
_gpio_access(uint8_t no, uint8_t write, uint8_t *val)
{
   switch(no)
     {
      case 1 ... 6:
         if (no > 2)
           no = _adjust_gpio(no);

         if (write)
           {
              if (*val)
                PORTD |= (1 << (no - 1));
              else
                PORTD &= ~(1 << (no - 1));
           }
         else
           {
              if (bit_is_clear(PIND, (no - 1)))
                *val = 0;
              else
                *val = 1;
           }
         break;
      case 7 ... 12:
         if (write)
           {
              if  (*val)
                PORTB |= (1 << (no - 7));
              else
                PORTB &= ~(1 << (no - 7));
           }
         else
           {
              if (bit_is_clear(PINB, (no - 7)))
                *val = 0;
              else
                *val = 1;
           }
         break;
      default:
         break;
     }
}

usbMsgLen_t
usbFunctionSetup(uchar data[8])
{
   usbRequest_t *rq = (void *)data;
   uint8_t len = 0;

   pkt_syn.command = pkt_ack.command = rq->bRequest;

   switch(pkt_syn.command)
     {
      case BOARD_INIT:
         //do board init stuffs,
         len = 1;
         //blink leds etcs ? we could use some port for blinking? not sure?
         break;

      case BOARD_RESET:
         while(1); // watchdog will reset the board.
         break;

      case GPIO_INPUT:
         pkt_syn.gpio.no =  pkt_ack.gpio.no = rq->wValue.bytes[0];
         _gpio_init(pkt_syn.gpio.no, 1);
         len = 2;
         break;

      case GPIO_OUTPUT:
         pkt_syn.gpio.no =  pkt_ack.gpio.no = rq->wValue.bytes[0];
         _gpio_init(pkt_syn.gpio.no, 0);
         len = 2;
         break;

      case GPIO_READ:
         pkt_syn.gpio.no =  pkt_ack.gpio.no = rq->wValue.bytes[0];
         _gpio_access(pkt_syn.gpio.no, 0, &pkt_ack.gpio.val);
         len = 3;
         break;

      case GPIO_WRITE:
         pkt_syn.gpio.no =  pkt_ack.gpio.no = rq->wValue.bytes[0];
         pkt_syn.gpio.val = pkt_ack.gpio.val = rq->wValue.bytes[1];
         _gpio_access(pkt_syn.gpio.no, 1, &pkt_ack.gpio.val);

         len = 3;
         break;

      default:
         break;
     }

   usbMsgPtr = (unsigned char *)(&pkt_ack);

   return len; // should not get here
}

int __attribute__((noreturn))
main(void)
{
   uchar i = 0;

   wdt_enable(WDTO_1S);
   usbInit();
   usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */

   while(--i)
     {             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
     }
   usbDeviceConnect();
   sei();
   while(1)
     {                /* main event loop */
        wdt_reset();
        usbPoll();
     }
}
