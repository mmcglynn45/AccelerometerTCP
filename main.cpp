/* A simple server in the internet domain using TCP
The port number is passed as an argument */

#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "IMU.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "endpoint.h"

#define portNum 4000

using namespace std;

endpoint accelEndpoint;
bool webThreadCreated = false;
pthread_t webThread;

void* webAccept(void * webSocketDataPointer);

struct webSocketData
{
	int socketFileDescriptor;
	IMU * imuRef;
	bool currentlyAccepting = false;
};

void error(const char *msg)
{
	perror(msg);
	exit(1);
}



int main(int argc, char *argv[])
{
	IMU piIMU;

	printf("PIIMU Setup starting...\n");
	piIMU.setup();
	printf("PIIMU Setup completed...\n");
	piIMU.pitchComp = 0;
	piIMU.rollComp = 0;
	piIMU.mXComp = 0;
	piIMU.mYComp = 0;
	for (int i = 0; i<200; i++) {
		while (!piIMU.updateIMU()) {}
	}
	printf("Initial measurement complete...\n");


	int sockfd, newsockfd, portno;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	printf("Opening TCP/IP socket...\n");
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	bzero((char *)&serv_addr, sizeof(serv_addr));
	portno = portNum;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
		sizeof(serv_addr)) < 0)
		error("ERROR on binding");
	listen(sockfd, 5);
	clilen = sizeof(cli_addr);
	printf("TCP socket created on port %i",portno);

	struct webSocketData webData;
	webData.socketFileDescriptor = sockfd;
	webData.imuRef = &piIMU;

	while (true) {
		while (!piIMU.updateIMU()) {}
		
		if (!webThreadCreated&&!webData.currentlyAccepting) {
			pthread_create(&webThread, NULL, webAccept, &webData);
			webThreadCreated = true;
		}

		if (webThreadCreated&&!webData.currentlyAccepting)
		{
			pthread_join(webThread, NULL);
			webThreadCreated = false;
		}

	}
	
	close(webData.socketFileDescriptor);
	return 0;
}

void* webAccept(void * webSocketDataPointer) {

	char buffer[256];
	int connectionFileDescriptor;
	struct webSocketData * myWebSocket;
	myWebSocket = (webSocketData*)webSocketDataPointer;

	myWebSocket->currentlyAccepting = true;

	sockaddr_in cli_addr;
	socklen_t clilen;

	clilen = sizeof(cli_addr);

	connectionFileDescriptor = accept(myWebSocket->socketFileDescriptor,
		(struct sockaddr *) &cli_addr,
		&clilen);

	if (connectionFileDescriptor < 0)
		error("ERROR on accept");

	int n;

	bzero(buffer, 256);
	n = read(connectionFileDescriptor, buffer, 255);

	if (n < 0) error("ERROR reading from socket");
	printf("Here is the message: %s\n", buffer);

	IMU * imuPointer;
	imuPointer = (IMU *)myWebSocket->imuRef;
	double roll = imuPointer->roll.getAverage();
	double pitch = imuPointer->pitch.getAverage();
	double yaw = imuPointer->yaw.getAverage();
	double mX = imuPointer->mX.getAverage();
	double mY = imuPointer->mY.getAverage();

	char response[100];
	bzero(response, sizeof(response));
	snprintf(response, sizeof(response), "Roll: %f, Pitch: %f, Yaw: %f, MX: %f, MY: %f", roll, pitch, yaw, mX, mY);

	n = write(connectionFileDescriptor, response, sizeof(response));

	myWebSocket->currentlyAccepting = false;
	 
	close(connectionFileDescriptor);
}

