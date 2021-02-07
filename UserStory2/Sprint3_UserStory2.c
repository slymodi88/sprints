/*
 * Sprint3_UserStory2.c
 *
 * Created: 2/1/2020 6:43:59 PM
 *  Author: Amr
 */ 
/************************************* INCLUDES *****************************************/
#include "Sprint2_UserStory1.h"
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"
#include "queue.h"
#include "semphr.h"
#include <stdlib.h>
#include "std_types.h"
#include "lcd.h"
#include "pushButton.h"
#include "led.h"
#include "keypad.h"
#include "BCM.h"
#include "led.h"
/********************************** GLOBAL VARIABLES *************************************/
/* mutex for the hardware resources */
static xSemaphoreHandle gMutex_LCD = NULL;

/* binary semaphores */
static xSemaphoreHandle gBinarySemaphore_Press = NULL;
static xSemaphoreHandle gBinarySemaphore_TxLed = NULL;
static xSemaphoreHandle gBinarySemaphore_RxLed = NULL;
static xSemaphoreHandle gBinarySemaphore_ReceiveAgain = NULL;

/* queues */
static QueueHandle_t TxQueue = NULL;
static QueueHandle_t RxQueue = NULL;

/* tasks handles */
extern TaskHandle_t Task_UpdateButtons_Handle;
extern TaskHandle_t Task_UpdateKeypad_Handle;
extern TaskHandle_t Task_ReadKeypadInput_Handle;
extern TaskHandle_t Task_BCMSend_Handle;
extern TaskHandle_t Task_BCMReceive_Handle;
extern TaskHandle_t Task_RunDispatcher_Handle;
extern TaskHandle_t Task_TxLED_Handle;
extern TaskHandle_t Task_RxLED_Handle;

/****************************** STATIC FUNCTIONS PROTOTYPES *****************************/
static void Function_RX_Cbk(void)
{
	/* give the semaphore */
	xSemaphoreGive(gBinarySemaphore_ReceiveAgain);
}

/****************************** FUNCTIONS PROTOTYPES *****************************/

/*
 * Input:	p_Parameter: input to the task
 * Output:	----
 * In/Out:	----
 * Return:	----
 * Description: task to initialize the system only once
 */
void Task_SystemInit(void* p_Parameter)
{
	/* specify required delay time for the LCD hardware before it can be used */
	TickType_t u16_Delay = 30/portTICK_PERIOD_MS;
	
	/* create the mutex */
	gMutex_LCD = xSemaphoreCreateMutex();
	
	/* create semaphores */
	gBinarySemaphore_TxLed = xSemaphoreCreateBinary();
	gBinarySemaphore_RxLed = xSemaphoreCreateBinary();
	gBinarySemaphore_Press = xSemaphoreCreateBinary();
	gBinarySemaphore_ReceiveAgain = xSemaphoreCreateBinary();
	
	/* create a queue for Tx & Rx */
	TxQueue = xQueueCreate(16, sizeof(uint8));
	RxQueue = xQueueCreate(16, sizeof(uint8));
	
	while(1)
	{
		/* suspend all periodic tasks after initialization */
		vTaskSuspend(Task_UpdateButtons_Handle);
		vTaskSuspend(Task_UpdateKeypad_Handle);
		vTaskSuspend(Task_ReadKeypadInput_Handle);
		vTaskSuspend(Task_BCMSend_Handle);
		vTaskSuspend(Task_BCMReceive_Handle);
		vTaskSuspend(Task_RunDispatcher_Handle);
		vTaskSuspend(Task_TxLED_Handle);
		vTaskSuspend(Task_RxLED_Handle);
		
		/* initialize LCD */
		LCD_Init();
		/* block the task till initialization finish(just a delay) */
		vTaskDelay(u16_Delay);
		
		/* initialize 1 push button */
		pushButton_Init(BTN_0);
		
		/* initialize Keypad */
		Keypad_Init();
		
		/* initialize LED 0, 1 */
		Led_Init(LED_0);
		Led_Init(LED_1);
		Led_Init(LED_2);
		Led_Init(LED_3);
		
		Led_Off(LED_0);
		Led_Off(LED_1);
		Led_Off(LED_2);
		Led_Off(LED_3);
		
		/* initialize BCM */
		BCM_Init();
		
		/* resume all periodic tasks after initialization */
		vTaskResume(Task_UpdateButtons_Handle);
		vTaskResume(Task_UpdateKeypad_Handle);
		vTaskResume(Task_ReadKeypadInput_Handle);
		vTaskResume(Task_BCMSend_Handle);
		vTaskResume(Task_BCMReceive_Handle);
		vTaskResume(Task_RunDispatcher_Handle);
		vTaskResume(Task_TxLED_Handle);
		vTaskResume(Task_RxLED_Handle);
				
		/* suspend the Task_SystemInit task */
		vTaskSuspend(NULL);
	}
}


/**
 * Input:	p_Parameter: input to the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to scan and update the keypad press by saving the pressed key with an internal static variable
 */
void Task_UpdateKeypad(void* p_Parameter)
{
	/* specify required delay */
	TickType_t u16_Delay = 5/portTICK_PERIOD_MS;
	
	for (;;)
	{
		/* always update the keypad reading every 20ms */
		Keypad_Update();
		
		/* debouncing delay */
		vTaskDelay(u16_Delay);
	}
}


/**
 * Input:	p_Parameter: input to the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to get the pressed key and print it on the LCD
 */
void Task_ReadKeypadInput(void* p_Parameter)
{
	/* specify required delay */
	TickType_t u16_Delay = 5/portTICK_PERIOD_MS;
	
	/* key variable */
	uint8 u8_Char;
	
	while(1)
	{
		/* save the pressed key at the state of prepressed into the queue */
		u8_Char = Keypad_GetPressedKey(Key_PrePressed_State);
		if ( u8_Char != NO_KEY_IS_PRESSED )
		{
			/* save the pressed key into the queue */
			xQueueSend(TxQueue, (void*)&u8_Char, (TickType_t)0);
			
			/* make sure we have the LCD mutex before using it */
			if ( xSemaphoreTake(gMutex_LCD, (TickType_t)0) == pdTRUE )
			{
				/* write data on the LCD */
				LCD_WriteData(u8_Char);
					
				/* then release the LCD mutex */
				xSemaphoreGive(gMutex_LCD);
			}
			else
			{
				/* Do Nothing */
			}
		}
		else
		{
			/* Do Nothing */
		}
		vTaskDelay(u16_Delay);	
	}
}


/**
 * Input:	p_Parameter: input to the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to update the push buttons and overcome the bouncing with real time delay
 */
void Task_UpdateButtons(void* p_Parameter)
{
	/* specify required delay for the push button to debounce */
	TickType_t u16_Delay = 5/portTICK_PERIOD_MS;
	
	/* enum variable to save push button state */
	En_buttonStatus_t enu_Button_State;
	
	while(1)
	{
		/* always update the push button state */
		pushButton_Update();
		
		/* get the button state */
		enu_Button_State = pushButton_GetStatus(BTN_0);
		
		/* if button is in the Prepressed state */
		if ( Prepressed == enu_Button_State )
		{
			/* give the semaphore */
			xSemaphoreGive(gBinarySemaphore_Press);
		}
		else
		{
			/* Do Nothing */
		}
		
		/* delay between each update for debouncing */
		vTaskDelay(u16_Delay);
	}
}

/**
 * Input:	p_TaskHandle: handle for the task "Task_AnnounceWinner"
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to read the push button and send the queue data with the BCM UART
 */
void Task_BCMSend(void* pvParameters)
{
	/* specify required delay for the push button to debounce */
	TickType_t u16_Delay = 5/portTICK_PERIOD_MS;
	
	/* buffer */
	uint8 au8_Buffer[16] = {'\0'};
	
	/*  buffer counter */
	uint8 u8_BufferCounter = 0;
	
	while(1)
	{
		/* if we can take the semaphore, that means it has been given and button is press */
		if ( xSemaphoreTake(gBinarySemaphore_Press, (TickType_t)0) == pdTRUE )
		{				
			/* if queue is not empty */
			if ( uxQueueMessagesWaiting(TxQueue) != 0 )
			{
				/* read the message from the queue */
				xQueueReceive(TxQueue, (void*)&au8_Buffer[u8_BufferCounter], (TickType_t)0);
				
				/* increment the counter to save the character at the next position  */
				u8_BufferCounter++;
					
				/* give the semaphore so we can enter here again */
				xSemaphoreGive(gBinarySemaphore_Press);
			}
			else
			{
				/* if succeeded reading the from the queue, then send the data with BCM UART */
				BCM_Send(au8_Buffer, 4, NULL);
				
				/* give the semaphore so the task responsible for toggling the LED can work now */
				xSemaphoreGive(gBinarySemaphore_TxLed);

				/* reset the counter  */
				u8_BufferCounter = 0;
				
				/* make sure we have the LCD mutex before using it */
				if ( xSemaphoreTake(gMutex_LCD, (TickType_t)0) == pdTRUE )
				{
					/* clear the first row of the LCD */
					LCD_GoToLineColumn(1,1);
					LCD_WriteString((uint8 *)"                ");
					LCD_GoToLineColumn(1,1);
					
					/* then release the LCD mutex  */
					xSemaphoreGive(gMutex_LCD);
				}
				else
				{
					/* Do Nothing */
				}
			}
		}
		else
		{
			/* Do Nothing */
		}
		
		/* delay to give the OS the control again */
		vTaskDelay(u16_Delay);
	}
}


/**
 * Input:	p_Parameter: input to the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to receive the data from the BCM UART
 */
void Task_BCMReceive(void* p_Parameter)
{
	/* specify required delay */
	TickType_t u16_Delay = 5/portTICK_PERIOD_MS;
	
	/* loop counter */
	uint8 u8_LoopCounter;
	
	/* receive buffer array */
	uint8 au8_RecBuffer[17];

	/* make all array buffer elements equal null */
	for ( u8_LoopCounter = 0; u8_LoopCounter < 17; u8_LoopCounter++)
	{
		au8_RecBuffer[u8_LoopCounter] = '\0';
	}
		
	/* call the start the receive functionality to lock the buffer and initiate receiving */
	BCM_SetupReceiveBuffer(au8_RecBuffer, 17, Function_RX_Cbk);
	
	for (;;)
	{
		/* make sure we have the semaphore before using it */
		if ( xSemaphoreTake(gBinarySemaphore_ReceiveAgain, (TickType_t)0) == pdTRUE )
		{
			/* give the semaphore so the task responsible for toggling the LED can work now */
			xSemaphoreGive(gBinarySemaphore_RxLed);
			
			/* make the element before the null equals to null as it is the checksum byte from the transmitter */
			for ( u8_LoopCounter = 0; u8_LoopCounter < 17; u8_LoopCounter++)
			{
				if ( au8_RecBuffer[u8_LoopCounter] == '\0' )
				{
					au8_RecBuffer[u8_LoopCounter-1] = '\0';
					break;
				}
				else
				{
					/* Do Nothing */
				}
			}
			
			/* make sure we have the LCD mutex before using it */
			if ( xSemaphoreTake(gMutex_LCD, (TickType_t)0) == pdTRUE )
			{
				/* clear the first row of the LCD */
				LCD_GoToLineColumn(2,1);
				LCD_WriteString((uint8 *)"                ");
				LCD_GoToLineColumn(2,1);
				LCD_WriteString((uint8 *)au8_RecBuffer);
				vTaskDelay(2000);
				LCD_GoToLineColumn(2,1);
				LCD_WriteString((uint8 *)"                ");
				LCD_GoToLineColumn(1,1);
				
				/* then release the LCD mutex  */
				xSemaphoreGive(gMutex_LCD);
			}
			else
			{
				/* Do Nothing */
			}
			
			/* make all array buffer elements equal null */
			for ( u8_LoopCounter = 0; u8_LoopCounter < 17; u8_LoopCounter++)
			{
				au8_RecBuffer[u8_LoopCounter] = '\0';
			}
			/* unlock the BCm Rx Buffer for another sending */
			BCM_RxUnlock();
			
			/* call the start the receive functionality again to lock the buffer and initiate receiving again */
			BCM_SetupReceiveBuffer(au8_RecBuffer, 17, Function_RX_Cbk);
		}
		else
		{
			
		}	
		
		/* delay */
		vTaskDelay(u16_Delay);
	}
}


/**
 * Input:	p_Parameter: input to the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to run the BCM dispatcher periodically
 */
void Task_RunDispatcher(void* p_Parameter)
{
	/* specify required delay */
	TickType_t u16_Delay = 1/portTICK_PERIOD_MS;
	
	while(1)
	{
		/* run the BCM Tx Dispatcher */
		BCM_TxDispatcher();
		
		/* run the BCM Rx Dispatcher */
		BCM_RxDispatcher();
		
		/* delay */
		vTaskDelay(u16_Delay);
	}
}


/**
 * Input:	p_TaskHandle: handle for the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to turn led 1 on if Tx of data is finished
 */
void Task_TxLED(void* pvParameters)
{
	/* specify required delay */
	TickType_t u16_Delay = 5/portTICK_PERIOD_MS;
	
	while(1)
	{
		
		/* if we can take the semaphore, that means it has been given */
		if ( xSemaphoreTake(gBinarySemaphore_TxLed, (TickType_t)0) == pdTRUE )
		{
			/* turn led on */
			Led_On(LED_0);
			
			/* delay 200 ms */
			vTaskDelay(200);
			
			/* turn led off */
			Led_Off(LED_0);
		}
		else
		{
			/* Do Nothing */
		}
		
		vTaskDelay(u16_Delay);
	}
}

/**
 * Input:	p_TaskHandle: handle for the task
 * Output:	----
 * In/Out:	----			
 * Return:	----		
 * Description: task to turn led 1 on if Rx of data is finished
 */
void Task_RxLED(void* pvParameters)
{
	/* specify required delay */
	TickType_t u16_Delay = 5/portTICK_PERIOD_MS;
	
	while(1)
	{
		/* if we can take the semaphore, that means it has been given */
		if ( xSemaphoreTake(gBinarySemaphore_RxLed, (TickType_t)0) == pdTRUE )
		{
			/* turn led on */
			Led_On(LED_1);
			
			/* delay 500 ms */
			vTaskDelay(500);
			
			/* turn led off */
			Led_Off(LED_1);
		}
		else
		{
			/* Do Nothing */
		}
		vTaskDelay(u16_Delay);
	}
}