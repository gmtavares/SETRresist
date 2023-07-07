/** \file main.c
 *  \brief Implementation of an input/output module with ADC and PWM, with command input via UART
 * 
 * 
 *  The system implements 4 Inputs (analog inputs) and 1 Output - PWM "DAC".
 *  In order to obtain those inputs and outputs, 2 periodic threads were implemented, 
 *  > 1 for reading the state of analog inputs;
 *  > 1 for writing the PWM output
 *  The communication via UART for command input to change periods of the aforementioned threads was also implemented.
 *  The command must be in the format: $TXYYYY&  (or $tXYYY&),
 *  where X can be O/o for the PWM thread or I/i for the analog input thread and YYYY are four digits of the time (in ms).
 *  
 * Base documentation:
 *  Zephyr kernel:  
 *      https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/kernel/services/index.html#kernel-services
 *      
 *  DeviceTree 
 *      Board DTS can be found in BUILD_DIR/zephyr/zephyr.dts
 *      https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/guides/dts/api-usage.html#dt-from-c  
 *
 *      HW info
 *      https://infocenter.nordicsemi.com/topic/struct_nrf52/struct/nrf52840.html
 *      Section: nRF52840 Product Specification -> Peripherals -> GPIO / GPIOTE
 * 
 *      Board specific HW info can be found in the nRF52840_DK_User_Guide_20201203. I/O pins available at pg 27
 *		
 * 
 *  \author Gonçalo Tavares, Mec. 93030
 *  \date 05/07/2023
 */

/*******************************/

#include <zephyr/kernel.h>          /* for k_msleep() */
#include <zephyr/device.h>          /* for device_is_ready() and device structure */
#include <zephyr/devicetree.h>      /* for DT_NODELABEL() */
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>    /* for GPIO API*/
#include <zephyr/timing/timing.h>   /* for timing services */
#include <zephyr/drivers/adc.h>     /* for ADC API*/
#include <zephyr/drivers/uart.h>    /* for UART*/
#include <zephyr/sys/printk.h>      /* for printk()*/
#include <zephyr/drivers/pwm.h>		/* For PWM api */
#include <zephyr/sys/util.h>
#include <inttypes.h>
#include <string.h>
#include "GMTadc.h"
#include "GMTpwm.h"
#include "rtdb.h"

/*******************************/

#define Receive_Buff_Size 10/**< Define the size of the UART receive buffer */
#define Stacksize 1024 /**< Size of the threads stack*/

/*******************************/
/* THREADS DEFINITION; SKELETON*/

/* Define each thread's priority*/
#define thread_print_prio 3/**< Priority of the print thread*/
#define thread_an_prio 3/**< Priority of the analog input thread*/
#define thread_pwm_prio 3/**< Priority of the PWM thread*/
#define thread_cmd_prio 4/**< Priority of the command thread*/


/* Define each thread's period (in ms) */
#define thread_print_period 1000 /**< Print thread static period */
volatile int thread_an_period = 1000;/**< Analog input thread period - can vary via UART*/
volatile int thread_pwm_period = 1000;/**< PWM thread Pediod*/
#define thread_cmd_period 1000/**< Command thread static period*/


/* Allocate stack for each thread */
K_THREAD_STACK_DEFINE(thread_print_stack, Stacksize);/**< Allocate stack for the print thread*/
K_THREAD_STACK_DEFINE(thread_an_stack, Stacksize);/**< Allocate stack for the analog input thread*/
K_THREAD_STACK_DEFINE(thread_pwm_stack, Stacksize);/**< Allocate stack for PWM thread*/
K_THREAD_STACK_DEFINE(thread_cmd_stack, Stacksize);/**< Allocate stack for the command thread*/

/* Creating variables for each thread's inf */
struct k_thread thread_print_data;/**< data of the print thread*/
struct k_thread thread_an_data;/**< data of the analog input thread*/
struct k_thread thread_pwm_data;/**< data of the pwm thread*/
struct k_thread thread_cmd_data;/**< data of the command thread*/

/* Creating task IDs */
k_tid_t thread_print_tid;/**< ID of the task of the print thread*/
k_tid_t thread_an_tid;/**< ID of the task of the analog inputs thread*/
k_tid_t thread_pwm_tid;/**< ID of the task of the PWM thread*/
k_tid_t thread_cmd_tid;/**< ID of the task of the command input*/

/* Thread prototypes */
void thread_print_code(void *argA , void *argB, void *argC);
void thread_an_code(void *argA , void *argB, void *argC);
void thread_pwm_code(void *argA , void *argB, void *argC);
void thread_cmd_code(void *argA , void *argB, void *argC);

/*******************************/
/*UART definitions*/

/** Global Variables (Shared Memory)*/
volatile int cmd = 0;/**< This variable indicates a command has been input when it's '1'*/
volatile int res = 1;

/**Internal variables*/
static unsigned char cmdLen = 0; 
int SOF_C = -1;
int EOF_C = -1;
static char cmdString[Receive_Buff_Size];

#define Receive_Buff_Size 10 /**< Define the size of the receive buffer*/
#define Receive_Timeout 100 /**< Define the UART timeout period*/
#define UART_NODE DT_NODELABEL(uart0) /**< UART node identifier*/

static uint8_t tx_buf[]= {""}; /**< Define the uart Tx that holds the content to be transmitted by the uart*/
static uint8_t rx_buf[Receive_Buff_Size] = {0}; /**< Define the Rx buffer*/

/*******************************/
/**Semaphore defining */
struct k_sem sem_rtdb_adc;

/*******************************/
/**Function prototyping */
void startup_config(void);
int CmdChar(unsigned char newChar);
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data);
void resetcmd(void);
int cmdProcess(void);


int ret;
int errorcount = 0;

/** Get the device pointer of the UART hardware */
const struct device *uart = DEVICE_DT_GET(UART_NODE);

/** Define the structure of possible inputs and outputs */
#define MAX_CMDSTRING_SIZE 10   /**< MAX COMMAND STRING SIZE */ 
#define SOF_SYM '$'             /**< START OF COMMAND SYMBOL */
#define EOF_SYM '&'             /**< END OF COMMAND SYMBOL */
#define EXIT_SUCCESS    0;      /**< SUCCESSFUL EXIT */
#define EMPTY_STRING   -1;      /**< EMPTY STRING */
#define STRING_FULL    -1;      /**< FULL STRING */
#define CMD_NOT_FOUND  -2;      /**< INVALID CMD */
#define WRONG_STR_FORMAT -3;    /**< WRONG FORMAT */

/** \brief Main Function
 * 
 * The main function creates the threads, configures and handles the inputs and outputs.
 *
 */
void main(void)
{

	startup_config(); // sets up the RTDB, configures input and output pins and ADC; UART; 

	/*Create the threads*/

	thread_print_tid = k_thread_create(&thread_print_data, thread_print_stack,
        K_THREAD_STACK_SIZEOF(thread_print_stack), thread_print_code,
        NULL, NULL, NULL, thread_print_prio, 0, K_NO_WAIT);

	thread_an_tid = k_thread_create(&thread_an_data, thread_an_stack,
        K_THREAD_STACK_SIZEOF(thread_an_stack), thread_an_code,
        NULL, NULL, NULL, thread_an_prio, 0, K_NO_WAIT);

	thread_pwm_tid = k_thread_create(&thread_pwm_data, thread_pwm_stack,
        K_THREAD_STACK_SIZEOF(thread_pwm_stack), thread_pwm_code,
        NULL, NULL, NULL, thread_pwm_prio, 0, K_NO_WAIT);

	thread_cmd_tid = k_thread_create(&thread_cmd_data, thread_cmd_stack,
        K_THREAD_STACK_SIZEOF(thread_cmd_stack), thread_cmd_code,
        NULL, NULL, NULL, thread_cmd_prio, 0, K_NO_WAIT);
		
	resetcmd();	
	return;
}

/** \brief UART callback function
 * 
 * This function keeps the values inserted by the user in a buffer until "enter" is pressed
 * 
 * 
 */
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data){
	int res = 1;
	switch (evt->type) {

	case UART_RX_RDY:
        if(evt->data.rx.len > 0){
            if(evt->data.rx.buf[evt->data.rx.offset] == '\r') { // Wait until "enter" is pressed
                cmd = 1; // Indentify a command has been input
                break;
            }
            res = CmdChar(evt->data.rx.buf[evt->data.rx.offset]);
        }
        break;

	case UART_RX_DISABLED:
		uart_rx_enable(dev ,rx_buf,sizeof rx_buf,Receive_Timeout);
		break;
		
	default:
		break;
    }
}
/** \brief This string adds a char introduced by user to the buffer
 * 
 * \return 	0: if success 			         		        
 * \return -1: if cmd string full 	    
 * */
int CmdChar(unsigned char newChar) {
	/* If cmd string not full add char to it */
	if (cmdLen < MAX_CMDSTRING_SIZE) {
		cmdString[cmdLen] = newChar;
		cmdLen ++;
		return EXIT_SUCCESS;
	}
	/* If cmd string full return error */
	return STRING_FULL;
}

/** \brief Function to Reset commands buffer
 * 
 * Resets the buffer were commands are kept and the start and end of frame
 * 
*/
void resetcmd(void) {
	cmdLen = 0;
	SOF_C = -1;
	EOF_C = -1;		
}

/** \brief Function to Process the inserted command string
 *                                                      
 * \return  0: valid command                        	    
 * \return	-1: empty string or incomplete command                   
 * \return	-2: invalid command found                            
 * \return	-3: incorrect string format found                       
 */

int cmdProcess(void){
	int i;
	/*This is the process for verifying that the command follows the standard outlined.*/
	/* check for the first $*/
	for(i=0;i<cmdLen;i++){
		if(cmdString[i] == '$'){
			SOF_C = i; /*< First $ should be in the Start of the string*/
			break;
		}
	}
	/*check for the first &*/
	for(i=0;i<cmdLen;i++){
		if(cmdString[i] == '&'){
			EOF_C = i; /*< First & should be at the End of the string*/
			break;
		}
	}
	/* if an & was found before $*/
	if(EOF_C < SOF_C){
		SOF_C = -1;
		EOF_C = -1;
		return WRONG_STR_FORMAT; /*< If an & was found before $, then it's wrongly formatted*/
	}
	/* if it found a $ between the first $ and &*/
	for(i=SOF_C+1;i<EOF_C;i++){
		if(cmdString[i] == '$'){
			return WRONG_STR_FORMAT; /*< If it found a $ between the first $ and &, then it's wrongly formatted*/
		}
	}

	if(SOF_C == -1){
		return WRONG_STR_FORMAT;
	}
	if(EOF_C == -1){
		return WRONG_STR_FORMAT;
	}

	/* Detect empty cmd string */
	if(cmdLen == 0) {
		return EMPTY_STRING;
    } 
	/* Now I am sure i have a "$...&"" string, so I can proceed*/
	if(cmdString[SOF_C+1] == 'T' || cmdString[SOF_C+1] == 't'){
		if(cmdLen<8){
			return WRONG_STR_FORMAT;
		}
		for(i = SOF_C+3;i<EOF_C;i++){
			if(cmdString[i] < '0' || cmdString[i] > '9'){
				return CMD_NOT_FOUND;
			}
		}
		/* change period of PWM thread */
		if(cmdString[SOF_C+2] == 'O' || cmdString[SOF_C+2] == 'o'){
			if( (EOF_C - (SOF_C+3)) == 4){
				thread_pwm_period = (cmdString[SOF_C+3]-'0')*1000+(cmdString[SOF_C+4]-'0')*100+(cmdString[SOF_C+5]-'0')*10+(cmdString[SOF_C+6]-'0')*1;
				return EXIT_SUCCESS;
			}
			else{
				return CMD_NOT_FOUND;
			}
			return EXIT_SUCCESS;
		}
		/* change period of analog input thread */
		else if(cmdString[SOF_C+2] == 'I' || cmdString[SOF_C+2] == 'i'){
			if( (EOF_C - (SOF_C+3)) == 4){
				thread_an_period = (cmdString[SOF_C+3]-'0')*1000+(cmdString[SOF_C+4]-'0')*100+(cmdString[SOF_C+5]-'0')*10+(cmdString[SOF_C+6]-'0')*1;
				return EXIT_SUCCESS;
			}
			else{
				return CMD_NOT_FOUND;
			}
			return EXIT_SUCCESS;
		}
		return EXIT_SUCCESS;
	}
	else{
		return CMD_NOT_FOUND;
	}
	return WRONG_STR_FORMAT;
}

/** \brief Printing Thread for the values of analog inputs and the periods of the threads
 * 
 * This periodic thread with static period prints out the values
 * of the analog inputs converted into volts and also the PWM output
 * 
*/
void thread_print_code(void *argA , void *argB, void *argC){
	/* Timing variables to control task periodicity */
    int64_t fin_time=0, release_time=0;

	printk("Thread print init (periodic)\n");

	/* Compute next release instant */
    release_time = k_uptime_get() + thread_print_period;

	while(1){

		
		// PRINT ADC STATES AND THREAD PERIODS
		printk("\r");
		printk("Analog Read Period: %d  \n\r",thread_an_period);
		printk("\r");

		// PRINT PWM?
		printk("\r");
		printk("PWM Period: %d  \n\r",thread_pwm_period);
		printk("\r");
		printk("\r");
		/* Wait for next release instant */ 
		fin_time = k_uptime_get();
		if( fin_time < release_time) {
        	k_msleep(release_time - fin_time); /* There are other variants, k_sleep(), k_usleep(), ... */
			release_time += thread_print_period;
		}
	}
	timing_stop();
}


/** \brief ADC reading thred
 * 
 * This thread implements reading the Analog Entries and the ADCs.
 * This is a periodic thread whose period can be changed via UART input.
 * 
 * 
*/
void thread_an_code(void *argA , void *argB, void *argC){

	/* Timing variables to control task periodicity */
    int64_t fin_time=0, release_time=0;

	/* Compute next release instant */
    release_time = k_uptime_get() + thread_an_period;


	//ADC SETUP

	
		
	/* Main loop */
	while(true){
		
		/*
		Process:
		1. Call adc_sample for each of the channels
		2. Save the value of err so it can be sent out of the UART
		*/
		k_sem_take(&sem_rtdb_adc,  K_FOREVER);
		if (adc_collect() != 0) {
			errorcount ++;
		}
		k_sem_give(&sem_rtdb_adc);
		k_sem_take(&sem_rtdb_adc,  K_FOREVER);
		adc_print();
		k_sem_give(&sem_rtdb_adc);
		
		/* Wait for next release instant */ 
        fin_time = k_uptime_get();
        if( fin_time < release_time) {
            k_msleep(release_time - fin_time);
            release_time += thread_an_period;
		}
	}
	timing_stop();
}

/** \brief PWM Thread
 * 
 * This is a thread that implements the PWM output.
 * It is periodic and the peirod can be changed via UART input, which also changes the output.
 * 
*/
void thread_pwm_code(void *argA , void *argB, void *argC){

	/* Timing variables to control task periodicity */
    int64_t fin_time=0, release_time=0;

	/* Compute next release instant */
    release_time = k_uptime_get() + thread_pwm_period;
	
	while(1){
		static int div = 1; /* Divider for computing the duty-cycle */
	
		/* Toggle led1 */
		gpio_pin_toggle_dt(&led1);

		/* Adjust the brightness of led0 (associated with pwm) 
		* PWM_NLEVELS levels of intensity, which are actually dividers that set the duty-cycle */
		div = 100 - ((thread_pwm_period - 500) * 99) / 4500;
		
		printk("PWM divider set to %d\n\r", div);
		
		pwm_set_dt(&pwm_led0, PWM_PERIOD, (PWM_PERIOD)/((unsigned int)div)); /* args are period and Ton */


		/* Wait for next release instant */ 
        fin_time = k_uptime_get();
        if( fin_time < release_time) {
            k_msleep(release_time - fin_time);
            release_time += thread_pwm_period;
		}
	}
	timing_stop();
}

/** \brief Thread de comandos
 * 
 * This thread implements commands and does their verification
 * This thread is periodic, with a static period
 * 
*/
void thread_cmd_code(void *argA , void *argB, void *argC){

	/* Timing variables to control task periodicity */
    int64_t fin_time=0, release_time=0;

	/* Compute next release instant */
    release_time = k_uptime_get() + thread_cmd_period;

	while(1){
		
		if(cmd){
			res = cmdProcess();
			printk("\n\rcmdProcess output: %d\n\r", res);
			resetcmd();
			cmd=0;
		}

		/* Wait for next release instant */ 
        fin_time = k_uptime_get();
        if( fin_time < release_time) {
            k_msleep(release_time - fin_time);
            release_time += thread_cmd_period;
		}
	}
	timing_stop();
}

/** \brief Input/output configuration
 * 
 *
 * 
*/
void startup_config(void){

	RTDB_init();

	/* SEMAPHORES */
	k_sem_init(&sem_rtdb_adc, 1, 1);

    /*GPIO*/
	/* Check if devices are ready */
	if (!device_is_ready(led1.port)) {
		printk("Error: led1 device %s is not ready\n", led1.port->name);
		return;
	}
	/* Configure the GPIO pin - led for output */
	ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: gpio_pin_configure_dt failed for led1, error:%d", ret);
		return;
	}

	/* UART */
    if (!device_is_ready(uart)) {
        printk("UART device not ready\r\n");
        return;
    }
	
	ret = uart_callback_set(uart, uart_cb, NULL);
    if (ret) {
		return;
	}
	/* Send the data over UART by calling uart_tx() */
	ret = uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_MS);
    if (ret) {
		return;
	}

    /* Start receiving by calling uart_rx_enable() and pass it the address of the receive  buffer */
	ret = uart_rx_enable(uart ,rx_buf,sizeof rx_buf,Receive_Timeout);
	if (ret) {
		return;
	}

	/*Begin timing function*/
	
	timing_init();
	timing_start();

	/* Set up ADC*/
	adc_init();

	/* Set up PWM*/
	pwm_init();
}