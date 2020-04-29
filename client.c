#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>

void handle_connection(int network_socket){
	char buffer[4096];
	char message[1000000];
	int n;
	while(1){
		bzero(buffer, sizeof(buffer));
		bzero(message, sizeof(message));
		n = 0;
		printf("Enter message: ");
		while((buffer[n++] = getchar()) != '\n')
			;
		buffer[n-1] = '\0';
		write(network_socket, buffer, sizeof(buffer));
		read(network_socket, message, sizeof(message));
		printf("From server: %s", message);
		if((strncmp(message, "exit", 4)) == 0){
			printf("Client exit\n");
			break;
		}
	}
}

int main(int argc, char** argv){
	int port = atoi(argv[1]);
	int network_socket;
	network_socket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = INADDR_ANY;
	int connection_status = connect(network_socket, (struct sockaddr*) &server_address, sizeof(server_address));
	if(connection_status == -1){
		printf("Connection failed\n");
		return EXIT_FAILURE;
	}
	printf("Connected to the server\n");
	handle_connection(network_socket);
	close(network_socket);
	
	
	
	
	
	return EXIT_SUCCESS;
}
