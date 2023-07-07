/**
 * \file rtdb.h
 * 
 * \brief Real Time Data Base Header
 * 
 * \version 1.0
 * 
 * \date 05-07-2023
 * 
 * \author Gonçalo Tavares 
*/


#ifndef RTDB_H_
#define RTDB_H_

#include <inttypes.h>
#include <stdint.h>
#include "GMTadc.h"


// INPUT VALUES

struct adc_value_container {
    uint16_t original_values[4];
    uint16_t converted_values[4];
};

extern struct adc_value_container adc_channel_values;

// OUTPUT VALUES



// FUNCTIONS
/** \brief RTDBinit()
 * 
 * Initialises the RTDB:
 * > ADC database is initialised with 0
 * 
 */
void RTDB_init();

#endif /* RTDB_H_ */
