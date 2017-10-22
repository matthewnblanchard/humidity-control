// user_connect.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_connect.h"

// HTML for front end webpage
char *front_page = {
        "HTTP/1.1 200 OK\r\n\
        Content-type: text/html\r\n\r\n\
        <html>\
        <head>\
                <title>HBFC/D Front Page</title>\
                <script>\
                        var websocket;\
                        function button_ws() {\
                                websocket = new WebSocket('ws://' + window.location.hostname + ':80/');\
                                console.log('Web socket opened (hopefully)');\
                        };\
                </script>\
        <body>\
                <h1>\
                        Humidity Based Fan Controller / Dehumidifer Monitoring & Control\
                </h1>\
                <button id=\"ws_start\" type=\"button\" onclick=\"button_ws();\">Start Websocket</button>\
        </body>\
        </html>"
};

// WebSocket constants
char *ws_header = "Upgrade: websocket\r\n";
char *ws_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
char *ws_key = "Sec-WebSocket-Key: ";
char *ws_response = {
        "HTTP/1.1 101 Switching Protocols\r\n"\
        "Upgrade: websocket\r\n"\
        "Connection: Upgrade\r\n"\
        "Sec-WebSocket-Accept: %s\r\n\r\n"\
};

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
};

void ICACHE_FLASH_ATTR user_front_init(os_event_t *e)
{
        sint8 result = 0;

        // Set up TCP server on port 80
        os_memset(&tcp_front_conn, 0, sizeof(tcp_front_conn));          // Clear connection settings
        os_memset(&tcp_front_proto, 0, sizeof(tcp_front_proto));
        tcp_front_proto.local_port = HTTP_PORT;                         // Listen for HTTP traffic
        tcp_front_conn.type = ESPCONN_TCP;                              // TCP protocol
        tcp_front_conn.state = ESPCONN_NONE;
        tcp_front_conn.proto.tcp = &tcp_front_proto;
        
        // Register callbacks for the TCP server
        result = espconn_regist_connectcb(&tcp_front_conn, user_front_connect_cb);
        if (result < 0) {
                os_printf("failed to register connect callback, error=%d\r\n", result);
                CALL_ERROR(ERR_FATAL);
        }

        // Start listening
        result = espconn_accept(&tcp_front_conn);      
        if (result < 0) {
                os_printf("failed to start tcp server, error=%d\r\n", result);
                CALL_ERROR(ERR_FATAL);
        }

        return;
};

void ICACHE_FLASH_ATTR user_front_connect_cb(void *arg)
{
        os_printf("client connected to webserver front end\r\n");
        struct espconn *client_conn = arg;       

        // Register callbacks for the connected client
        espconn_regist_recvcb(client_conn, user_front_recv_cb);
        espconn_regist_reconcb(client_conn, user_front_recon_cb);
        espconn_regist_disconcb(client_conn, user_front_discon_cb);
        espconn_regist_sentcb(client_conn, user_front_sent_cb);

        return;
};

void ICACHE_FLASH_ATTR user_front_recon_cb(void *arg, sint8 err)
{
        os_printf("tcp connection error occured\r\n");
};

void ICACHE_FLASH_ATTR user_front_discon_cb(void *arg)
{
        os_printf("tcp connection disconnected\r\n");
};

void ICACHE_FLASH_ATTR user_front_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
        struct espconn *client_conn = arg;      // Pull client connection info 

        os_printf("received data from client\r\n");
        os_printf("data=%s\r\n", pusrdata);

        
        // Check if we recieved an HTTP GET request
        if (os_strncmp(pusrdata, "GET ", 4) == 0) {
                os_printf("http GET request detected\r\n");

                // Check if the request is for a web socket
                if (os_strstr(pusrdata, "Upgrade: websocket") != NULL) {
                        

                // Check if the request is ordinary
                } else {
                        os_printf("sending front page\r\n");
                        espconn_send(client_conn, front_page, os_strlen(front_page)); 
                }
        }

        return;
};

void ICACHE_FLASH_ATTR user_front_sent_cb(void *arg)
{
        os_printf("sent to client\r\n");
};
