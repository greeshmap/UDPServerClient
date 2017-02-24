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


typedef struct
{		
	enum {Request, Reply} messageType;  	/* same size as an unsigned int */
	unsigned int RPCId; 			/* unique identifier */
	unsigned int procedureId; 		/* (1,2,3,4,5,6) for (+, -, *, /,Ping,Stop) */
	int arg1; 				/* argument/ return parameter */
	int arg2; 				/* argument/ return parameter */
} RPCMessage; 


typedef enum
{ 	
	Ok, 					// operation successful
	Bad, 					// unrecoverable error
	Wronglength,				// bad message length supplied
	DivZero					// divide by zero exception
}Status;

typedef struct 
{
	unsigned int length;
	unsigned char data[SIZE];
} Message;

typedef struct sockaddr_in SocketAddress;
static int flag=0;

struct hostent *gethostbyname() ;
void printSA(struct sockaddr_in sa) ;
void makeDestSA(struct sockaddr_in * sa, char *hostname, int port) ;
void makeLocalSA(struct sockaddr_in *sa) ;
void server(int port) ;
void makeReceiverSA(SocketAddress *sa, int port);
void printSA(struct sockaddr_in sa);
void changeclientSA(SocketAddress *newclientSA);
void marshal(RPCMessage *rm, Message *message);
void unMarshal(RPCMessage *rm, Message *message);
Status GetRequest(Message *callMessage, int s, SocketAddress *clientSA);
Status SendReply(Message *replyMessage, int s, SocketAddress clientSA);
Status UDPsend(int s, Message *m, SocketAddress destination);
Status UDPreceive(int s, Message *m, SocketAddress *origin);
int myAtoi(char *str);
Status add( int a, int b, int *c) ;
Status subtract( int a, int b, int *c) ;
Status multiply( int a, int b, int *c) ;
Status divide( int a, int b, int *c) ;
char* tostring(int num) ;



void main()
{
	int port=PORT, s=0, aLength=0, n=0;
	Status status=Ok, tempStatus;
	SocketAddress mySocketAddress, clientSocketAddress;
	RPCMessage rpcReqMessage, rpcReplyMessage;
	Message reqMessage;
	memset(&rpcReqMessage, 0, sizeof(rpcReqMessage));
	memset(&rpcReplyMessage, 0, sizeof(rpcReplyMessage));
	memset(&reqMessage, 0, sizeof(reqMessage));
	//If socket creation fails, then exits the program
	if((s = socket(AF_INET, SOCK_DGRAM, 0))<0) {
		perror("socket failed");
		return;
	}
	memset(&mySocketAddress, 0, sizeof(mySocketAddress));
	memset(&clientSocketAddress, 0, sizeof(clientSocketAddress));

	//call to makereceiverSA: Make a socket Address using any of the addressses of this computer for a local socket on given port
	makeReceiverSA(&mySocketAddress, port);

	//Binds socket to a local port and returns if bind fails
	if( bind(s, (struct sockaddr *)&mySocketAddress, sizeof(struct sockaddr_in))!= 0){
		perror("Bind failed\n");
		close(s);
		return;
	}
	//prints it's own socket address
	printSA(mySocketAddress);
	while(1)
	{		
		aLength = sizeof(clientSocketAddress);
		clientSocketAddress.sin_family = AF_INET;
	
		//call to GetRequest: Gets back unmarshalled message, returns status	
		if(GetRequest(&reqMessage, s, &clientSocketAddress)==Ok)
		{
     			printf("Request picked from GetRequest\n");
		}
		else
		{	
			printf("Requesting or Sending messagesg was not good\n");
		}
		Status tempStatus= SendReply(&reqMessage, s, clientSocketAddress);
		char *temp1="Stop";
		//If stop message is received from the client, exit after sending reply
		if(flag==6) 
		{
			printf("Exiting because of stop message from client\n");
			exit(1);
		}
	}
}


// Converts given Message into RPCMessage. Uses hton byte ordering
void marshal(RPCMessage *rm, Message *message)
{
	rm->messageType =Reply;
	rm->RPCId= htons(message->length);
	rm->arg1 = htonl(myAtoi((char *) message->data));
	if(flag==6) rm->procedureId=htons(6);
	if(flag==5) rm->procedureId=htons(5);
}

// Converts given RPCMessage into Message. Uses ntoh byte ordering
void unMarshal(RPCMessage *rm, Message *message)
{
	int c=0;	
	if(rm->messageType==Request)
	{
		int ch=ntohs(rm->procedureId);
	
		switch(ch) {
		//Add operation
      		case 1 :if(add(ntohs(rm->arg1), ntohs(rm->arg2), &c)==Ok)
         			printf("Add operation done!\n" );
				char* temp= tostring(c);
         			strncpy(message->data, temp, strlen(temp));
         			break;
		//Subtract operation
      		case 2 :if(subtract(ntohs(rm->arg1), ntohs(rm->arg2), &c)==Ok)
         			printf("Subtract operation done!\n" );
         			strcpy(message->data, tostring(c));
         			break;
		//Multiply operation
      		case 3 :if(multiply(ntohs(rm->arg1),ntohs(rm->arg2), &c)==Ok)
         			printf("Multiplication operation done!\n" );
         			strcpy(message->data, tostring(c));
         			break; 
		//Division operation       
     		case 4 :if(divide(ntohs(rm->arg1), ntohs(rm->arg2), &c)==Ok)
			{
         			printf("Division operation done!\n" );
         			strcpy(message->data, tostring(c));
			}
			else
			{
				printf("Divide by zero exception!! Returning 0\n");
			}
         			break;
		//Ping message from client, sets flag to 5, sends back Ok and continues
         	case 5: printf("Server running\n");
         			strncpy(message->data, "Ok", strlen("Ok"));
				flag=5;
         			break;
		//Stop message from client, sets flag to 6, sends back Ok and terminates
         	case 6: printf("Exiting server\n");
         			strncpy(message->data, "Ok", strlen("Ok"));
				flag=6;
         			break;
      		default:printf("No operator\n" );
   		}   		
   		message->length=ntohs(rm->RPCId);
	}

}

//Performs add operation and returns status Ok
Status add( int a, int b, int *c) 
{
	Status status=Ok;
	*c=a+b;
	return status;
}

//Performs subtract operation and returns status Ok
Status subtract( int a, int b, int *c) 
{
	Status status=Ok;
	*c=a-b;
	return status;
}

//Performs multiply operation and returns status Ok
Status multiply( int a, int b, int *c) 
{
	Status status=Ok;
	*c=a*b;
	return status;
}

//Performs division operation returns status 
Status divide( int a, int b, int *c) 
{
	Status status=Ok;
	if(b==0)
	{
		status=DivZero; // set status to Bad if denominator is a 0. Sends 0 as result to client
	}
	else
	{
		*c=a/b;
	}
	return status;
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
	return status; 
}

//receives a message and the socket address of the sender into two arguments
Status UDPreceive(int s, Message *m, SocketAddress *origin)
{
	Status status;
	int addrlen=sizeof(SocketAddress);
	int n=0;
	Message msg;
	memset(&msg, 0, sizeof(msg));
	RPCMessage rmsg;
	memset(&rmsg, 0, sizeof(rmsg));
	if((n = recvfrom(s, &rmsg,  sizeof(RPCMessage), 0, (struct sockaddr *)origin, &addrlen))> 0) 
	{
		status=Ok;
		unMarshal(&rmsg, m);
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
	{
		status=Ok;
	}
	else status=Bad;
	return status;
}

//sends a given message through a socket to a given socket address
Status UDPsend(int s, Message *m, SocketAddress destination)
{
	Status status;int n;
	RPCMessage rmsg;
	memset(&rmsg, 0, sizeof(rmsg));
	marshal(&rmsg, m);
	if( (n = sendto(s, (RPCMessage *)&rmsg, sizeof(RPCMessage), 0, (struct sockaddr *)&destination, sizeof(destination))) > 0)
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

//Converts given interger into string and returns the same
char* tostring(int num)
{
	char *buffer;
	buffer=(char *)malloc(100);
    	int i, rem, len = 0, n;
 
   	n = num;
    	while (n != 0)
    	{
        	len++;
        	n /= 10;
    	}
    	for (i = 0; i < len; i++)
    	{
        	rem = num % 10;
        	num = num / 10;
        	buffer[len - (i + 1)] = rem + '0';
    	}
    	buffer[len] = '\0';
	return buffer;
}



/* make a socket address using any of the addressses of this computer
for a local socket on given port */
void makeReceiverSA(SocketAddress *sa, int port)
{
	sa->sin_family = AF_INET;
	printf("AF_INET:%d\n", AF_INET);
	sa->sin_port = htons(port);
	sa->sin_addr.s_addr = htonl(INADDR_ANY);
}


/*print a socket address */
void printSA(SocketAddress sa)
{
	char mybuf[80];
	inet_ntop(AF_INET, &sa.sin_addr, mybuf, 80);
	printf("sa = %d, %s, %d\n", sa.sin_family, mybuf, ntohs(sa.sin_port));
}

//Converts given string into an integer and returns the same
int myAtoi(char *str)
{
    int res = 0;   
    // Iterate through all characters of input string and
    // update result
    for (int i = 0; str[i] != '\0'; ++i)
        res = res*10 + str[i] - '0';
  
    // return result.
    return res;
}

