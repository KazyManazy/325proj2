#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>

#define MAX_PENDING 1
#define MAX_LENGTH 1024

int main(int argc, char * argv[]) {

	char* clientPortString;
	int clientPortNumber;
	char homeIPAddress[] = "127.0.0.1";
	int homePortNumber = 23;
	//Structure used to represent the home server.
	struct hostent *homeServer;
	//Structure that represent the home IP Address
	struct sockaddr_in homeAddress;
	int homeSocketID;
	int homeConnectionCheck;

	struct sockaddr_in clientAddress;
	int clientSocketID;
	int clientBindCheck;
	int clientAddressLength;
	int newClientSocketID;
	int largestDescriptor;
	int numFileDescriptors;
	//Buffer between the cproxy and localhost.
	char buffer[MAX_LENGTH];
	int bufferLength;
	//Represents the set of sockets to be selected from.
	fd_set readfds;

	//Makes sure there is only one argument and it is the client port number, or the port number to listen to.
	if (argc==2) {
		clientPortString = argv[1];
		clientPortNumber = atoi(clientPortString);
  	}
  	else {
	    fprintf(stderr, "Need 1 argument when running this file: Client Port Number\n");
	    exit(1);
  	}


  	//Creates the hostent struct, but no lookup is performed since the hostIPAddress is an IPV4 address.
  	homeServer = gethostbyname(homeIPAddress);
  	//Checks to see of the hostServer hostent struct was initialized.
  	if(!homeServer) {
  		fprintf(stderr, "A error occurred trying to connect to localhost.\n");
  		exit(1);
  	}

  	// The following section builds the sockaddr_in struct for the server's address.
  	//This sets the entire serverAddress structure to 0.  
  	bzero((char *)&homeAddress, sizeof(homeAddress));
  	//Specifies that the address is in the internet domain with an IPv4 Address.
  	homeAddress.sin_family = AF_INET;
  	//Copies the hostServer's IPv4 Address to the serverAddress's sin_addr struct.
  	bcopy(homeServer->h_addr, (char *)&homeAddress.sin_addr, homeServer->h_length);
  	//Sets the port number for the server address and makes sure that it is stored in memory correctly (htons).
  	homeAddress.sin_port = htons(homePortNumber);

    //Creates a socket and socketID is the identifier to reference that socket.
  	//PF_INET and AF_INET are the same, but PF_INET is commonly used when calling socket.  Reference in Beej's Guide to Network Programming.
  	//SOCK_STREAM is for two-way byte streams.
  	//0 is given so the default socket protocol will be used.
  	homeSocketID = socket(PF_INET, SOCK_STREAM, 0);
  	if(homeSocketID < 0) {
  		perror("Error in creating the localhost socket: \n");
  		exit(1);
  	}

  	//Creates the connection with the server using the socket and server address.
  	homeConnectionCheck = connect(homeSocketID, (struct sockaddr *)&homeAddress, sizeof(homeAddress));
  	if (homeConnectionCheck < 0) {
  		perror("Error in connecting to the localhost socket: \n");
  		close(homeSocketID);
  		exit(1);
  	}



  	// This sections builds the sockaddr_in struct for the client's address
	//This sets the entire clientAddress to 0.
	bzero((char *)&clientAddress, sizeof(clientAddress));
	//Specifies that the address is an internet IPv4 address.
	clientAddress.sin_family = AF_INET;
	//Represents a wildcard address so that any outside address can connect.
	clientAddress.sin_addr.s_addr = INADDR_ANY;
	//Sets the port number for the client to connect with.
	clientAddress.sin_port = htons(clientPortNumber);

	//Creates a socket and socketID is the identifier to reference that socket.
	//PF_INET and AF_INET are the same, but PF_INET is commonly used when calling socket.
	//SOCK_STREAM is for two-way byte streams.
	//0 is given so the default socket protocol will be used.
	clientSocketID = socket(PF_INET, SOCK_STREAM, 0);
	if (clientSocketID < 0) {
		perror("Error in creating the client socket: \n");
		exit(1);
	}

	//binds a name (client address) to the socket.
	clientBindCheck = bind(clientSocketID, (struct sockaddr *)&clientAddress, sizeof(clientAddress));
	if (clientBindCheck < 0) {
		perror("Error in binding the client socket: \n");
		exit(1);
	}

	//Marks the socket as a passive socket to accept incoming connections.
	//MAX_PENDING is the backlog or the maximum number of pending connections.
	listen(clientSocketID, MAX_PENDING);

	//Accepts the first connection request from the queue of pending connections for the listening socket.
	newClientSocketID = accept(clientSocketID, (struct sockaddr *)&clientAddress, &clientAddressLength);
	if (newClientSocketID < 0) {
		perror("Error in accepting new home socket connection: \n");
		close(clientSocketID);
		close(homeSocketID);
		exit(1);
	}

	//Cleas the set of sockets to be selected from.
	FD_ZERO(&readfds);

	//Populates the set of sockets to be selected from.
	FD_SET(homeSocketID, &readfds);
	FD_SET(newClientSocketID, &readfds);

	//Finds the greatest socket id.
	if (homeSocketID > newClientSocketID) {
		largestDescriptor = homeSocketID + 1;
	}
	else {
		largestDescriptor = newClientSocketID + 1;
	}

	while(1) {

		//Cleas the set of sockets to be selected from.
		FD_ZERO(&readfds);

		//Populates the set of sockets to be selected from.
		FD_SET(homeSocketID, &readfds);
		FD_SET(newClientSocketID, &readfds);

		numFileDescriptors = select(largestDescriptor, &readfds, NULL, NULL, NULL);

		if(numFileDescriptors == -1) {
			perror("Error in select function on the client proxy.");
			close(homeSocketID);
  			close(clientSocketID);
  			close(newClientSocketID);
			exit(1);
		}
		else if (numFileDescriptors == 0) {
			printf("Timeout occured!  Error since timeout is set to NULL.");
			close(homeSocketID);
  			close(clientSocketID);
  			close(newClientSocketID);
			exit(1);
		}
		else {
			if (FD_ISSET(homeSocketID, &readfds)) {
				//TESTING
				//printf("RECV FROM SERVER\n");
				//TESTING
				bufferLength = recv(homeSocketID, buffer, MAX_LENGTH, 0);

				//Connection is closed
				if (bufferLength == 0) {
					close(homeSocketID);
		  			close(clientSocketID);
		  			close(newClientSocketID);
	    			exit(0);
				}

				if(bufferLength < 0) {
	    			perror("Error in keeping connected with the server: \n");
	    			close(homeSocketID);
		  			close(clientSocketID);
		  			close(newClientSocketID);
	    			exit(1);
	    		}
	    		send(newClientSocketID, buffer, bufferLength, 0);
			}
			if (FD_ISSET(newClientSocketID, &readfds)) {
				//TESTING
				//printf("RECV FROM LOCALHOST\n");
				//TESTING
				bufferLength = recv(newClientSocketID, buffer, sizeof(buffer), 0);

				if(bufferLength < 0) {
	    			perror("Error in keeping connected with the localhost: \n");
	    			close(homeSocketID);
		  			close(clientSocketID);
		  			close(newClientSocketID);
	    			exit(1);
	    		}

	    		send(homeSocketID, buffer, bufferLength, 0);
			}
			bzero(buffer, sizeof(buffer));
		}

  	}

  	close(homeSocketID);
  	close(clientSocketID);
  	close(newClientSocketID);
  	exit(0);
}