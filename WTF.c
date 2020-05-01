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

void configure(char* string_ip, char* string_port);
void checkout(int network_socket, char* project_name);
void update(int network_socket, char* project_name);
void get_path(char* path, char* project_name, char* extension);
void get_token(char* message, char* token, char delimeter);
int get_manifest_version(char* manifest_path);

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
	int manifest_file, bytes_read;
	if((manifest_file = open(manifest_path, O_RDONLY)) == -1){
		printf("Client manifest not found\n");
		return;
	}
	char buffer[strlen(project_name)+8];
	strcpy(buffer, "update:");
	strcat(buffer, project_name);
	write(network_socket, buffer, sizeof(buffer));
	char message[10000];
	bzero(message, sizeof(message));
	bytes_read = read(network_socket, message, sizeof(message));
	if(bytes_read == -1){
		printf("Read failed\n");
		return;
	}
	printf("message:\n%s\n", message);
	//the message that will be sent over will be in this format
	//manifest version:# of files:file version:length of file path:file pathlength of hash:hashfile version...
	char update_file_path[strlen(project_name)*2+9];
	get_path(update_file_path, project_name, "Update");
	int update_file = creat(update_file_path, S_IRWXG|S_IRWXO|S_IRWXU);
	char manifest_version_string[strchr(message, ':')-message+1];
	get_token(message, manifest_version_string, '\n');
	int manifest_version = atoi(manifest_version_string);
	if(get_manifest_version(manifest_path) == manifest_version){
		printf("Up to date\n");
		char conflict_file_path[strlen(project_name)*2+11];
		get_path(conflict_file_path, project_name, "Conflict");
		remove(conflict_file_path);
		return;
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
    
    close(manifest_file);
    close(update_file);
    //close conflict file
}

void get_path(char* path, char* project_name, char* extension){
	strcpy(path, project_name);
	strcat(path, "/");
	strcat(path, project_name);
	strcat(path, ".");
	strcat(path, extension);
}

void get_token(char* message, char* token, char delimeter){
	char copy[strlen(message)+1];
	strcpy(copy, message);
	char* cut = strchr(copy, delimeter);
	*cut = '\0';
	int i;
	for(i = 0; i <= strlen(copy); i++)
		token[i] = copy[i];
	strcpy(message, strchr(message, delimeter)+1);
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
