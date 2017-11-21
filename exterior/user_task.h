// user_task.h
// Authors: Christian Auspland & Matthew Blanchard
// Desc: header file containing resources for user task book-keeping
//      and timing

#ifndef USER_TASK_H
#define USER_TASK_H

#include <user_interface.h>
#include <osapi.h>

#define MSG_QUEUE_LENGTH 4

#define CALL_ERROR(X) system_os_post(USER_TASK_PRIO_2, X, 0)
#define ERR_FATAL 0

// Message Queues - The ESP8266 SDK prefers for programs to be executed via user
//      tasks, which can be scheduled and interrupted according to their priorities.
//      This ensures that the backend functions/interrupts which provide the wifi functionality
//      get approproate time allocated to them. Tasks act similarly to functions, resting
//      until a message is "posted" to them, which they can read for parameters and act
//      accordingly. Messages are stored in message queues for reading.
//
//      There are three priorities are take in descending order (2 > 1 > 0). Only one task
//      of each priority may exist at given time, and once a new task is registered with a
//      preexisting priority, the old task is overwritten. Our tasks priorities are defined
//      as follows:
//              
//              Level 2: Extremely important, time sensitive tasks, e.g. error handling
//              Level 1: Initialization/configuration tasks. These tasks must be completed
//                      in order for the device to operate correctly
//              Level 0: Low priority book-keeping/data collection tasks.
os_event_t * user_msg_queue_0;
os_event_t * user_msg_queue_1;
os_event_t * user_msg_queue_2;

// Timers
os_timer_t timer_1;
os_timer_t timer_2;
os_timer_t timer_humidity;

// User Task: user_task_error(os_event_t * e)
// Desc: User task which is called when an error occurs. If recovery is possible,
//      this task will handle recovery. If recovery is not possible, this task
//      will call itself, forcing the user program to idle indefinetly until
//      reset.
void user_task_error(os_event_t * e);
#endif
