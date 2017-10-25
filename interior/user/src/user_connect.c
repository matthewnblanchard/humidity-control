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
                        var ws;\
                        function button_ws() {\
                                ws = new WebSocket('ws://' + window.location.hostname + ':80/');\
                                console.log('WebSocket opened (hopefully)');\
                                ws.onopen = function(evt) {\
                                        console.log('WebSocket connected');\
                                        ws.send('Hello HBFC/D');\
                                };\
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
        "Sec-WebSocket-Accept: %s\r\n\r\n"
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
        uint8 *p1;                              // Character pointers for navagating data
        uint8 *p2;
        uint8 key[64];                          // Concatenated secure key
        uint8 key_len = 0;                      // Length of secure key
        uint8 guid_len = os_strlen(ws_guid);    // Length of GUID
        uint8 sha1_sum[20];                     // SHA-1 Hash sum
        uint8 sha1_key[32];                     // Secure key post-hash
        size_t olen = 0;                        // Base-64 econding length     
        uint8 response_buf[256];                // Buffer for HTTP response
        uint8 response_len = 0;                 // Length of HTTP response

        os_printf("received data from client\r\n");
        os_printf("data=%s\r\n", pusrdata);

        
        // Check if we recieved an HTTP GET request
        if (os_strncmp(pusrdata, "GET ", 4) == 0) {
                os_printf("http GET request detected\r\n");

                // Check if the request is for a web socket
                if (os_strstr(pusrdata, ws_header) != NULL) {

                        // Get the secure key from the client data
                        p1 = (uint8 *)os_strstr(pusrdata, ws_key);      // Navigate to key line
                        p1 += os_strlen(ws_key);                        // Move to beginning of the actual key portion of the key line
                        p2 = (uint8 *)os_strstr(p1, "\r\n");            // Navigate to end of key
                        key_len = p2 - p1;
                        os_printf("found key of length %d\r\n", key_len);

                        // Concatenate the secure key with the GUID for WebSockets
                        if ((key_len + guid_len) < 64) {
                                os_memcpy(key, p1, key_len);                            // First half = client key
                                os_memcpy(&key[key_len], ws_guid, guid_len);            // Second half = GUID        
                                os_printf("concat_key=%s\r\n", key);    
                        } else {
                                os_printf("failed to create websocket response\r\n");
                                return;
                        } 

                        // Calculate SHA-1 Hash
                        os_memset(sha1_sum, 0, 20);
                        mbedtls_sha1(key, guid_len + key_len, sha1_sum);    
                        os_printf("sha1_hash=%s\r\n", sha1_sum);
                        int i = 0;
                        for (i = 0; i < 20; i++){
                                os_printf("%x:", sha1_sum[i]);
                        }
                        os_printf("\r\n");

                        // Encode SHA-1 Hash in base 64
                        mbedtls_base64_encode(NULL, 0, &olen, sha1_sum, 20);                                    // Buffer length needed for Base64 encoding -> olen
                        if (mbedtls_base64_encode(sha1_key, sizeof(sha1_key), &olen, sha1_sum, 20) == 0) {      // Encode in base 64
                       
                                // Form WebSocket request response & send
                                sha1_key[olen] = '\0'; 
                                os_printf("base64=%s\r\n", sha1_key);
                                response_len = os_sprintf(response_buf, ws_response, sha1_key); 
                                espconn_send(client_conn, response_buf, response_len);
                                os_printf("sent=%s\r\n", response_buf);                         

                                // The connection has upgraded to a WebSocket. Apply WebSocket callbacks for data transmission
                                espconn_regist_recvcb(client_conn, user_ws_recv_cb);
                         
                        } else {
                                os_printf("failed to create websocket response\r\n");
                                return;
                        }

                // The request is ordinary
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

void ICACHE_FLASH_ATTR user_ws_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
        struct espconn *client_conn= arg;       // Grab connection
        uint8 opcode = 0;                       // WebSocket packet Opcode
        uint64 payload_len = 0;                 // Payload size
        uint8 mask[4];                          // Payload mask
        bool mask_flag = 0;                     // Flag indicating if data is masked
        uint8 mask_start = 0;                   // Index of start of mask
        uint64 i = 0;                           // Loop index

        /* Websocket Packet Structure ================================= */
        /*                                                              */
        /* Byte 0:                                                      */
        /*      Bit 0:    FIN (1 if last packet, 0 otherwise            */
        /*      Bit 1-3:  RSV (Reserved)                                */
        /*      Bit 4-7:  OPCODE (denotes data type                     */
        /* Byte 1:                                                      */
        /*      Bit 0:    MASK (1 if data is masked)                    */
        /*      Bit 1-7:  Payload Length (length of actual data)        */
        /* ----- If Payload Length is 126        ---------------------- */
        /* Byte 2-3:                                                    */
        /*      Bit 0-7:  Extended payload length                       */
        /* Byte 4-7:                                                    */
        /*      Bit 0-7:  Masking key                                   */
        /* ----- If Payload Length is 127 ----------------------------- */
        /* Byte 2-9:                                                    */
        /*      Bit 0-7:  Extended payload length                       */
        /* Byte 10-13:                                                  */
        /*      Bit 0-7:  Masking key                                   */
        /* ----- Else ------------------------------------------------- */
        /* Byte 2-5:                                                    */
        /*      Bit 0-7: Masking key                                    */
        /* ---- Always ------------------------------------------------ */
        /* Byte 6/8/14 - ?:                                             */
        /*      Bit 0-7: Payload data                                   */
        /* ============================================================ */
  
        // The smallest possible packet (Opcode and payload length with no mask or payload)      
        if (length < 2) {
                os_printf("received malformed ws packet\r\n");
                return;
        }

        // Evaluate Opcode 
        opcode = (pusrdata[0] & 0x0F);
        switch (opcode) {
                case 0x01:      // Text   data
                case 0x02:      // Binary data
                case 0x08:      // Close signal
                        os_printf("opcode=%d\r\n", opcode);

                        // Parse payload length
                        payload_len = (pusrdata[1] & 0xEF);
                        if (payload_len == 126) {
                                payload_len = ((uint64)pusrdata[2] << 8) | ((uint64)pusrdata[3] << 16);
                                mask_start = 4;
                        } else if (payload_len == 127) {
                                payload_len = ((uint64)pusrdata[2] << 0) | ((uint64)pusrdata[3] << 8) | ((uint64)pusrdata[4] << 16) | ((uint64)pusrdata[5] << 24);
                                payload_len |= ((uint64)pusrdata[6] << 32) | ((uint64)pusrdata[7] << 40) | ((uint64)pusrdata[8] << 48) | ((uint64)pusrdata[9] << 56);
                                mask_start = 10;
                        } else {
                                mask_start = 2;
                        } 

                        // Extract mask if necessary
                        mask_flag = pusrdata[1] & 0x80;
                        if (mask_flag == 0) {
                                os_printf("received unmasked packet=%s\r\n", pusrdata);
                                return;
                        }       
                        os_strncpy(mask, &pusrdata[mask_start], 4); 

                        // Perform unmasking operation: each byte can be unmasked by XORing it with byte (i % 4) of the mask
                        for (i = 0; i < (length - mask_start + 4); i++) {
                                pusrdata[mask_start + 4 + i] ^= mask[i % 4];
                        }
                        os_printf("received masked packet=%s\r\n", &pusrdata[mask_start + 4]);
                        break;
        }; 
        return;
};
