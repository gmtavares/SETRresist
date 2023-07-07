/**
 * \file GMTadc.c
 * 
 * \brief ADC Base code
 * 
 * \version 1.0
 * 
 * \date 05-07-2023
 * 
 * \author Gonçalo Tavares 
*/

#include <zephyr/kernel.h>          /* for k_msleep() */
#include <zephyr/device.h>          /* for device_is_ready() and device structure */
#include <zephyr/devicetree.h>	    /* for DT_NODELABEL() */
#include <zephyr/drivers/gpio.h>    /* for GPIO API*/
#include <zephyr/drivers/adc.h>     /* for ADC API*/
#include <zephyr/sys/printk.h>      /* for printk()*/
#include <hal/nrf_saadc.h>
#include <string.h>
#include <stdio.h>
#include "GMTadc.h"
#include "rtdb.h"
const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);	

void adc_init(void) 
{
    /* It is recommended to calibrate the SAADC at least once before use, and whenever the ambient temperature has changed by more than 10 °C */
	NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
    printk("\n\r ADC SETUP PROCESS\n\r");
	printk(" Reads an analog input connected to AN 1-4 and stores the raw and mV value \n\r");
	printk(" *** ASSURE THAT ANx IS BETWEEN [0...3V]\n\r");
    int err;
	/* For Cycle for the setup of the ADCs*/
    for (int i = 0; i < NUM_CHANNELS; i++) {
        err = adc_channel_setup(adc_dev, &channel_cfg[i]);
        if (err) {
            printk("adc_channel_setup() for channel %d failed with error code %d\n", i, err);
        }
    }
}

/* Takes one sample */
int adc_sample(int cid)
{		
	int ret;

	const struct adc_sequence sequence = {
		.channels = BIT(cid), /*ADC_CHANNEL_ID,*/
		.buffer = adc_sample_buffer,
		.buffer_size = sizeof(adc_sample_buffer),
		.resolution = ADC_RESOLUTION,
	};

	if (adc_dev == NULL) {
            printk("adc_sample(): error, must bind to adc first \n\r");
            return -1;
	}

	ret = adc_read(adc_dev, &sequence);
	if (ret) {
            printk("adc_read() failed with code %d\n", ret);
	}	

	return ret;
}

int adc_collect()
{
    int err = 0;
    for(int i = 0; i < NUM_CHANNELS; i++) {
		err=adc_sample(i);
		if(err) {
			printk("adc_sample() for adc %d failed with errocode %d\n\r",i,err);
            return err;
		}
        else {
            adc_channel_values.original_values[i] = adc_sample_buffer[0];
            adc_channel_values.converted_values[i] = (uint16_t)(1000 * adc_sample_buffer[0] * ((float)3 / 1023));
        }
/*        else {
            ADC_DB[i].actual_value = adc_sample_buffer[0];
            ADC_DB[i].converted_value = (uint16_t)(1000 * adc_sample_buffer[0] * ((float)3 / 1023));
        }*/
	}
    return err;
}

void adc_print()
{
    for(int i = 0; i < NUM_CHANNELS; i++) {
		if(adc_sample_buffer[0] > 1023) {
			printk("adc %d reading out of rang(value is %u)\n\r",i,adc_channel_values.original_values[i]);
		}
		else {
			/* ADC is set to use gain of 1/and reference VDD/4, so inpurange is 0...VDD (3 V), with 1bit resolution */
			printk("adc %d reading: %4u mV: \n\r",i,adc_channel_values.converted_values[i]);
			}
    }
}