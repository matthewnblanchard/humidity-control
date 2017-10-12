#include<stdio.h>
#include<sys/socket.h>
#include<inet.h>

int main(int arg, char *argv[])
{
	int socket_desc;

	//Sets the broadcast message
	char *message = "Hello ESP";

	//creates new socket
	socket_desc = socket(AF_INET, SOCK_DGRAM, 0);

	//Broadcasts broadcast message: "Hello ESP"
	sendto(socket_desc, message, strlent(message), 0, __, __);
