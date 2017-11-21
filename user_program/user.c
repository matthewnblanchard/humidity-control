#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

int main(int arg, char *argv[])
{
	int socket_desc;		//used as socket descriptor
	int error = 0;			//error checking
	char *bIP = "255.255.255.255";	//broadcast location
	int bPort = 50000;		//currently arbitrary
	struct sockaddr_in bAddress;	//contains destination info

	//Sets the broadcast message
	char *message = "Hello";

	/*Variable containing our broadcast permissions
	 * Permission Granted = 1; Permission Denied = 0;*/
	int broadcastPermission = 1;

	/* socket() opens a socket. AF_INET is the communication
	 * protocol used, SOCK_DGRAM is the type of socket, final argument
	 * is set automatically to match the communication protocol when 
	 * set to zero*/
	if ((socket_desc = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("socket error\n");
		return -1;
	}

	if (setsockopt(socket_desc, SOL_SOCKET, SO_BROADCAST, (void *)&broadcastPermission, sizeof(broadcastPermission)) < 0){
		printf("Socket Permissions error: %s\n", strerror(errno));
		return -1;	       
	}
	
	//bAddress is cleared
	memset(&bAddress, '0', sizeof(bAddress));

	//sin_family member is set to AF_INET to match the socket
	bAddress.sin_family = AF_INET;

	//converts address to be broadcasted into data that the system 
	//will understand.
	error = inet_aton(bIP, &bAddress.sin_addr);
	if (error == 0){
		printf("inet_aton failure\n");
		return -1;
	}
	error = 0;


	//sets the port of the broadcast address
	
	bAddress.sin_port = htons(bPort);	
	error = htons(bPort);
	printf("%d\n", error);
	error = 0;

	//Broadcast message in datagram
	error = sendto(socket_desc, message, strlen(message), 0, (struct sockaddr *)&bAddress, sizeof(bAddress));
	printf("Before - Send error: %s\n", strerror(errno));
	if (error < 0){
		printf("After - Send error: %s\n", strerror(errno));
		return -1;
	}
	
	return 0;
}
