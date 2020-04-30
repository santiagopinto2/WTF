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

typedef struct read_nodes{
	char c;
	struct read_nodes* next;
} read_node;

void* handle_connection(void*);
void checkout(int, char*);
file_node* parse_manifest(int);
//void cut_at_char(char*, char);
//char* get_token2(char*, char*, char);
//char* get_token(int, char);

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
	/*int client_socket = *((int*)p_client_socket);
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
		/*while((bytes_read = read(file, message+msgsize, sizeof(message)-msgsize-1)) > 0)
			msgsize += bytes_read;
		write(client_socket, message, sizeof(message));
		close(file);
		printf("File sent\n");
	}*/
	
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
	/*file_node* tmp = head;
	while(tmp != NULL){
		printf("%d %s %s\n", tmp -> version, tmp -> path, tmp -> hash);
		tmp = tmp -> next;
	}*/
	
	close(file);
	
	
	
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
			for(i = 0; i <= sizeof(copy); i++){
				token[i] = copy[i];
			}
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
	
	
	/*
	file_node* head = (file_node*)malloc(sizeof(file_node));
	char* token;
	int count = 0;
	token = get_token(file, '\n');
	if((token = get_token(file, '\n')) == NULL)
		head = NULL;
	else{	
		head -> version = atoi(token);
		head -> path = get_token(file, '\n');
		head -> hash = get_token(file, '\n');
		head -> next = NULL;
		file_node* tmp = head;
		while((token = get_token(file, '\n')) != NULL){
			if(count%3 == 0){
				file_node* new_file_node = (file_node*)malloc(sizeof(file_node));
				new_file_node -> next = NULL;
				tmp -> next = new_file_node;
				tmp = tmp -> next;
				tmp -> version = atoi(token);
			}
			else if(count%3 == 1)
				tmp -> path = token;
			else if(count%3 == 2)
				tmp -> hash = token;
			count++;
		}
	}
	/*
	file_node* tmp2 = head;
	if(tmp2 == NULL)
		printf("head null\n");
	while(tmp2 != NULL){
		printf("%d %s %s\n", tmp2 -> version, tmp2 -> path, tmp2 -> hash);
		tmp2 = tmp2 -> next;
	}*/
}

/*
void cut_at_char(char* string, char c)
{
    //valid parameter
    if (!string) return;
	int i = 0;
    //find the char you want or the end of the string.
    while(string[i] != '\0' && string[i] != c)
		i++;

    //make that location the end of the string (if it wasn't already).
    string[i] = '\0';
}

char* get_token2(char* string, char* token, char delimeter){
	memcpy(token, string, sizeof(string)+1);
	strcpy(string, strchr(string, delimeter)+1);
	cut_at_char(token, delimeter);
	return token;
}*/
/*
char* get_token(int file, char delimeter){
	char buffer;
	int flag, start = 0, count = 0;
	read_node* head = (read_node*)malloc(sizeof(read_node));
	while((flag = read(file, &buffer, sizeof(buffer))) > 0){
		if(start == 0){
			start = 1;
			count++;
			head -> c = buffer;
			head -> next = NULL;
		}
		else if(buffer == delimeter){
			start = 0;
			char* s = malloc(count+1);
			s[count] = '\0';
			count = 0;
			read_node* tmp = head;
			while(tmp -> next != NULL){
				s[count] = tmp -> c;
				count++;
				tmp = tmp -> next;
			}
			s[count] = tmp -> c;
			//free head
			return s;
		}
		else{
			count++;
			read_node* new_read_node = (read_node*)malloc(sizeof(read_node));
			new_read_node -> c = buffer;
			new_read_node -> next = NULL;
			read_node* tmp = head;
			while(tmp -> next != NULL)
				tmp = tmp -> next;
			tmp -> next = new_read_node;
		}
	}
	//free head
	return NULL;
}*/
