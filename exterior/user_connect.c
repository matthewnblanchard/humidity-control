// user_connect.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_connect.h"

void ICACHE_FLASH_ATTR udp_listen_cb(void *arg, char *pdata, unsigned short len)
{
        struct espconn *rec_conn = arg;         // Pull espconn from passed args
        remot_info *info = NULL;                // Connection info

        os_printf("received data\r\n");

        // Get connection info, print source IP
        espconn_get_connection_info(rec_conn, &info, 0);
        os_printf("src_ip=%d.%d.%d.%d\r\n",
                info->remote_ip[0],
                info->remote_ip[1],
                info->remote_ip[2],
                info->remote_ip[3]
        );
}
