/**
 * \file rtdb.c
 * 
 * \brief Real Time Database code
 * 
 * \version 1.0
 * 
 * \date 05-07-2023
 * 
 * \author Gonçalo Tavares 
*/

#include "rtdb.h"            
#include <inttypes.h>
#include <stdio.h>

struct adc_value_container adc_channel_values;

void RTDB_init() {
    // ADC DATABASE INITIALISATION
    
    for (int i = 0; i < NUM_CHANNELS; i++) {
        adc_channel_values.original_values[i] = 0;
        adc_channel_values.converted_values[i] = 0;
    }
    
    //


}