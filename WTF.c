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

void configure(char* string_ip, char* string_port);
void checkout(int network_socket, char* project_name);
void update(int network_socket, char* project_name);
void update_write(int file, char c, char* path, char* hash, int* has_update);
void get_path(char* path, char* project_name, char* extension);
file_node* parse_manifest(int file);
file_node* parse_message(char* message);
void get_token(char* message, char* token, char delimeter);
int get_manifest_version(char* manifest_path);
int get_file_list_length(file_node* head);
void free_file_node(file_node* head);

int main(int argc, char** argv){
	if(strcmp(argv[1], "configure") == 0)
		configure(argv[2], argv[3]);
	else{		
		int ip_port_file=open("ip_port.configure", O_RDONLY);
		if(ip_port_file == -1){
			printf("Configure file not found\n");
			return EXIT_FAILURE;
		}
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
		if((inet_pton(AF_INET, ip, &(server_address.sin_addr))) == -1){
			printf("Invalid address\n");
			return EXIT_FAILURE;
		}
		int connection_status = connect(network_socket, (struct sockaddr*) &server_address, sizeof(server_address));
		if(connection_status == -1){
			printf("Connection failed\n");
			return EXIT_FAILURE;
		}
		printf("Connected to server\n");
		
		
		
	
		if(strcmp(argv[1], "checkout") == 0)
			checkout(network_socket, argv[2]);
		else if(strcmp(argv[1], "update") == 0)
			update(network_socket, argv[2]);
		
		/*
		 * 
		 * 
		 * add other functions here
		 * 
		 * 
		 * */
		
		
		
		
			
		close(network_socket);
		printf("Disconnected from server\n");
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
	struct stat st = {0};
	if(stat(project_name, &st) != -1){
		printf("Folder already exists client side\n");
		return;
	}
	mkdir(project_name, S_IRWXG|S_IRWXO|S_IRWXU);
	char buffer[strlen(project_name)+10];
	strcpy(buffer, "checkout:");
	strcat(buffer, project_name);
	write(network_socket, buffer, sizeof(buffer));
	char message[1000000];
	bzero(message, sizeof(message));
	int bytes_read;
	bytes_read = read(network_socket, message, sizeof(message));
	if(bytes_read == -1){
		printf("Read failed\n");
		return;
	}
	printf("Project received, storing...\n");
	get_path(buffer, project_name, "Manifest");
	int manifest_file = creat(buffer, S_IRWXG|S_IRWXO|S_IRWXU);
	//the message that will be sent over will be in this format
	//manifest version:# of files:file version:length of file path:file pathlength of hash:hashlength of file:filefile version...
	char manifest_version[strchr(message, ':')-message+1];
	get_token(message, manifest_version, ':');
	write(manifest_file, manifest_version, strlen(manifest_version));
	if(strlen(message) == 1){
		printf("Finished storing project\n");
		return;
	}
	char file_list_length_string[strchr(message, ':')-message+1];
	get_token(message, file_list_length_string, ':');
	int file_list_length = atoi(file_list_length_string);
	int i, j;
	for(i = 1; i <= file_list_length; i++){
		char file_version[strchr(message, ':')-message+1];
		get_token(message, file_version, ':');
		write(manifest_file, "\n", 1);
		write(manifest_file, file_version, strlen(file_version));
		char file_path_length_string[strchr(message, ':')-message+1];
		get_token(message, file_path_length_string, ':');
		int file_path_length = atoi(file_path_length_string);
		char file_path_and_hash_length[strchr(message, ':')-message+1];
		get_token(message, file_path_and_hash_length, ':');
		char file_path[file_path_length+1];
		strncpy(file_path, file_path_and_hash_length, file_path_length);
		file_path[file_path_length] = '\0';
		write(manifest_file, " ", 1);
		write(manifest_file, file_path, strlen(file_path));
		char hash_length_string[strlen(file_path_and_hash_length)-file_path_length+1];
		for(j = file_path_length; j < strlen(file_path_and_hash_length); j++)
			hash_length_string[j-file_path_length] = file_path_and_hash_length[j];
		int hash_length = atoi(hash_length_string);
		char hash_and_file_length[strchr(message, ':')-message+1];
		get_token(message, hash_and_file_length, ':');
		char hash[hash_length+1];
		strncpy(hash, hash_and_file_length, hash_length);
		hash[hash_length] = '\0';
		write(manifest_file, " ", 1);
		write(manifest_file, hash, strlen(hash));
		char file_length_string[strlen(hash_and_file_length)-hash_length+1];
		for(j = hash_length; j < strlen(hash_and_file_length); j++)
			file_length_string[j-hash_length] = hash_and_file_length[j];
		int file_length = atoi(file_length_string);
		if(i == file_list_length){
			write(manifest_file, "\n", 1);
			int new_file = creat(file_path, S_IRWXG|S_IRWXO|S_IRWXU);
			write(new_file, message, strlen(message));
			close(new_file);
		}
		else{
			char file_and_file_version[strchr(message, ':')-message+1];
			for(j = 0; message[j] != ':'; j++)
				file_and_file_version[j] = message[j];
			file_and_file_version[strchr(message, ':')-message] = '\0';
			char file_full[file_length+1];
			strncpy(file_full, file_and_file_version, file_length);
			file_full[file_length] = '\0';
			int new_file = creat(file_path, S_IRWXG|S_IRWXO|S_IRWXU);
			write(new_file, file_full, file_length);
			close(new_file);
			char tmp[strlen(message)+1];
			bzero(tmp, sizeof(tmp));
			for(j = file_length; j < strlen(message); j++)
				tmp[j-file_length] = message[j];
			bzero(message, strlen(message));
			strcpy(message, tmp);
		}	
	}
	close(manifest_file);
	printf("Finished storing project\n");
}

void update(int network_socket, char* project_name){
	char manifest_path[strlen(project_name)*2+11];
	get_path(manifest_path, project_name, "Manifest");
	int manifest_file, bytes_read, i, has_conflict = 0, has_update = 0;
	if((manifest_file = open(manifest_path, O_RDONLY)) == -1){
		printf("Client manifest not found\n");
		return;
	}
	file_node* client_head = parse_manifest(manifest_file);
	close(manifest_file);
	char buffer[strlen(project_name)+8];
	strcpy(buffer, "update:");
	strcat(buffer, project_name);
	write(network_socket, buffer, sizeof(buffer));
	char* message = malloc(10000);
	bzero(message, sizeof(message));
	i = 0;
	char buffer2;
	while((bytes_read = read(network_socket, &buffer2, sizeof(buffer2))) > 0){
		message[i] = buffer2;
		i++;
	}
	if(bytes_read == -1){
		printf("Read failed\n");
		free_file_node(client_head);
		return;
	}
	char* update_file_path = malloc(strlen(project_name)*2+9);
	get_path(update_file_path, project_name, "Update");
	int update_file = creat(update_file_path, S_IRWXG|S_IRWXO|S_IRWXU);
	char* conflict_file_path = malloc(strlen(project_name)*2+11);
	get_path(conflict_file_path, project_name, "Conflict");
	//the message that will be sent over will be in this format
	//manifest version:# of files:file version:length of file path:file pathlength of hash:hashfile version...
	char* manifest_version_string = malloc(strchr(message, ':')-message+1);
	get_token(message, manifest_version_string, ':');
	int manifest_version = atoi(manifest_version_string);
	if(get_manifest_version(manifest_path) == manifest_version){
		printf("Up to date\n");
		remove(conflict_file_path);
		close(update_file);
		free_file_node(client_head);
		return;
	}
	printf("Starting project comparison...\n\n");
	if(client_head -> version == -1){
		printf("Finished comparison, no differences found besides manifest version\n");
		remove(conflict_file_path);
		close(update_file);
		free_file_node(client_head);
		return;
	}
	//int file_list_length = get_file_list_length(head);
	int conflict_file = creat(conflict_file_path, S_IRWXG|S_IRWXO|S_IRWXU);
	file_node* server_head = parse_message(message);
	/*file_node* tmp4 = client_head;
	while(tmp4!= NULL){
		printf("version: %d\npath: %s\nhash: %s\n", tmp4->version, tmp4->path, tmp4->hash);
		tmp4=tmp4->next;
	}
	printf("\n");
	file_node* tmp3 = server_head;
	while(tmp3!= NULL){
		printf("version: %d\npath: %s\nhash: %s\n", tmp3->version, tmp3->path, tmp3->hash);
		tmp3=tmp3->next;
	}*/
	file_node* client_tmp = client_head;
	file_node* server_tmp = server_head;
	while(client_tmp != NULL){
		if(strcmp(client_tmp -> path, server_tmp -> path) != 0){
			file_node* server_tmp2 = server_tmp -> next;
			int delete_or_add = 0;
			while(server_tmp2 != NULL){
				if(strcmp(server_tmp2 -> path, client_tmp -> path) == 0){
					delete_or_add = 1;
					break;
				}
				server_tmp2 = server_tmp2 -> next;
			}
			if(has_conflict){
				if(delete_or_add)
					server_tmp = server_tmp -> next;
				else
					client_tmp = client_tmp -> next;
			}
			else{
				if(delete_or_add){
					update_write(update_file, 'A', server_tmp -> path, server_tmp -> hash, &has_update);
					server_tmp = server_tmp -> next;
				}
				else{
					update_write(update_file, 'D', client_tmp -> path, client_tmp -> hash, &has_update);
					client_tmp = client_tmp -> next;
				}
			}
		}
		else{
			unsigned char* buf = malloc(SHA_DIGEST_LENGTH);
			int file = open(client_tmp -> path, O_RDONLY);
			bzero(message, sizeof(message));
			read(file, message, sizeof(message));
			char* client_local_hash = malloc(SHA_DIGEST_LENGTH*2);
			SHA1(message, strlen(message), buf);
			for (i = 0; i < SHA_DIGEST_LENGTH; i++)
				sprintf((char*)&(client_local_hash[i*2]), "%02x", buf[i]);
		//	printf("client local hash: %s\n", client_local_hash);
		//	printf("client hash: %s\n", client_tmp -> hash);
		//	printf("server hash: %s\n", server_tmp -> hash);
			if(strcmp(client_tmp -> hash, server_tmp -> hash) != 0 && strcmp(client_tmp -> hash, client_local_hash) != 0){
				if(!has_conflict)
					printf("\nConflict found, removing update file\n");
				//printf("server hash: %s\nclient hash: %s\nlive hash: %s\n", server_tmp->hash, client_tmp->hash, client_local_hash);
				update_write(conflict_file, 'C', client_tmp -> path, client_local_hash, &has_conflict);	
			}
			else if(!has_conflict && client_tmp -> version != server_tmp -> version
			&& strcmp(client_tmp -> hash, server_tmp -> hash) != 0 && strcmp(client_tmp -> hash, client_local_hash) == 0)
				update_write(update_file, 'M', client_tmp -> path, client_tmp -> hash, &has_update);
			client_tmp = client_tmp -> next;
			server_tmp = server_tmp -> next;
		}
	}
	if(server_tmp != NULL && !has_conflict){
		while(server_tmp != NULL){
			update_write(update_file, 'A', server_tmp -> path, server_tmp -> hash, &has_update);
			server_tmp = server_tmp -> next;
		}
	}
	
	
	/*
	char data[] = "hello world";
	unsigned char hash[SHA_DIGEST_LENGTH];
	SHA1(data, strlen(data), hash);
	int i;
	char buf[SHA_DIGEST_LENGTH*2];
	for (i = 0; i < SHA_DIGEST_LENGTH; i++){
        sprintf((char*)&(buf[i*2]), "%02x", hash[i]);
    }
    printf("hash: %s\n", buf);
    * 
    * 
    * compile like
    * gcc WTF.c -o WTF -pthread -lssl -lcrypto
    * */
    
    free_file_node(client_head);
    free_file_node(server_head);
    close(update_file);
    close(conflict_file);
    if(has_conflict){
		remove(update_file_path);
		printf("\nFinished comparison, conflicts were found and must be resolved\nbefore project can be updated\n");
	}
    else{
		remove(conflict_file_path);
		if(has_update)
			printf("\nFinished comparison, updates were found\n");
		else
			printf("Finished comparison, no differences found besides manifest version\n");
	}
}

void update_write(int file, char c, char* path, char* hash, int* has_update_or_conflict){
	*has_update_or_conflict = 1;
	write(file, &c, 1);
	write(file, " ", 1);
	write(file, path, strlen(path));
	write(file, " ", 1);
	write(file, hash, strlen(hash));
	write(file, "\n", 1);
	printf("%c %s\n", c, path);
}

void get_path(char* path, char* project_name, char* extension){
	strcpy(path, project_name);
	strcat(path, "/");
	strcat(path, project_name);
	strcat(path, ".");
	strcat(path, extension);
}

file_node* parse_manifest(int file){
	char* buffer = malloc(10000);
	bzero(buffer, sizeof(buffer));
	int bytes_read, i = 0;
	char buffer2;
	while((bytes_read = read(file, &buffer2, sizeof(buffer2))) > 0){
		buffer[i] = buffer2;
		i++;
	}
	if(bytes_read == -1){
		printf("Read failed\n");
		return NULL;
	}
	file_node* head = (file_node*)malloc(sizeof(file_node));
	char* buffer_tmp = malloc(strlen(strchr(buffer, '\n'))+1);
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

file_node* parse_message(char* message){
	file_node* head = (file_node*)malloc(sizeof(file_node));
	file_node* tmp = head;
	char* file_list_length_string = malloc(strchr(message, ':')-message+1);
	get_token(message, file_list_length_string, ':');
	int file_list_length = atoi(file_list_length_string);
	int i, j;
	for(i = 1; i <= file_list_length; i++){
		tmp -> next = NULL;
		//the message that will be sent over will be in this format
		//manifest version:# of files:file version:length of file path:file pathlength of hash:hashfile version...
		char* file_version_string = malloc(strchr(message, ':')-message+1);
		get_token(message, file_version_string, ':');
		int file_version = atoi(file_version_string);
		tmp -> version = file_version;
		char* file_path_length_string = malloc(strchr(message, ':')-message+1);
		get_token(message, file_path_length_string, ':');
		int file_path_length = atoi(file_path_length_string);
		char* file_path_and_hash_length = malloc(strchr(message, ':')-message+1);
		get_token(message, file_path_and_hash_length, ':');
		char* file_path = malloc(file_path_length+1);
		strncpy(file_path, file_path_and_hash_length, file_path_length);
		file_path[file_path_length] = '\0';
		tmp -> path = file_path;
		if(i == file_list_length){
			char* hash = malloc(strlen(message)+1);
			strncpy(hash, message, strlen(message));
			hash[strlen(message)] = '\0';
			tmp -> hash = hash;
			return head;
		}
		char* hash_length_string = malloc(strlen(file_path_and_hash_length)-file_path_length+1);
		for(j = file_path_length; j < strlen(file_path_and_hash_length); j++)
			hash_length_string[j-file_path_length] = file_path_and_hash_length[j];
		int hash_length = atoi(hash_length_string);
		char* hash_and_file_version = malloc(strchr(message, ':')-message+1);
		for(j = 0; message[j] != ':'; j++)
			hash_and_file_version[j] = message[j];
		hash_and_file_version[strchr(message, ':')-message] = '\0';
		char* hash = malloc(hash_length+1);
		strncpy(hash, hash_and_file_version, hash_length);
		hash[hash_length] = '\0';
		tmp -> hash = hash;
		char* tmp2 = malloc(strlen(message)+1);
		bzero(tmp2, sizeof(tmp2));
		for(j = hash_length; j < strlen(message); j++)
			tmp2[j-hash_length] = message[j];
		bzero(message, strlen(message));
		strcpy(message, tmp2);
		file_node* new_file_node = (file_node*)malloc(sizeof(file_node));
		tmp -> next = new_file_node;
		tmp = tmp -> next;
	}
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
