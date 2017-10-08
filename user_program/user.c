#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int main(int arg, char *argv[])
{
	int socket_desc;		//used as socket descriptor
	char *bIP = "255.255.255.255";	//broadcast location
	int bPort = 20;			//currently arbitrary
	struct sockaddr_in bAddress;	//contains destination info

	//Sets the broadcast message
	char *message = "Hello";

	/* socket() opens a socket. AF_INET is the communication
	 * protocol used, SOCK_DGRAM is the type of socket, final argument
	 * is set automatically to match the communication protocol when 
	 * set to zero*/
	socket_desc = socket(AF_INET, SOCK_DGRAM, 0);

	//bAddress is cleared
	memset(&bAddress, '0', sizeof(bAddress));

	//sin_family member is set to AF_INET to match the socket
	bAddress.sin_family = AF_INET;

	//converts address to be broadcasted into data that the system 
	//will understand.
	inet_aton(bIP, &bAddress.sin_addr);

	//sets the port of the broadcast address
	bAddress.sin_port = htons(bPort);	


	//Broadcasts broadcast message: "Hello ESP"
	sendto(socket_desc, message, strlen(message), 0, (struct sockaddr *)&bAddress, sizeof(bAddress	));
	return 0;
}
