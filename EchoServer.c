#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 10609
#define SIZE 1000

typedef enum
{ 	
	Ok, 		// operation successful
	Bad, 		// unrecoverable error
	Wronglength // bad message length supplied
}Status;

typedef struct 
{
	unsigned int length;
	unsigned char data[SIZE];
} Message;

typedef struct sockaddr_in SocketAddress;

struct hostent *gethostbyname() ;
void printSA(struct sockaddr_in sa) ;
void makeDestSA(struct sockaddr_in * sa, char *hostname, int port) ;
void makeLocalSA(struct sockaddr_in *sa) ;
void server(int port) ;
void makeReceiverSA(SocketAddress *sa, int port);
void printSA(struct sockaddr_in sa);
void changeclientSA(SocketAddress *newclientSA);
Status GetRequest(Message *callMessage, int s, SocketAddress *clientSA);
Status SendReply(Message *replyMessage, int s, SocketAddress clientSA);
Status UDPsend(int s, Message *m, SocketAddress destination);
Status UDPreceive(int s, Message *m, SocketAddress *origin);



void main()
{
	int port=PORT, s=0, aLength=0, n=0;
	Status status=Ok, tempStatus;
	SocketAddress mySocketAddress, clientSocketAddress;
	Message message, replyMessage;
	memset(&message, 0, sizeof(message));
	memset(&replyMessage, 0, sizeof(replyMessage));
	//creates UDP socket, if fails, print error and return to main
	if((s = socket(AF_INET, SOCK_DGRAM, 0))<0) {
		perror("socket failed");
		return;
	}
	memset(&mySocketAddress, 0, sizeof(mySocketAddress));
	memset(&clientSocketAddress, 0, sizeof(clientSocketAddress));
	
	makeReceiverSA(&mySocketAddress, port);
	//Binds socket to the local port, if fails, closes the socket and returns to main
	if( bind(s, (struct sockaddr *)&mySocketAddress, sizeof(struct sockaddr_in))!= 0){
		perror("Bind failed\n");
		close(s);
		return;
	}
	//prints it's own socket address
	printSA(mySocketAddress);
	
	while(1)
	{
		memset(&replyMessage, 0, sizeof(replyMessage));
		aLength = sizeof(clientSocketAddress);
		clientSocketAddress.sin_family = AF_INET;
		//GetRequest picks message from socket and if status is Ok, print received message		
		if(GetRequest(&message, s, &clientSocketAddress)==Ok)
		{
     			printf("Received Message: %s\n", message.data);
		}
		//Echo the received message to client
		strncpy(replyMessage.data, message.data, strlen(message.data));
		replyMessage.length=strlen(replyMessage.data);
		Status tempStatus= SendReply(&replyMessage, s, clientSocketAddress);
		if(tempStatus==Bad)
		{
			printf("Something has gone wrong when replying to client\n");
		}
	}
}


//receives a request message and the client's socket address
Status GetRequest (Message *callMessage, int s, SocketAddress *clientSA)
{

	Status status=Bad;int n=0, len=sizeof(SocketAddress);
	if(UDPreceive(s, callMessage, clientSA) ==Ok) 
	{
		status=Ok;
	}
	else
	{
		printf("Get request: Could not receive message from client, Status is bad\n");
		status=Bad;	
	}
	//Exit server if client's message is 'q'
	if(strcmp(callMessage->data, "q") ==0)
	{
		printf("Exiting server..bye..\n");
		exit(-1);
	}
	return status; 
}

//receives a message and the socket address of the sender into two arguments
Status UDPreceive(int s, Message *m, SocketAddress *origin)
{
	Status status;
	int addrlen=sizeof(SocketAddress);
	int n=0;
	if((n = recvfrom(s, m,  sizeof(Message), 0, (struct sockaddr *)origin, &addrlen))> 0) 
	{
		//printf("UDP Receive: Message data is %s\n",m->data);
		status=Ok;
	}
	else
	{
		printf("UDP Receive: Status is bad\n");
		status=Bad;
	} 
	return status;
}

//sends a reply message to the given client's socket address
Status SendReply(Message *replyMessage, int s, SocketAddress clientSA)
{
	Status status;
	if( (UDPsend(s, replyMessage, clientSA)) == Ok)
		status=Ok;
		else status=Bad;
	return status;
}

//sends a given message through a socket to a given socket address
Status UDPsend(int s, Message *m, SocketAddress destination)
{
	Status status;int n;
	if( (n = sendto(s, m, sizeof(Message), 0, (struct sockaddr *)&destination, sizeof(destination))) > 0)
	{
		status=Ok;
		printf("Echoing message: %s\n", m->data);
	}		
	else 
	{
		printf("UDPsend: Could not send reply back to client, Status is bad\n");
		status=Bad;
	}
	return status;
}

// make a socket address using any of the addressses of this computer for a local socket on given port 
void makeReceiverSA(SocketAddress *sa, int port)
{
	sa->sin_family = AF_INET;
	printf("AF_INET:%d\n", AF_INET);
	sa->sin_port = htons(port);
	sa->sin_addr.s_addr = htonl(INADDR_ANY);
}


//print a socket address 
void printSA(SocketAddress sa)
{
	char mybuf[80];
	inet_ntop(AF_INET, &sa.sin_addr, mybuf, 80);
	printf("sa = %d, %s, %d\n", sa.sin_family, mybuf, ntohs(sa.sin_port));
}