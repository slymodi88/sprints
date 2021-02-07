/*
 * Sprint3_UserStory2.h
 *
 * Created: 2/1/2020 6:44:18 PM
 *  Author: Amr
 */ 


#ifndef SPRINT3_USERSTORY2_H_
#define SPRINT3_USERSTORY2_H_

/************************************* INCLUDES *****************************************/
#include "std_types.h"

/******************************* FUNCTIONS PROTOTYPES ***********************************/


/*
 * Input:	p_Parameter: input to the task
 * Output:	----
 * In/Out:	----
 * Return:	----
 * Description: task to initialize the system only once
 */
void Task_SystemInit(void* p_Parameter);


/**
 * Input:	p_Parameter: input to the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to update the push buttons and overcome the bouncing with real time delay
 */
void Task_UpdateButtons(void* p_Parameter);

/**
 * Input:	p_Parameter: input to the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to scan and update the keypad press by saving the pressed key with an internal static variable
 */
void Task_UpdateKeypad(void* p_Parameter);


/**
 * Input:	p_Parameter: input to the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to get the pressed key and print it on the LCD
 */
void Task_ReadKeypadInput(void* p_Parameter);


/**
 * Input:	p_TaskHandle: handle for the task "Task_AnnounceWinner"
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to read the push button and send the queue data with the BCM UART
 */
void Task_BCMSend(void* pvParameters);

/**
 * Input:	p_Parameter: input to the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to receive the data from the BCM UART
 */
void Task_BCMReceive(void* p_Parameter);

/**
 * Input:	p_Parameter: input to the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to run the BCM dispatcher periodically
 */
void Task_RunDispatcher(void* p_Parameter);

/**
 * Input:	p_TaskHandle: handle for the task "Task_AnnounceWinner"
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to turn led 1 on if Tx of data is finished
 */
void Task_TxLED(void* pvParameters);

/**
 * Input:	p_TaskHandle: handle for the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to turn led 1 on if Rx of data is finished
 */
void Task_RxLED(void* pvParameters);

#endif /* SPRINT3_USERSTORY2_H_ */