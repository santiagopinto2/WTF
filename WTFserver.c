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

typedef struct file_nodes{
	int version;
	char* path;
	char* hash;
	struct file_nodes* next;
} file_node;

void* handle_connection(void*);
void checkout(int, char*);
file_node* parse_manifest(int);
int get_int_length(int);
int get_file_list_length(file_node*);

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
		return;
	}
	if((listen(server_socket, 100)) == -1){
		printf("Listen failed\n");
		return;
	}
	int client_socket, client_addr_size;
	struct sockaddr_in client_addr;
	while(1){
		client_addr_size = sizeof(struct sockaddr_in);
		if((client_socket = accept(server_socket, (struct sockaddr*) &client_addr, (socklen_t*) &client_addr_size)) == -1){
			printf("Accept failed\n");
			return;
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

void* handle_connection(void* p_client_socket){	
	int client_socket = *((int*)p_client_socket);
	free(p_client_socket);
	char buffer[1000];
	bzero(buffer, sizeof(buffer));
	int bytes_read;
	bytes_read = read(client_socket, buffer, sizeof(buffer));
	if(bytes_read == -1){
		printf("Read failed\n");
		return NULL;
	}
	if(strstr(buffer, "checkout") != NULL)
		checkout(client_socket, strchr(buffer, ':')+1);
		
	/*
	 * 
	 * add other functions here
	 * 
	 * 
	 * */
	
	printf("Disconnecting from client\n");
	close(client_socket);
	return NULL;
}

void checkout(int client_socket, char* project_name){
	int file;
	if((file = open(project_name, O_RDONLY)) == -1){
		printf("Project folder not found\n");
		return;
	}
	printf("Project found, sending over...\n");
	char manifest_path[strlen(project_name)*2+11];
	manifest_path[strlen(manifest_path)] = '\0';
	strcpy(manifest_path, project_name);
	strcat(manifest_path, "/");
	strcat(manifest_path, project_name);
	strcat(manifest_path, ".Manifest");
	if((file = open(manifest_path, O_RDONLY)) == -1){
		printf("Manifest not found\n");
		return;
	}
	file_node* head = parse_manifest(file);
	close(file);
	//need to implement check for no files, just manifest
	int manifest_version = get_manifest_version(manifest_path);
	int file_list_length = get_file_list_length(head);
	int i;
	//the message that will be sent over will be in this format
	//manifest version:# of files:file version:length of file path:file pathlength of hash:hashlength of file:file   file version...
	char message[1000000];
	bzero(message, sizeof(message));
	char string[get_int_length(manifest_version)+1];
	sprintf(string, "%d", manifest_version);
	strcpy(message, string);
	strcat(message, ":");
	if(head -> version == -1){
		strcat(message, "0");
		write(client_socket, message, strlen(message));
		return;
	}
	char string2[get_int_length(file_list_length)+1];
	sprintf(string2, "%d", file_list_length);
	strcat(message, string2);
	strcat(message, ":");
	file_node* tmp = head;
	for(i = 1; i <= file_list_length; i++){
		if((file = open(tmp -> path, O_RDONLY)) == -1){
			printf("File not found\n");
			return;
		}
		char buffer[1000000];
		bzero(buffer, sizeof(buffer));
		int bytes_read;
		bytes_read = read(file, buffer, sizeof(buffer));
		if(bytes_read == -1){
			printf("Read failed\n");
			return;
		}
		buffer[strlen(buffer)-1] = '\0';
		int path_length = strlen(tmp -> path);
		int hash_length = strlen(tmp -> hash);
		int buffer_length = strlen(buffer);
		char string3[get_int_length(tmp -> version)+1];
		sprintf(string3, "%d", tmp -> version);
		strcat(message, string3);
		strcat(message, ":");
		char string4[get_int_length(path_length)+1];
		sprintf(string4, "%d", path_length);
		strcat(message, string4);
		strcat(message, ":");
		strcat(message, tmp -> path);
		char string5[get_int_length(hash_length)+1];
		sprintf(string5, "%d", hash_length);
		strcat(message, string5);
		strcat(message, ":");
		strcat(message, tmp -> hash);
		char string6[get_int_length(buffer_length)+1];
		sprintf(string6, "%d", buffer_length);
		strcat(message, string6);
		strcat(message, ":");
		strcat(message, buffer);
		close(file);
		tmp = tmp -> next;
	}
	write(client_socket, message, strlen(message));
	printf("Project sent\n");
	
	//free head
}

file_node* parse_manifest(int file){
	char buffer[1000000];
	bzero(buffer, sizeof(buffer));
	int bytes_read;
	bytes_read = read(file, buffer, sizeof(buffer));
	if(bytes_read == -1){
		printf("Read failed\n");
		return NULL;
	}
	file_node* head = (file_node*)malloc(sizeof(file_node));
	strcpy(buffer, (strchr(buffer, '\n'))+1);
	if(strchr(buffer, '\n') == NULL){
		head -> version = -1;
		head -> path = NULL;
		head -> hash = NULL;
		head -> next = NULL;
	}
	else{
		int count = 0;
		file_node* tmp = head;
		while(strchr(buffer, '\n') != NULL){
			char copy[sizeof(buffer)+1];
			strcpy(copy, buffer);
			char* cut = strchr(copy, '\n');
			*cut = '\0';
			char* token = malloc(sizeof(copy)+1);
			int i;
			for(i = 0; i <= sizeof(copy); i++)
				token[i] = copy[i];
			strcpy(buffer, strchr(buffer, '\n')+1);
			if(count%3 == 0)
				tmp -> version = atoi(token);
			else if(count%3 == 1)
				tmp -> path = token;
			else if(count%3 == 2){
				tmp -> hash = token;
				if(strchr(buffer, '\n') == NULL){
					tmp -> next = NULL;
					return head;
				}
				file_node* new_file_node = (file_node*)malloc(sizeof(file_node));
				tmp -> next = new_file_node;
				tmp = tmp -> next;
			}
			count++;
		}
	}
	return head;
}

int get_manifest_version(char* manifest_path){
	int file; 
	if((file = open(manifest_path, O_RDONLY)) == -1)
		return -1;
	int flag;
	int version = 0;
	char buffer;
	while((flag = read(file, &buffer, sizeof(buffer))) > 0){
		if(buffer == '\n'){
			close(file);
			return version;
		}
		version = version*10+buffer-48;
	}
	close(file);
	return -2;
}

int get_int_length(int num){
	int a=1;
	while(num>9){
		a++;
		num/=10;
	}
	return a;
}

int get_file_list_length(file_node* head){
	int count = 1;
	file_node* tmp = head;
	while(tmp -> next != NULL){
		count++;
		tmp = tmp -> next;
	}
	return count;
}
