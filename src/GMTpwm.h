/**
 * \file GMTpwm.h
 * 
 * \brief PWM functions header
 * 
 * \version 1.0
 * 
 * \date 05-07-2023
 * 
 * \author Gonçalo Tavares
*/
#ifndef GMTPWM_H_
#define GMTPWM_H_

#define PWM_PERIOD 10000000 /* Value specified in ns */ 

/* Get node IDs for LED1 and pwm0, noting that LED1 is labeld led0 in DTS file. */ 
#define LED1_NODE DT_NODELABEL(led1) /**ID for LED1*/
#define PWM0_NODE DT_NODELABEL(pwm_led0) /**ID for pwm0*/

/* Now get the corresponding device pointer, pin number, configuration flags, ... */
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

/** \brief PWM init
 * 
 * This function initializes the PWM using the primitive function from the library
 * 
 */
void pwm_init(void);

#endif /* GMTPWM_H_ */