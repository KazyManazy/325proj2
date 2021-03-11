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

	char *hostIPAddress;
	char *hostPortString;
	int hostPortNumber;
	//Structure used to represent a host.
	struct hostent *hostServer;

	//localhost information for telnet.
	char *homePortString;
	int homePortNumber;
	char homeIPAddress[] = "127.0.0.1";
	struct hostent *homeServer;

	//Structures used to represent internet addresses.
	struct sockaddr_in serverAddress;
	struct sockaddr_in homeAddress;

	int serverSocketID;
	int homeSocketID;

	int serverConnectionCheck;
	int homeBindCheck;

	int newHomeSocketID;
	int homeAddressLength;

	int homeConnectionCheck;

	//Represents the set of sockets to be selected from.
	fd_set readfds;
	//Represents the largest socketID + 1.
	int largestDescriptor;
	//Buffer between the cproxy and localhost.
	char buffer[MAX_LENGTH];
	//The return value from select()
	int numFileDescriptors;

	int bufferLength;

	//Checks to make sure there is 3 arguments entered that will be the home port number, the host IP addressm and the port number.
	if (argc==4) {
		homePortString = argv[1];
		homePortNumber = atoi(homePortString);
    	hostIPAddress = argv[2];
    	hostPortString = argv[3];
    	hostPortNumber = atoi(hostPortString);
  	}
  	else {
	    fprintf(stderr, "Need 3 arguments when running this file: Local Port Number, Host IPV4 Address, Host Port Number.\n");
	    exit(1);
  	}

  	
  	//Creates the hostent struct, but no lookup is performed since the hostIPAddress is an IPV4 address.
  	hostServer = gethostbyname(hostIPAddress);
  	//Checks to see of the hostServer hostent struct was initialized.
  	if(!hostServer) {
  		fprintf(stderr, "A valid IPV4 Address was not entered as the second argument when running this file.\n");
  		exit(1);
  	}

  	// The following section builds the sockaddr_in struct for the server's address.
  	//This sets the entire serverAddress structure to 0.  
  	bzero((char *)&serverAddress, sizeof(serverAddress));
  	//Specifies that the address is in the internet domain with an IPv4 Address.
  	serverAddress.sin_family = AF_INET;
  	//Copies the hostServer's IPv4 Address to the serverAddress's sin_addr struct.
  	bcopy(hostServer->h_addr, (char *)&serverAddress.sin_addr, hostServer->h_length);
  	//Sets the port number for the server address and makes sure that it is stored in memory correctly (htons).
  	serverAddress.sin_port = htons(hostPortNumber);

  	//Creates a socket and socketID is the identifier to reference that socket.
  	//PF_INET and AF_INET are the same, but PF_INET is commonly used when calling socket.  Reference in Beej's Guide to Network Programming.
  	//SOCK_STREAM is for two-way byte streams.
  	//0 is given so the default socket protocol will be used.
  	serverSocketID = socket(PF_INET, SOCK_STREAM, 0);
  	if(serverSocketID < 0) {
  		perror("Error in creating the server socket: \n");
  		exit(1);
  	}

  	//Creates the connection with the server using the socket and server address.
  	serverConnectionCheck = connect(serverSocketID, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
  	if (serverConnectionCheck < 0) {
  		perror("Error in connecting to the server socket: \n");
  		close(serverSocketID);
  		exit(1);
  	}

  	

  	//Creates the hostent struct, but no lookup is performed since the homeIPAddress is an IPV4 address.
  	homeServer = gethostbyname(homeIPAddress);

  	//Checks to see of the hostServer hostent struct was initialized.
  	if(!homeServer) {
  		fprintf(stderr, "There was an issue connecting to the home IP address.\n");
  		exit(1);
  	}

  	// The following section builds the sockaddr_in struct for the home address.
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
  		perror("Error in creating the home socket: \n");
  		exit(1);
  	}

	//binds a name (home address) to the socket.
	homeBindCheck = bind(homeSocketID, (struct sockaddr *)&homeAddress, sizeof(homeAddress));
	if (homeBindCheck < 0) {
		perror("Error in binding the socket: \n");
		close(serverSocketID);
		close(homeSocketID);
		exit(1);
	}
	
	//Marks the socket as a passive socket to accept incoming connections.
	//MAX_PENDING is the backlog or the maximum number of pending connections.
	listen(homeSocketID, MAX_PENDING);

	//Accepts the first connection request from the queue of pending connections for the listening socket.
	newHomeSocketID = accept(homeSocketID, (struct sockaddr *)&homeAddress, &homeAddressLength);
	if (newHomeSocketID < 0) {
		perror("Error in accepting new home socket connection: \n");
		close(serverSocketID);
		close(homeSocketID);
		exit(1);
	}

	//Finds the greatest socket id.
	if (serverSocketID > newHomeSocketID) {
		largestDescriptor = serverSocketID + 1;
	}
	else {
		largestDescriptor = newHomeSocketID + 1;
	}
	while(1) {

		//Cleas the set of sockets to be selected from.
		FD_ZERO(&readfds);

		//Populates the set of sockets to be selected from.
		FD_SET(serverSocketID, &readfds);
		FD_SET(newHomeSocketID, &readfds);

		numFileDescriptors = select(largestDescriptor, &readfds, NULL, NULL, NULL);

		if(numFileDescriptors == -1) {
			perror("Error in select function on the client proxy.");
			close(serverSocketID);
  			close(homeSocketID);
  			close(newHomeSocketID);
			exit(1);
		}
		else if (numFileDescriptors == 0) {
			printf("Timeout occured!  Error since timeout is set to NULL.");
			close(serverSocketID);
  			close(homeSocketID);
  			close(newHomeSocketID);
			exit(1);
		}
		else {
			if (FD_ISSET(serverSocketID, &readfds)) {
				//TESTING
				//printf("RECV FROM SERVER\n");
				//TESTING
				bufferLength = recv(serverSocketID, buffer, MAX_LENGTH, 0);

				//Connection is closed
				if (bufferLength == 0) {
					close(serverSocketID);
		  			close(homeSocketID);
		  			close(newHomeSocketID);
	    			exit(0);
				}

				if(bufferLength < 0) {
	    			perror("Error in keeping connected with the server: \n");
	    			close(serverSocketID);
		  			close(homeSocketID);
		  			close(newHomeSocketID);
	    			exit(1);
	    		}
	    		send(newHomeSocketID, buffer, bufferLength, 0);
			}
			if (FD_ISSET(newHomeSocketID, &readfds)) {
				//TESTING
				//printf("RECV FROM LOCALHOST\n");
				//TESTING
				bufferLength = recv(newHomeSocketID, buffer, sizeof(buffer), 0);

				if(bufferLength < 0) {
	    			perror("Error in keeping connected with the localhost: \n");
	    			close(serverSocketID);
		  			close(homeSocketID);
		  			close(newHomeSocketID);
	    			exit(1);
	    		}

	    		send(serverSocketID, buffer, bufferLength, 0);
			}
			bzero(buffer, sizeof(buffer));
		}

  	}

  	close(serverSocketID);
  	close(homeSocketID);
  	close(newHomeSocketID);
  	exit(0);
}