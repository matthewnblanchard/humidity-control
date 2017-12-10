// user_connect.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_connect.h"

// HTML for front end webpage
const char const *front_page = {
        "HTTP/1.1 200 OK\r\n\
        Content-type: text/html\r\n\r\n\
	<!DOCTYPE html>\
        <html>\
        <head>\
		<meta charset=\"UTF-8\"/>\
                <title>HBFC/D Front Page</title>\
                <script>\
                        var ws;\
                        function button_ws() {\
                                ws = new WebSocket('ws://' + window.location.hostname + ':80/');\
                                ws.binaryType = \"arraybuffer\";\
                                console.log('WebSocket opened');\
                                ws.onopen = function(evt) {\
                                        console.log('WebSocket connected');\
                                        ws.send('Hello HBFC/D');\
                              	};\
                                ws.onmessage = function(evt) {\
                                        if(evt.data instanceof ArrayBuffer) {\
                                                var buffer = evt.data;\
                                                var view = new DataView(buffer);\
						var int_element = document.getElementById(\"int_humidity\");\
						var ext_element = document.getElementById(\"ext_humidity\");\
						var rpm_element = document.getElementById(\"rpm\");\
						int_element.innerHTML = view.getFloat32(0, true);\
						ext_element.innerHTML = view.getFloat32(4, true);\
						rpm_element.innerHTML = view.getInt32(8, true);\
                                        };\
					ws.send(\"ack\");\
                                };\
				var ws_init = document.getElementById(\"ws_init\");\
				ws_init.style.display = \"none\";\
				var config = document.getElementById(\"config\");\
				config.style.display = \"block\";\
				var data = document.getElementById(\"data\");\
				data.style.display = \"block\";\
                        };\
			function config_submit() {\
				var config_speed = document.getElementById(\"config_speed\");\
				ws.send(\"speed=\" + config_speed.value + \",\");\
				console.log(\"Sending configuration\");\
			};\
                </script>\
        <body>\
                <h1>\
                        Humidity Based Fan Controller / Dehumidifer Monitoring & Control\
                </h1>\
		<div id=\"ws_init\" style=\"\">\
                	<button id=\"ws_start\" type=\"button\" onclick=\"button_ws();\">Start Websocket</button>\
		</div>\
		<div id=\"config\" style=\"display:none\">\
			<input id=\"config_speed\" type=\"number\" step=\"1\"><br>\
			<button id=\"config_submit\" type=\"button\" onclick=\"config_submit();\">Modify Configuration</button><br>\
		</div>\
		<div id=\"data\" style=\"display:none\">\
			<table>\
				<tr>\
					<th>Interior Humidity (%RH)</th>\
					<td id=\"int_humidity\">Unknown</td>\
				</tr>\
				<tr>\
					<th>Exterior Humidity (%RH)</th>\
					<td id=\"ext_humidity\">Unknown</td>\
				</tr>\
				<tr>\
					<th>Fan RPM</th>\
					<td id=\"rpm\">Unknown</td>\
				</tr>\
			<table>\
		</div>\
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

struct espconn *ws_conn = NULL;		// WebSocket control structure

// espconn structs - these are control structures for TCP/UDP connections
struct espconn tcp_ext_conn;
struct _esp_tcp tcp_ext_proto;
struct espconn tcp_front_conn;
struct _esp_tcp tcp_front_proto;

// WebSocket update timer
os_timer_t ws_timer;

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
                PRINT_DEBUG(DEBUG_ERR, "ERROR: failed to register connect callback, error=%d\r\n", result);
		TASK_RETURN(SIG_WEB, PAR_WEB_INIT_FAILURE);
		return;
        }

        // Start listening
        result = espconn_accept(&tcp_front_conn);      
        if (result < 0) {
                PRINT_DEBUG(DEBUG_ERR, "ERROR: failed to start tcp server, error=%d\r\n", result);
		TASK_RETURN(SIG_WEB, PAR_WEB_INIT_FAILURE);
		return;
        }

        return;
};

void ICACHE_FLASH_ATTR user_front_connect_cb(void *arg)
{
        PRINT_DEBUG(DEBUG_LOW, "client connected to webserver front end\r\n");
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
        PRINT_DEBUG(DEBUG_ERR, "tcp connection error occured\r\n");
	return;
};

void ICACHE_FLASH_ATTR user_front_discon_cb(void *arg)
{
        PRINT_DEBUG(DEBUG_LOW, "tcp connection disconnected\r\n");
	return;
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
	sint8 result = 0;			// API call result

        PRINT_DEBUG(DEBUG_LOW, "received data from client\r\n");
        PRINT_DEBUG(DEBUG_HIGH, "data=%s\r\n", pusrdata);

        
        // Check if we recieved an HTTP GET request
        if (os_strncmp(pusrdata, "GET ", 4) == 0) {
                PRINT_DEBUG(DEBUG_HIGH, "http GET request detected\r\n");

                // Check if the request is for a web socket
                if (os_strstr(pusrdata, ws_header)) {

                        // Get the secure key from the client data
                        p1 = (uint8 *)os_strstr(pusrdata, ws_key);      // Navigate to key line
                        p1 += os_strlen(ws_key);                        // Move to beginning of the actual key portion of the key line
                        p2 = (uint8 *)os_strstr(p1, "\r\n");            // Navigate to end of key
                        key_len = p2 - p1;
                        PRINT_DEBUG(DEBUG_HIGH, "found key of length %d\r\n", key_len);

                        // Concatenate the secure key with the GUID for WebSockets
                        if ((key_len + guid_len) < 64) {
                                os_memcpy(key, p1, key_len);                            // First half = client key
                                os_memcpy(&key[key_len], ws_guid, guid_len);            // Second half = GUID        
                                PRINT_DEBUG(DEBUG_HIGH, "concat_key=%s\r\n", key);    
                        } else {
                                PRINT_DEBUG(DEBUG_ERR, "failed to create websocket response\r\n");
                                return;
                        } 

                        // Calculate SHA-1 Hash
                        os_memset(sha1_sum, 0, 20);
                        mbedtls_sha1(key, guid_len + key_len, sha1_sum);    
                        PRINT_DEBUG(DEBUG_HIGH, "sha1_hash=%s\r\n", sha1_sum);
                        uint16 i = 0;
                        for (i = 0; i < 20; i++){
                                PRINT_DEBUG(DEBUG_HIGH, "%x:", sha1_sum[i]);
                        }
                        PRINT_DEBUG(DEBUG_HIGH, "\r\n");

                        // Encode SHA-1 Hash in base 64
                        mbedtls_base64_encode(NULL, 0, &olen, sha1_sum, 20);                                    // Buffer length needed for Base64 encoding -> olen
                        if (mbedtls_base64_encode(sha1_key, sizeof(sha1_key), &olen, sha1_sum, 20) == 0) {      // Encode in base 64
                       
                                // Form WebSocket request response & send
                                sha1_key[olen] = '\0'; 
                                PRINT_DEBUG(DEBUG_HIGH, "base64=%s\r\n", sha1_key);
                                response_len = os_sprintf(response_buf, ws_response, sha1_key); 
                                espconn_send(client_conn, response_buf, response_len);
                                PRINT_DEBUG(DEBUG_HIGH, "sent=%s\r\n", response_buf);                         

                                // The connection has upgraded to a WebSocket. Apply WebSocket callbacks for data transmission
                                espconn_regist_recvcb(client_conn, user_ws_recv_cb);

				// Upgrade the keep-alive time for the WebSocket to 30 minutes (1800 seconds)
				result = espconn_regist_time(client_conn, 1800, true);
				if (result < 0) {
					PRINT_DEBUG(DEBUG_ERR, "ERROR: failed to upgrade timeout interval for WebSocket\r\n");
				}

                                // Arm WebSocket update timer
                                os_timer_setfn(&ws_timer, user_ws_update, client_conn);
                                os_timer_arm(&ws_timer, WS_UPDATE_TIME, 1);
                         
                        } else {
                                PRINT_DEBUG(DEBUG_ERR, "failed to create websocket response\r\n");
                                return;
                        }

                // The request is ordinary
                } else {
                        PRINT_DEBUG(DEBUG_LOW, "sending front page\r\n");
                        espconn_send(client_conn, (uint8 *)front_page, os_strlen(front_page)); 
                }
        }

        return;
};

void ICACHE_FLASH_ATTR user_front_sent_cb(void *arg)
{
        PRINT_DEBUG(DEBUG_LOW, "sent to client\r\n");
};

void ICACHE_FLASH_ATTR user_ws_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
        struct espconn *client_conn = arg;      // Grab connection
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
                PRINT_DEBUG(DEBUG_ERR, "received malformed ws packet\r\n");
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
                        for (i = 0; i < (length - mask_start - 4); i++) {
                                pusrdata[mask_start + 4 + i] ^= mask[i % 4];
                        }
                        os_printf("received masked packet=%s\r\n", &pusrdata[mask_start + 4]);
			user_ws_parse_data(&pusrdata[mask_start + 4], length - mask_start - 4);
                        break;
        }; 
        return;
};

void ICACHE_FLASH_ATTR user_ws_update(void *parg)
{
        sint8 result = 0;                       // Function result
        struct espconn *ws_conn = parg;         // Grab WebSocket connection
        uint8 data[14];                         // Packet data
        os_memset(&data, 0, 14);                  

        // Contruct packet. See user_sw_recv_cb for information on WebSocket packet structure.
        //      Packets from the server should never be masked


        // Contruct bytes 1 & 2
        data[0] = (0x80) | (0x02);     // Unfragmented, binary data 
        data[1] = 0x0C;                // Unmasked, payload length 12 bytes  
        
        // Add humidity data
	os_memcpy(&data[2], &sensor_data_int, 4);
	os_memcpy(&data[6], &sensor_data_ext, 4);

	// Add measured RPM
	os_memcpy(&data[10], &measured_rpm, 4);

        // Send data to WebSocket
        result = espconn_send(ws_conn, data, 14);

        return;
};

void ICACHE_FLASH_ATTR user_endian_flip(uint8 *buf, uint8 n)
{
        uint8 swp = 0;  // Swap byte
        uint8 i = 0;    // Loop index

        // Swap each pair of bytes, moving in from the ends
        for (i = 0; i < (n/2); i++) {
                swp = buf[i];
                buf[i] = buf[n - 1 - i];
                buf[n - 1 - i] = swp;               
        };

        return;
};

void ICACHE_FLASH_ATTR user_ws_parse_data(uint8 *data, uint16 len)
{
	uint8 *p1 = NULL;	// Char pointer 1, for data navigation
	uint8 *p2 = NULL;	// Char pointer 2, for data navigation
	uint32 speed = 0;	// Fan speed in RPM

	// Search for each config element
	p1 = (uint8 *)os_strstr(data, "speed=");		// Locate speed element
	if (p1 != NULL) {
		p1 += 6;						// Move to end of 6 char substr "speed="
		p2 = (uint8 *)os_strstr(p1, ",");			// Find end of speed element value (CSV)
		speed = user_atoi(p1, p2 - p1);				// Extract integer fan RPM
	
		speed > FAN_RPM_MAX ? (speed = FAN_RPM_MAX) : 0;	// Cut speed down to max if RPM is above maximum
		speed < FAN_RPM_MIN ? (speed = FAN_RPM_MIN) : 0;	// Bump speed up to min if the RPM is below minimum
		desired_rpm = speed;				// Modify triac delay 		
	}

	return;
};

uint32 ICACHE_FLASH_ATTR user_atoi(uint8 *str, uint16 len)
{
	uint32 val = 0;		// Converted value
	uint32 mult = 1;	// Multiplier for each digit
	sint8 i = 0;		// Loop index
	uint8 c = 0;		// Extracted character
	sint16 digit = 0;	// Converted digit 

	// A 32 bit unsigned integer can hold at most 10 digits
	if (len > 10) {
		os_printf("atoi conversion error, input too large\r\n");
		return 0;
	}

	// Convert each digit, working from the least significant
	for (i = (len - 1); i >= 0; i--) {
		digit = str[i] - 0x30;
		if ((digit < 0) || (digit > 9)) {	// Stop converting if we find a non-digit
			return val;
		}
		val += (digit * mult);
		mult *= 10;	
	}
	
	return val;
};
