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

void configure(char*, char*);
void checkout(int, char*);

int main(int argc, char** argv){
	if(strcmp(argv[1], "configure") == 0)
		configure(argv[2], argv[3]);
	else{
		int ip_port_file=open("ip_port.configure", O_RDONLY);
		char buffer[100];
		bzero(buffer, 100);
		int bytes_read;
		bytes_read = read(ip_port_file, buffer, sizeof(buffer));
		if(bytes_read == -1){
			printf("Fetching the IP and port failed\n");
			return EXIT_FAILURE;
		}
		int n_index = strchr(buffer, '\n')-buffer;
		int n_size = sizeof(strchr(buffer, '\n'));
		char ip[n_index+1];
		strncpy(ip, buffer, n_index);
		ip[n_index] = '\0';
		char port[n_size];
		strncpy(port, buffer+n_index+1, n_size);
		port[n_size] = '\0';
		int network_socket;
		if((network_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
			printf("Socket creation failed\n");
			return EXIT_FAILURE;
		}
		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(atoi(port));
		/*if((inet_pton(AF_INET, INADDR_ANY, &(server_address.sin_addr))) == -1){
			printf("Invalid address\n");
			return EXIT_FAILURE;
		}*/
		server_address.sin_addr.s_addr = INADDR_ANY;
		int connection_status = connect(network_socket, (struct sockaddr*) &server_address, sizeof(server_address));
		if(connection_status == -1){
			printf("Connection failed\n");
			return EXIT_FAILURE;
		}
		printf("Connected to the server\n");
		
		
		
	
		if(strcmp(argv[1], "checkout") == 0)
			checkout(network_socket, argv[2]);
		/*
		 * 
		 * 
		 * add other functions here
		 * 
		 * 
		 * */
		
		
		
		
			
			
		close(network_socket);
	}
	return EXIT_SUCCESS;
}

void configure(char* string_ip, char* string_port){
	int ip_port_file = creat("./ip_port.configure", S_IRWXG|S_IRWXO|S_IRWXU);
	int write_int;
	write_int = write(ip_port_file, string_ip, strlen(string_ip));
	write_int = write(ip_port_file, "\n", 1);
	write_int = write(ip_port_file, string_port, strlen(string_port));
}

void checkout(int network_socket, char* project_name){
	return;
}
