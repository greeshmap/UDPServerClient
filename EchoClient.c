#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define PORT 10609
#define SIZE 1000

typedef struct 
{
	unsigned int length;
	unsigned char data[SIZE];
} Message;

typedef enum
{ 	
	Ok, 		// operation successful
	Bad, 		// unrecoverable error
	Wronglength // bad message length supplied
}Status;

typedef struct sockaddr_in SocketAddress;


struct hostent *gethostbyname() ;
void makeDestSA(SocketAddress * sa, char *hostname, int port) ;
void printSA(SocketAddress sa);
void makeLocalSA(SocketAddress *sa) ;
void makeDestSA(SocketAddress * sa, char *hostname, int port);
Status anyThingThere(int s);
Status UDPsend(int s, Message *m, SocketAddress destination);
Status UDPreceive(int s, Message *m, SocketAddress *origin);
Status DoOperation (Message *message, Message *reply, int s, SocketAddress serverSA);


void main(int argc, char **argv)
{
	//Server's host name must be entered as an argument else the program exits
	if(argv[1] == NULL){
		printf("Server host name not entered..exiting\n");
		exit(1);
	}
	
	int s, port=PORT, n=0;
	Message message1, message2;
	SocketAddress mySocketAddress, yourSocketAddress;
	memset(&message1, 0, sizeof(message1));  

	//Create a socket, if failed to, exit from the program
	if(( s = socket(AF_INET, SOCK_DGRAM, 0))<0) {
		perror("socket failed");
		return;
	}

	//call makeLocalSA to make a socket address using any of the addressses of this computer for a local socket on any port 
	makeLocalSA(&mySocketAddress);
	
	//Bind to the local port, if failes to, close the socket and then return
	if( bind(s, (struct sockaddr *)&mySocketAddress, sizeof(struct sockaddr_in))!= 0){
		perror("Bind failed\n");
		close (s);
		return;
	}
	
	//prints socket address  
	
	printSA(mySocketAddress);
	makeDestSA(&yourSocketAddress, argv[1], port);
	
	while(1)
	{
		printf("Enter the message to be sent to server\n");
		
	//Reads the message from user
    	fgets(message1.data, SIZE, stdin); 
    	char *pos;
    	if((pos=strchr(message1.data, '\n')) != NULL)
    	{
    	//If the message does not have null in the end, adds null
    		*pos = '\0';
    	}
    	else
    	{
    		printf("Input too long for buffer\n");
    		exit(1);
    	}
		message1.length=strnlen(message1.data, SIZE);
		
		//call DoOperation function: sends message to the server and waits till a reply is received from the server
		DoOperation(&message1, &message2, s, yourSocketAddress);
		
		//client waits for 2 seconds before sending next message: sends peridic requests
		sleep(2);
	}
}


/*sends a given request message to a given socket address and blocks until it returns with a reply message and returns status*/
Status DoOperation (Message *message, Message *reply, int s, SocketAddress serverSA)
{
	Status status=Ok;
	int n=0;
	SocketAddress receiveSA;
	memset(&receiveSA, 0, sizeof(receiveSA));
    	socklen_t addrlen = sizeof(receiveSA);
    
   	//if status returned by UDPsend is Ok, continue to wait for receive else set the status to bad
	if( (status=UDPsend(s, message, serverSA)) ==Ok)
	{
		printf("Message sent ..waiting for the server to reply\n");
	}		
	else
	{
		status=Bad;
		printf("Could not send the message from client, status Bad\n");
	}
	if(status==Ok)
	{
		int i=1;
		while(i<4)
		{
			if(anyThingThere(s)==Bad) 
			{
				//no available socket fd, status bad, send message again
				status=Bad;
				printf("Sending message again\n");
				UDPsend(s, message, serverSA);
			}
			else
			{
				status=Ok;
				//socket fd available, receives the message from server
				if((status= UDPreceive(s, reply, &serverSA)) == Ok)
				{
					printf("Message received from server: %s\n", reply->data);
				}
				else
				{
					printf("No message received from server\n");
					status=Bad;
				}
				break;
			}
			i++;				
		}
		
	}
	if(status==Bad) // socket fd not available after checking 3 times also. Client program displays message and exits
	{	
		printf("Timeout, no response from server. Exiting client..\n");
		exit(1);
	}	return status;	
}


// use select to test whether there is any input on descriptor
Status anyThingThere(int s)
{
	Status timeoutstatus=Ok;
	fd_set rfds;
	struct timeval timeout;
	int n;
	FD_ZERO(&rfds);
        FD_SET(s, &rfds);
	timeout.tv_sec =3; /*seconds wait*/
	timeout.tv_usec = 0;        /* micro seconds*/
	if((n = select(s+1, &rfds, 0, 0, &timeout))<0)
		perror("Select fail:\n");
	else if(!n)
	{
		timeoutstatus=Bad;
	}
	return timeoutstatus;		
}

/* sends a given message through a socket to a given socket address and returns status*/
Status UDPsend(int s, Message *m, SocketAddress destination)
{
	Status status;int n;
	if( (n = sendto(s, m, sizeof(Message), 0, (struct sockaddr *)&destination, sizeof(destination))) > 0)
	{
		status=Ok;
	}		
	else 
	{

		printf("UDPsend: Could not send reply back to client, Status is bad\n");
		status=Bad;
	}
	return status;
}

/*receives a message and the socket address of the sender and returns status*/
Status UDPreceive(int s, Message *m, SocketAddress *origin)
{
	Status status;
	int addrlen=sizeof(SocketAddress);
	int n=0;
	if((n = recvfrom(s, m,  sizeof(Message), 0, (struct sockaddr *)origin, &addrlen)) > 0) 
	{
		status=Ok;
	}
	else
	{
		printf("UDP Receive: Status is bad\n");
		status=Bad;
	} 
	return status;
}



/* make a socket address using any of the addressses of this computer
for a local socket on any port */
void makeLocalSA(SocketAddress *sa)
{
	sa->sin_family = AF_INET;
	sa->sin_port = htons(0);
	sa-> sin_addr.s_addr = htonl(INADDR_ANY);
}


/*print a socket address */
void printSA(SocketAddress sa)
{
	char mybuf[80];
	inet_ntop(AF_INET, &sa.sin_addr, mybuf, 80);
	printf("sa = %d, %s, %d\n", sa.sin_family, mybuf, ntohs(sa.sin_port));
}


/* make a socket address for a destination whose machine and port are given as arguments */
void makeDestSA(SocketAddress * sa, char *hostname, int port)
{
	struct hostent *host;

	sa->sin_family = AF_INET;
	if((host = gethostbyname(hostname))== NULL){
		printf("Unknown host name\n");
		exit(-1);
	}
	sa-> sin_addr = *(struct in_addr *) (host->h_addr);
	sa->sin_port = htons(port);
}



