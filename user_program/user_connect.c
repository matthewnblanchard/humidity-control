#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main(int arg, char *argv[])
{
	int broadSockDesc;		//broadcast socket descriptor
	int TCPSockDesc;		//TIP socket descriptor
	int error = 0;			//error checking
	int notReceived = 1;			//received data flag
	char *bAddr = "255.255.255.255";//Broadcast Address 
	int port = 5000;		//currently arbitrary
	struct sockaddr_in dAddress;	//communication info struct
	int broadTimer = 0;		//Timeout counter

	/*recieved address with which 
	 * the program will establish a TCP 
	 * connection*/
	char *TCPAddr = "127.0.0.1";	//set for now, won't be later 
	
	//Sets the broadcast message
	char *message = "Hello";

	/*Variable containing our broadcast permissions
	 * Permission Granted = 1; Permission Denied = 0;*/
	int broadcastPermission = 1;

	/* socket() opens a socket. First socket is to broadcast a message for the
	 * ESP-12S to find, and the second is to communicate*/
	if ((broadSockDesc = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("Broadcast socket error: %s\n", strerror(errno));
		return -1;
	}

	if ((TCPSockDesc = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("TCP socket error: %s\n", strerror(errno));
		return -1;
	}

	//Set permissions to allow broadcasting
	if (setsockopt(broadSockDesc, SOL_SOCKET, SO_BROADCAST, (void *)&broadcastPermission, sizeof(broadcastPermission)) < 0){
		printf("Socket Permissions error: %s\n", strerror(errno));
		return -1;
	}
	
	//dAddress is cleared
	memset(&dAddress, '0', sizeof(dAddress));

	//sin_family member is set to AF_INET to match the socket
	dAddress.sin_family = AF_INET;

	//converts address to be broadcasted into data that the system 
	//will understand.
	error = inet_aton(bAddr, &dAddress.sin_addr);
	if (error == 0){
		printf("inet_aton failure: %s\n", strerror(errno));
		return -1;
	}
	error = 0;


	//sets the port of the broadcast address
	
	dAddress.sin_port = htons(port);
	
	printf("lets send a message\n");
	while(notReceived){
		//Broadcast message in datagram
		printf("Broadcasting message...\n");
		error = sendto(broadSockDesc, message, strlen(message), 0, (struct sockaddr *)&dAddress, sizeof(dAddress));
		if (error < 0){
			printf("Send error: %s\n", strerror(errno));
		}	
		error = 0;
		
		sleep(2);
		broadTimer += 1;
		printf("Let's try again\n");
		/*Here is a section of code that has not been developed yet.
		 *It will take information that the ESP-12S has that is necessary
		 *for a TCP connection. */



		/*Hopefully we'll figure out how this is suppossed to go before
		 * too long, I'm a bit clueless at the moment and am just taking up
		 * space with this comment to keep the code clean looking*/
		
		if(broadTimer == 10){
			printf("Nothing saw my broadcast message :(\n");
			notReceived = 1;
		}	
	}


	//cast new address into dAddress data to establish TCP connection
	if (inet_aton(TCPAddr, &dAddress.sin_addr) == 0){
		printf("inet_aton failure: %s\n", strerror(errno));
		return -1;
	}
	
	//establish TCP connection with ESP-12S
	if (connect(TCPSockDesc, (struct sockaddr *)&dAddress, sizeof(dAddress)) !=0){
		printf("connection unsuccesful: %s\n", strerror(errno));
		return -1;
	}
	
	return 0;
}
