/**
 * @mainpage Input/Output Module with ADC and PWM
 *
 * @section intro_sec Introduction
 *
 * This documentation provides an overview of the input/output module implementation with ADC and PWM functionalities. The module allows reading analog inputs, generating PWM output, and receiving commands via UART for configuring the system.
 *
 * The main functionality of the module includes:
 * - Reading analog inputs periodically using the ADC driver.
 * - Generating PWM output using the PWM driver.
 * - Handling UART communication for receiving commands and changing thread periods.
 * - Printing system information and data.
 *
 * @section features_sec Key Features
 *
 * - Periodic reading of analog inputs and conversion to voltage values.
 * - Scaling voltage values to a desired range.
 * - PWM generation with configurable duty cycle and frequency.
 * - UART communication for command input and system configuration.
 *
 * @section dependencies_sec Dependencies
 *
 * - Zephyr kernel: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/kernel/services/index.html#kernel-services
 * - DeviceTree: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/guides/dts/api-usage.html#dt-from-c
 *
 * @section usage_sec Usage
 *
 * - Connect four potentiometers to the analog inputs of the board (AN3,4,28,29)
 * - Configure the system by setting up the ADC and PWM periods with the UART.
 * - The following command format must be followed:
 * - $TXYYYY&  (or $tXYYY&),
 *
 * @section author_sec Author
 *
 * - Name: Gonçalo Tavares
 * - Student ID: Mec. 93030
 * - Date: 05/07/2023
 */

