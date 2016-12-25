/*
 * D4 and D2 are used for vusb so we have 14 GPIOs left and PB6 and PB6 are for oscillators
 *  then 12 GPIOs are only left for general purpose.
 *  PD0  - PD7 (excluding PD2/D+ and PD4/D - ) = 6
 *	PB0  - PB5 (excluding PB6 and PB7)  = 6
 * Author: (C) Amitesh Singh <singh.amitesh@gmail.com>, 2016
 */

#include <avr/io.h>
#include <avr/wdt.h>        /* Watch dog */
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

#include "common.h"

static pktheader pkt_syn, pkt_ack;

static void
_gpio_init(uint8_t no, uint8_t input)
{
   switch (no)
     {
      case 1 ... 6:
         //some adjustments since PD2 and PD4 are not available.
         if (no == 3) no = 4;
         else if (no == 4) no = 6;
         else if (no == 5) no = 7;
         else if (no == 6) no = 8;

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
         if (no == 3) no = 4;
         else if (no == 4) no = 6;
         else if (no == 5) no = 7;
         else if (no == 6) no = 8;
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

   // pkt.gpio.val = gpio_val;
   switch(pkt_syn.command)
     {
      case BOARD_INIT:
         //do board init stuffs,
         len = 1;
         //blink leds etcs ? we could use some port for blinking? not sure?
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
