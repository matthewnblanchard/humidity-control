// user_task.h
// Authors: Christian Auspland & Matthew Blanchard
// Description: Contains definitions for the the user task
//	signals and parameters. These need to made available 
//	to all other source files for proper communication with
//	the main control task.

// Tasks/Message Queues - The ESP8266 SDK prefers for programs to be executed via user
//      tasks, which can be scheduled and interrupted according to their priorities.
//      This ensures that the backend functions/interrupts which provide the wifi functionality
//      get appropriate time allocated to them. Tasks act similarly to functions, resting
//      until a message is "posted" to them, which they can read for parameters and act
//      accordingly. Messages are stored in message queues for reading.
//
//      There are three priorities, numbered in descending order (2 > 1 > 0). Only one task
//      of each priority may exist at given time, and once a new task is registered with a
//      preexisting priority, the old task is overwritten. Our task priorities are defined
//      as follows:
//              
//              Level 2: Extremely important, time sensitive tasks. This is reserved for the control task in user_main
//              Level 1: Initialization/configuration tasks. These tasks must be completed
//                      in order for the device to operate correctly. 
//              Level 0: Low priority book-keeping/data collection tasks.

// Message Queues
#define MSG_QUEUE_LENGTH 4
os_event_t * user_msg_queue_0;
os_event_t * user_msg_queue_1;
os_event_t * user_msg_queue_2;

// Timers
os_timer_t timer_reboot;

// Task Calling Macros
#define TASK_RETURN(sig,par) 		system_os_post(USER_TASK_PRIO_2, (sig), (par))
#define TASK_START(task,sig,par)	({\
						system_os_task((task), USER_TASK_PRIO_1, user_msg_queue_1, MSG_QUEUE_LENGTH);\
						system_os_post(USER_TASK_PRIO_1, (sig), (par));\
					})
	
/* --------------------------------------------------- */
/* Control Signals/Parameters                          */
/* --------------------------------------------------- */
/* Signals/Parameters are designed to be combined into */
/* 32-bit value for easy switch case statements. The   */
/* signal occupies the upper 16-bits and the parameter */
/* occupies the lower 16-bits. They can then be OR'd   */
/* together to form a unique signal/parameter combo    */
/* --------------------------------------------------- */


// General control signals/parameters
#define SIG_CONTROL				(uint32)(0x0000 << 16)
#define PAR_CONTROL_START			(uint32)(0x0000)
#define PAR_CONTROL_ERR_DEADLOOP		(uint32)(0xFFFE)
#define PAR_CONTROL_ERR_FATAL			(uint32)(0xFFFF)

// AP Mode (Configuration Mode) signals
#define SIG_APMODE				(uint32)(0x0100 << 16)
#define PAR_APMODE_SETUP_COMPLETE		(uint32)(0x0000)
#define PAR_APMODE_DHCP_CONFIG_FAILURE		(uint32)(0xFFFD)
#define PAR_APMODE_MODE_CONFIG_FAILURE		(uint32)(0xFFFE) 
#define PAR_APMODE_AP_MODE_FAILURE		(uint32)(0xFFFF) 
