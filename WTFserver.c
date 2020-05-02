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
#include <openssl/sha.h>

typedef struct file_nodes{
	int version;
	char* path;
	char* hash;
	struct file_nodes* next;
} file_node;

void* handle_connection(void* p_client_socket);
void get_message(int client_socket, char* project_name, int file_full, char* looking_for);
void upgrade(int client_socket, char* project_name);
void get_path(char* path, char* project_name, char* extension);
file_node* parse_manifest(int file);
void get_token(char* message, char* token, char delimeter);
int get_manifest_version(char* manifest_path);
int get_int_length(int num);
int get_file_list_length(file_node* head);
void free_file_node(file_node* head);

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
		printf("Connected to client\n\n");
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
		get_message(client_socket, strchr(buffer, ':')+1, 1, "Project");
	else if(strstr(buffer, "update") != NULL)
		get_message(client_socket, strchr(buffer, ':')+1, 0, "Manifest");
	else if(strstr(buffer, "upgrade") != NULL)
		upgrade(client_socket, strchr(buffer, ':')+1);
		
		
		
	/*
	 * 
	 * add other functions here
	 * 
	 * 
	 * */
	
	
	close(client_socket);
	printf("\nDisconnected from client\n");
	return NULL;
}

void get_message(int client_socket, char* project_name, int file_full, char* looking_for){
	int file;
	if((file = open(project_name, O_RDONLY)) == -1){
		printf("Project folder not found\n");
		write(client_socket, "Project folder not found", strlen("Project folder not found"));
		return;
	}
	close(file);
	char manifest_path[strlen(project_name)*2+11];
	get_path(manifest_path, project_name, "Manifest");
	if((file = open(manifest_path, O_RDONLY)) == -1){
		printf("Manifest not found\n");
		return;
	}
	printf("%s found, sending over...\n\n", looking_for);
	file_node* head = parse_manifest(file);
	close(file);
	int manifest_version = get_manifest_version(manifest_path);
	int file_list_length = get_file_list_length(head);
	int i;
	/*
	 * if contents of files are fully requested (file_full == 1) then
	 * the message that will be sent over will be in this format
	 * manifest version:# of files:file version:length of file path:file pathlength of hash:hashlength of file:filefile version...
	 * 
	 * if just manifest is requested (file_full == 0) then
	 * the message that will be sent over will be in this format
	 * manifest version:# of files:file version:length of file path:file pathlength of hash:hashfile version...
	 * */
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
		int path_length = strlen(tmp -> path);
		int hash_length = strlen(tmp -> hash);
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
		if(file_full){
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
			int buffer_length = strlen(buffer);
			char string6[get_int_length(buffer_length)+1];
			sprintf(string6, "%d", buffer_length);
			strcat(message, string6);
			strcat(message, ":");
			strcat(message, buffer);
			close(file);
		}
		tmp = tmp -> next;
	}
	write(client_socket, message, strlen(message));
	printf("%s sent\n", looking_for);
	free_file_node(head);
}

void upgrade(int client_socket, char* project_name){
	int file, manifest_file;
	char manifest_path[strlen(project_name)*2+11];
	get_path(manifest_path, project_name, "Manifest");
	if((file = open(project_name, O_RDONLY)) == -1){
		printf("Project folder not found\n");
		write(client_socket, "Project folder not found", sizeof("Project folder not found"));
		return;
	}
	else{
		int manifest_version = get_manifest_version(manifest_path);
		char string[get_int_length(manifest_version)+1];
		sprintf(string, "%d", manifest_version);
		write(client_socket, string, strlen(string));
	}
	close(file);
	if((manifest_file = open(manifest_path, O_RDONLY)) == -1){
		printf("Manifest not found\n");
		return;
	}
	char buffer[10000], message[10000];
	int bytes_read, file_tmp;
	file_node* head = parse_manifest(manifest_file);
	close(manifest_file);
	while(1){
		bzero(buffer, sizeof(buffer));
		bytes_read = read(client_socket, buffer, sizeof(buffer));
		if(bytes_read == -1 || strcmp(buffer, "Upgrade done") == 0 || (file_tmp = open(buffer, O_RDONLY)) == -1){
			if(bytes_read == -1)
				printf("Read failed\n");
			else if((file_tmp = open(buffer, O_RDONLY)) == -1 && !(strcmp(buffer, "Upgrade done") == 0))
				printf("File not found\n");
			free_file_node(head);
			return;
		}
		file_node* tmp = head;
		while(tmp != NULL && strcmp(tmp -> path, buffer) != 0)
			tmp = tmp -> next;
		if(tmp == NULL){
			printf("File not found\n");
			free_file_node(head);
			close(file_tmp);
			return;
		}
		bzero(buffer, sizeof(buffer));
		bytes_read = read(file_tmp, buffer, sizeof(buffer));
		bzero(message, sizeof(message));
		char string[get_int_length(tmp -> version)+1];
		sprintf(string, "%d", tmp -> version);
		strcpy(message, string);
		strcat(message, ":");
		if(tmp -> next == NULL)
			strcat(message, "NULL");
		else
			strcat(message, tmp -> next -> path);
		strcat(message, ":");
		strcat(message, buffer);
		write(client_socket, message, strlen(message));
		close(file_tmp);
	}
}

void get_path(char* path, char* project_name, char* extension){
	strcpy(path, project_name);
	strcat(path, "/");
	strcat(path, project_name);
	strcat(path, ".");
	strcat(path, extension);
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
	char buffer_tmp[strlen(strchr(buffer, '\n'))+1];
	strcpy(buffer_tmp, strchr(buffer, '\n')+1);
	bzero(buffer, strlen(buffer));
	strcpy(buffer, buffer_tmp);
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
			char* token = malloc(strchr(buffer, '\n')-buffer+1);
			if(count%3 == 0){
				get_token(buffer, token, ' ');
				tmp -> version = atoi(token);
			}
			else if(count%3 == 1){
				get_token(buffer, token, ' ');
				tmp -> path = token;
			}
			else if(count%3 == 2){
				get_token(buffer, token, '\n');
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

void get_token(char* message, char* token, char delimeter){
	char* copy = malloc(strlen(message)+1);
	strcpy(copy, message);
	copy[strchr(message, delimeter)-message] = '\0';
	int i;
	for(i = 0; i <= strlen(copy); i++)
		token[i] = copy[i];
	char* message_tmp = malloc(strlen(strchr(message, delimeter))+1);
	strcpy(message_tmp, strchr(message, delimeter)+1);
	bzero(message, strlen(message));
	strcpy(message, message_tmp);
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

void free_file_node(file_node* head){
	file_node* tmp;
	while(head!=NULL){
		tmp = head;
		head = head -> next;
		free(tmp);
	}
}
