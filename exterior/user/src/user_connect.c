// user_connect.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_connect.h"

// espconn structs - these are control structures for TCP/UDP connections
struct espconn tcp_connect_conn;
struct _esp_tcp tcp_connect_proto;

static struct espconn *int_con = NULL;		// Connection to interior system

struct station_config station_conn;

void ICACHE_FLASH_ATTR user_int_connect_init(os_event_t *e)
{
	sint8 result = 0;

	// Prepare TCP server on port 6000
	os_memset(&tcp_connect_conn, 0, sizeof(tcp_connect_conn));
	os_memset(&tcp_connect_proto, 0, sizeof(tcp_connect_proto));

	tcp_connect_proto.local_port = ESPCONNECT_ACCEPT;
	tcp_connect_proto.remote_port = ESPCONNECT_ACCEPT;
	tcp_connect_conn.type = ESPCONN_TCP;
	tcp_connect_conn.proto.tcp = &tcp_connect_proto;
	
	// Register Callbacks for the TCP server
	result = espconn_regist_connectcb(&tcp_connect_conn, user_int_connect_cb);
	if (result < 0) {
		PRINT_DEBUG(DEBUG_ERR, "ERROR: failed to register tcp connection callback\r\n");
		TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_LISTEN_FAILURE);
		return;
	}
	
	// Start TCP server
	os_printf("Starting TCP server\r\n");
	result = espconn_accept(&tcp_connect_conn);
	if (result < 0) {
		PRINT_DEBUG(DEBUG_ERR, "failed to start tcp server\r\n");
		TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_LISTEN_FAILURE);
		return;
	}
	
	// Set TCP timeout interval to 1800 second
	espconn_regist_time(&tcp_connect_conn, 1800, 0);

	TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_LISTEN_INIT);
	return;
}

void ICACHE_FLASH_ATTR user_int_connect_cb(void *arg)
{
	PRINT_DEBUG(DEBUG_LOW, "Connection made to exterior over port ESPCONN\r\n");
	struct espconn *client_conn = arg;

	int_con = arg;	// Save the connection

	// Register callbacks for the connected client
	espconn_regist_reconcb(client_conn, user_tcp_recon_cb);
	espconn_regist_disconcb(client_conn, user_tcp_discon_cb);
	espconn_regist_sentcb(client_conn, user_tcp_sent_cb);

	TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_CONNECTED);
	return;
}

void ICACHE_FLASH_ATTR user_tcp_recon_cb(void *arg, sint8 err)
{
	PRINT_DEBUG(DEBUG_ERR, "tcp connection error occured\r\n");
}

void ICACHE_FLASH_ATTR user_tcp_discon_cb(void *arg)
{
	PRINT_DEBUG(DEBUG_LOW, "tcp connection disconnected\r\n");
	int_con = NULL;
}

void ICACHE_FLASH_ATTR user_tcp_sent_cb(void *arg)
{
	PRINT_DEBUG(DEBUG_LOW, "Data sent to server\r\n");
}

void ICACHE_FLASH_ATTR user_int_connect_cleanup(os_event_t *e)
{
	// Disconnect/delete interior connections
	espconn_disconnect(&tcp_connect_conn);
	espconn_delete(&tcp_connect_conn);

	TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_INT_CLEANUP);
};

void ICACHE_FLASH_ATTR user_int_send_data(os_event_t *e)
{
	uint8 send_buf[4];	// Buffer to send to interior
	sint8 result = 0;	// Send operation result	

	// Copy the exterior humidity into the buffer
	os_memcpy(send_buf, (uint8 *)&sensor_data_ext, 4);

	// Send the data
	result = espconn_send(int_con, send_buf, 4);
	if (result < 0) {
		PRINT_DEBUG(DEBUG_ERR, "failed to send RH to interior, code=%d\r\n", result);
	}
	return;
};
