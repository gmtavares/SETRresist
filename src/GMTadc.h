/**
 * \file GMTadc.h
 * 
 * \brief ADC Base functions
 * 
 * \version 1.0
 * 
 * \date 05-07-2023
 * 
 * \author Gonçalo Tavares
*/
#ifndef GMTADC_H_
#define GMTADC_H_

#include <zephyr/device.h>          /* for device_is_ready() and device structure */
#include <zephyr/devicetree.h>	    /* for DT_NODELABEL() */
#include <zephyr/drivers/gpio.h>    /* for GPIO API*/
#include <zephyr/drivers/adc.h>     /* for ADC API*/
#include <hal/nrf_saadc.h>
/*******************************/
/*ADC definitions and includes*/
#define ADC_RESOLUTION 10
#define NUM_CHANNELS 4
#define ADC_GAIN ADC_GAIN_1_4
#define ADC_REFERENCE ADC_REF_VDD_1_4
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40)

#define MEM_SIZE 10 /**< Number of Data Elements to be saved */

#define BUFFER_SIZE 1

//extern struct adc_channel_values; // for rtdb
//extern struct adc_channel_values ADC_DB[4]; // for rtdb

/* ADC channels configuration */
static const struct adc_channel_cfg channel_cfg[NUM_CHANNELS] = {
        {.gain = ADC_GAIN, .reference = ADC_REFERENCE, .acquisition_time = ADC_ACQUISITION_TIME, .channel_id = 0, .input_positive = NRF_SAADC_INPUT_AIN0},
        {.gain = ADC_GAIN, .reference = ADC_REFERENCE, .acquisition_time = ADC_ACQUISITION_TIME, .channel_id = 1, .input_positive = NRF_SAADC_INPUT_AIN1},
        {.gain = ADC_GAIN, .reference = ADC_REFERENCE, .acquisition_time = ADC_ACQUISITION_TIME, .channel_id = 2, .input_positive = NRF_SAADC_INPUT_AIN2},
        {.gain = ADC_GAIN, .reference = ADC_REFERENCE, .acquisition_time = ADC_ACQUISITION_TIME, .channel_id = 3, .input_positive = NRF_SAADC_INPUT_AIN3},
};

static uint16_t adc_sample_buffer[BUFFER_SIZE];

#define ADC_NODE DT_NODELABEL(adc)  

/** \brief ADC init
 * 
 * This function initializes the ADCs using the primitive function from the library
 * 
 */
void adc_init(void);

/** \brief ADC sample
 * 
 * This function takes one sample using the ADC
 * 
 * \param cid Channel ID for which the sample is to be taken
 * \return 0 on success, negative error code on failure
 */
int adc_sample(int cid);

/** \brief ADC collect
 * 
 * Collects the readings from the ADC during each cycle and saves them to the RTDB.
 * 
 * 
 */
int adc_collect();

/** \brief ADC print
 * 
 * Prints results of ADC readings
 * 
 * 
 */
void adc_print();

#endif /* GMTADC_H_ */