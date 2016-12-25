/*
 * common.h
 *
 *  Created on: Dec 25, 2016
 *      Author: ami
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

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
	GPIO_INPUT,
	GPIO_OUTPUT,
	GPIO_READ,
	GPIO_WRITE,
} command;


#endif /* COMMON_H_ */
