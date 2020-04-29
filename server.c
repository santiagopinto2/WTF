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

void* handle_connection(void* p_client_socket){
	int client_socket = *((int*)p_client_socket);
	free(p_client_socket);
	char buffer[4096];
	char message[1000000];
	size_t bytes_read;
	int msgsize = 0;
	char actualpath[PATH_MAX+1];
	while(1){
		bzero(buffer, 4096);
		bzero(message, 1000000);
		msgsize = 0;
		bytes_read = read(client_socket, buffer, sizeof(buffer));
		if(bytes_read == -1){
			printf("Read failed\n");
			break;
		}
		printf("Request: %s\n", buffer);
		char pathname[strchr(buffer, '\0')-buffer+2];
		pathname[0] = '.';
		pathname[1] = '/';
		pathname[2] = '\0';
		strcat(pathname, buffer);
		pathname[strchr(buffer, '\0')-buffer+2]='\0';
		int file = open(pathname, O_RDONLY);
		if(file == -1){
			perror("");
			break;
		}
		/*while((bytes_read = read(file, &buffer2, sizeof(buffer2))) > 0){
			printf("%c", buffer2);
			write(client_socket, &buffer2, 1);
		}*/
		while((bytes_read = read(file, message+msgsize, sizeof(message)-msgsize-1)) > 0)
			msgsize += bytes_read;
		write(client_socket, message, sizeof(message));
		close(file);
		printf("File sent\n");
	}
	close(client_socket);
	return NULL;
}

int main(int argc, char** argv){
	int port = atoi(argv[1]);
	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	if((bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr))) == -1){
		printf("Bind failed\n");
		return EXIT_FAILURE;
	}
	if((listen(server_socket, 100)) == -1){
		printf("Listen failed\n");
		return EXIT_FAILURE;
	}
	int client_socket, client_addr_size;
	struct sockaddr_in client_addr;
	while(1){
		client_addr_size = sizeof(struct sockaddr_in);
		if((client_socket = accept(server_socket, (struct sockaddr*) &client_addr, (socklen_t*) &client_addr_size)) == -1){
			printf("Accept failed\n");
			return EXIT_FAILURE;
		}
		printf("Connected\n");
		pthread_t t;
		int* pclient = malloc(sizeof(int));
		*pclient = client_socket;
		pthread_create(&t, NULL, handle_connection, pclient);
	}
	close(server_socket);
	
	
	
	
	
	return EXIT_SUCCESS;
}
