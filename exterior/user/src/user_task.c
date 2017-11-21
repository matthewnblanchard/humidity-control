// user_task.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_task.h"

void user_task_error(os_event_t * e) 
{
        switch (e->sig) {
                case ERR_FATAL:  // Fatal (default) error: hangs user program indefinetly
                default:
                        system_os_post(USER_TASK_PRIO_2, ERR_FATAL, 0);        
                        break;  
        }
};
