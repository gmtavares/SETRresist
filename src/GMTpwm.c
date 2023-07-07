/**
 * \file GMTpwm.c
 * 
 * \brief PWM code
 * 
 * \version 1.0
 * 
 * \date 05-07-2023
 * 
 * \author Gonçalo Tavares 
*/

#include <zephyr/kernel.h>          /* for k_msleep() */
#include <zephyr/device.h>          /* for device_is_ready() and device structure */
#include <zephyr/devicetree.h>		/* for DT_NODELABEL() */
#include <zephyr/drivers/gpio.h>    /* for GPIO api*/
#include <zephyr/drivers/pwm.h>		/* For PWM api */
#include <zephyr/sys/printk.h>      /* for printk()*/
#include "GMTpwm.h"

void pwm_init(void) {
    int ret;
	if (!device_is_ready(pwm_led0.dev)) {
		printk("Error: PWM device %s is not ready\n", pwm_led0.dev->name);
		return;
	}

}