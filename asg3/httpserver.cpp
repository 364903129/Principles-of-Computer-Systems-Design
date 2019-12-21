// Cite line 2 to line 55 are from the website TA recommend on piazza
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <assert.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <err.h>
#include <queue>
#include <unordered_map> 
#include <iostream>
#include <string>
#include <utility>

#define PORT 8080
int main(int argc, char const *argv[])
{
	std::queue<std::string> cache;
	std::unordered_map<std::string, std::string> umap;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	int accesss = 1;
	char *error_200 = "HTTP/1.1 200 OK\r\n";
	char *error_201 = "HTTP/1.1 201 Created\r\n";
	char *error_400 = "HTTP/1.1 400 Bad Request\r\n";
	char *error_403 = "HTTP/1.1 403 Forbidden\r\n";
	char *error_404 = "HTTP/1.1 404 Not Found\r\n";
	char *error_500 = "HTTP/1.1 500 Internal Server Error\r\n";
	char buffer[1024];
	char buffer_send[1024];
	char data[1024];
	char command_a[5];
	char file[128];
	char contentLen[10];
	int logOn = 0;
	int isCacheNeeded = 0;
	int offset = 0;
	char logName[100];
	if(argc > 2){
		for(int i = 1; i < argc; i++){
			if(strcmp(argv[i], "-l") == 0){
				if(argv[i+1] == NULL){
					break;
				}
				logOn = 1;
				strcpy(logName, argv[i + 1]);
			}
			if(strcmp(argv[i], "-c") == 0){
				isCacheNeeded = 1;
			}
		}
	}

	// server file descriptor
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(server_fd != -1);

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	// attach to 8080
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	int res = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
	assert(res != -1);

	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	while (1)
	{
		//if can't extract connection, print out error message and exit
		int connect_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
		assert(connect_sock != -1);

		//receive message from the socket
		recv(connect_sock, buffer, 1024, 0);
		strncpy(buffer_send, buffer, sizeof(buffer));
		command_a[sizeof(buffer)] = '\0';
		//split the buffer by space
		char *word = strtok(buffer, " ");
		int count = 0;
		//extract content lenth and file name
		while (word != NULL)
		{
			//get the command "put" or "get"
			if (count == 0)
			{
				strncpy(command_a, word, sizeof(word));
				command_a[sizeof(word)] = '\0';
			}
		
			//get the content conLength
			else if (count == 6)
			{
				char *word_token = strtok(word, "\n");
				while (word_token != NULL)
				{
					strncpy(contentLen, word_token, sizeof(word_token));
					contentLen[sizeof(word_token)] = '\0';
					break;
				}
			}

			//get the filename
			else if (count == 1)
			{
				strncpy(file, word, sizeof(word));
				file[sizeof(word)] = '\0';
			}
			word = strtok(NULL, " ");
			count++;
		}

		if (strlen(file) > 27)
		{
			//bad request
			send(connect_sock, buffer, strlen(buffer), 0);
			send(connect_sock, error_400, strlen(error_400), 0);
		}
		else
		{
			for (int i = 0; i < strlen(file); i++)
			{
				if ((97 <= file[i] && file[i] <= 122) || (65 <= file[i] && file[i] <= 90) 
					|| (48 <= file[i] && file[i] <= 57) || file[i] == 95 || file[i] == 45)
				{
					accesss = 0;
				}
				else
				{
					accesss = 364803129;
					break;
				}
			}

			if (accesss == 0)
			{
				if (strcmp(command_a, "PUT") == 0)
				{
					ssize_t size = recv(connect_sock, data, 1024, 0);
					int content_lenth = atoi(contentLen);
					if (access(file, F_OK) != -1)
					{
						int fd = open(file, O_RDWR);
						while (content_lenth > 0)
						{
							write(fd, data, size);
							content_lenth -= size;
						}
						if(isCacheNeeded == 1){
							if(umap.find(file) == umap.end()){
								printf("not found in the umap");
								if(cache.size() < 4){
									cache.push(file);
									umap.insert({file,data});

								}
								if(cache.size() >= 4){
									std::string info = cache.front();
									cache.pop();
									cache.push(file);
									umap.erase(info);
									umap.insert({file,data});
								}
								if(logOn == 1){
									char container[100]; //container is a char array
									char content[100]; //content array
									int conLength = atoi(contentLen); //get the content conLength
									int logfile = open(logName, O_CREAT | O_RDWR, 00777);
									sprintf(container, "%s %s contentLength %d [was not in cache]\n", command_a, file, conLength);
									int current = offset;
									int totalLen = strlen(container) + conLength / 20 * 69 + 3 * (conLength % 20) + 18;
									offset = offset + totalLen;
									pwrite(logfile, container, strlen(container), current);
									current = current + strlen(container);
									int rCounter = 0;
									char str[100];
									char temp[2];
									int index = 0;
									str[0] = '\0';
									while(conLength > 0){
										if(conLength >= 20){
											sprintf(content, "%08d %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
												rCounter, data[index], data[index + 1], data[index + 2], data[index + 3], data[index + 4], data[index + 5], data[index + 6], data[index + 7], data[index + 8],
												data[index + 9], data[index + 10], data[index + 11], data[index + 12], data[index + 13], data[index + 14],
												data[index + 15], data[index + 16], data[index + 17], data[index + 18], data[index + 19]);
											rCounter = rCounter + 20;
											index = index + 20;
											pwrite(logfile, content, strlen(content), current);
											current = current + strlen(content);
											conLength = conLength - 20;
										}
										else{
											sprintf(temp, "%08d", rCounter);
											strcat(str, temp);
											for(int j = 0; j < conLength; j++){
												sprintf(temp, " %02x", data[j]);
												strcat(str, temp);
											}
											strcat(str, "\n");
											pwrite(logfile, str, strlen(str), current);
											str[conLength] = NULL;
											current = current + strlen(str);
											conLength = 0;
										}
									}
									pwrite(logfile, "========\n", 9, current);
									current = current + 9;
								}
								send(connect_sock, buffer_send, strlen(buffer_send), 0);
							}
							else{
								printf("found in the umap");
								std::string fileContent = umap.at(file);
								int fileContentLen = fileContent.length();
								char buffer[fileContentLen+1];
								strcpy(buffer,fileContent.c_str());
								send(connect_sock, buffer, fileContentLen, 0);
								if(logOn == 1){
									char container[100]; //container is a char array
									char content[100]; //content array
									int conLength = atoi(contentLen); //get the content conLength
									int logfile = open(logName, O_CREAT | O_RDWR, 00777);
									sprintf(container, "%s %s conLength %d\n", command_a, file, conLength);
									int current = offset;
									int totalLen = strlen(container) + conLength / 20 * 69 + 3 * (conLength % 20) + 18;
									offset = offset + totalLen;
									pwrite(logfile, container, strlen(container), current);
									current = current + strlen(container);
									int rCounter = 0;
									char str[100];
									char temp[2];
									int index = 0;
									str[0] = '\0';
									while(conLength > 0){
										if(conLength >= 20){
											sprintf(content, "%08d %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
												rCounter, data[index], data[index + 1], data[index + 2], data[index + 3], data[index + 4], data[index + 5], data[index + 6], data[index + 7], data[index + 8],
												data[index + 9], data[index + 10], data[index + 11], data[index + 12], data[index + 13], data[index + 14],
												data[index + 15], data[index + 16], data[index + 17], data[index + 18], data[index + 19]);
											rCounter = rCounter + 20;
											index = index + 20;
											pwrite(logfile, content, strlen(content), current);
											current = current + strlen(content);
											conLength = conLength - 20;
										}
										else{
											sprintf(temp, "%08d", rCounter);
											strcat(str, temp);
											for(int j = 0; j < conLength; j++){
												sprintf(temp, " %02x", data[j]);
												strcat(str, temp);
											}
											strcat(str, "\n");
											pwrite(logfile, str, strlen(str), current);
											str[conLength] = NULL;
											current = current + strlen(str);
											conLength = 0;
										}
									}
									pwrite(logfile, "========\n", 9, current);
									current = current + 9;
								}
							}
						}
						else
						{
							send(connect_sock, buffer_send, strlen(buffer_send), 0);
						}
						close(fd);
						//ok
						send(connect_sock, error_200, strlen(error_200), 0);
					}
					else
					{
						int fd = open(file, O_CREAT, 00777);
						close(fd);
						int fd_1 = open(file, O_RDWR);
						write(fd_1, data, size);
						if(cache.size()<4){
							cache.push(file);
							umap.insert({file,data});
						}
						if(cache.size() >= 4){
								std::string info = cache.front();
								cache.pop();
								cache.push(file);
								umap.erase(info);
								umap.insert({file,data});
							}
						close(fd_1);

						//========================================================================
						if (logOn == 1){
							char container[100];
							char content[100];
							int conLength = atoi(contentLen);
							int logfile = open(logName, O_CREAT | O_RDWR, 00777);
							sprintf(container, "%s %s contentLength %d [was in cache]\n", command_a, file, conLength);
							int current = offset;
							int totalLen = 0;
							totalLen = strlen(container) + conLength / 20 * 69 + 3 * (conLength % 20) + 9 + 9;
							offset = offset + totalLen;
							pwrite(logfile, container, strlen(container), current);
							current = current + strlen(container);
							int rCounter = 0;
							int index = 0;
							char str[100];
							char temp[2];
							str[0] = '\0';
							while (conLength > 0)
							{
								if (conLength >= 20)
								{
									sprintf(content, "%08d %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
										rCounter, data[index], data[index + 1], data[index + 2], data[index + 3], data[index + 4], data[index + 5], data[index + 6], data[index + 7], data[index + 8],
										data[index + 9], data[index + 10], data[index + 11], data[index + 12], data[index + 13], data[index + 14],
										data[index + 15], data[index + 16], data[index + 17], data[index + 18], data[index + 19]);
									rCounter = rCounter + 20;
									index = index + 20;
									pwrite(logfile, content, strlen(content), current);
									current = current + strlen(content);
									conLength = conLength - 20;
								}
								else
								{
									sprintf(temp, "%08d", rCounter);
									strcat(str, temp);
									for (int i = 0; i < conLength; i++)
									{
										sprintf(temp, " %02x", data[i]);
										strcat(str, temp);
									}
									strcat(str, "\n");
									pwrite(logfile, str, strlen(str), current);
									str[conLength] = NULL;
									current = current + strlen(str);
									conLength = 0;
								}
							}
							pwrite(logfile, "========\n", 9, current);
							current = current + 9;
						}
						//========================================================================

						send(connect_sock, buffer_send, strlen(buffer_send), 0);
						//Created
						send(connect_sock, error_201, strlen(error_201), 0);
					}
				}
				else if (strcmp(command_a, "GET") == 0)
				{
					int fd = open(file, O_RDONLY);
					if (fd == -1)
					{
						//not found
						if (errno == ENOENT)
						{
							send(connect_sock, buffer_send, strlen(buffer_send), 0);
							send(connect_sock, error_404, strlen(error_404), 0);

							//========================================================================
							if(logOn == 1){
								char container[100];
								int logfile = open(logName, O_CREAT | O_RDWR, 00777);
								sprintf(container, "FAIL: %s %s HTTP/1.1 --- response 404\n", command_a, file);
								int current = offset;
								int totalLen = strlen(container);
								offset = offset + totalLen + 9;
								pwrite(logfile, container, strlen(container), current);
								current = current + strlen(container);
								pwrite(logfile, "========\n", 9, current);
								current = current + 9;
							}
							//========================================================================
						}

						//forbidden
						else if (errno == EACCES)
						{
							send(connect_sock, buffer_send, strlen(buffer_send), 0);
							send(connect_sock, error_403, strlen(error_403), 0);

							//========================================================================
							if(logOn == 1){
								char container[100];
								int logfile = open(logName, O_CREAT | O_RDWR, 00777);
								sprintf(container, "FAIL: %s %s HTTP/1.1 --- response 403\n", command_a, file);
								int current = offset;
								int totalLen = strlen(container);
								offset = offset + totalLen + 9;
								pwrite(logfile, container, strlen(container), current);
								current = current + strlen(container);
								pwrite(logfile, "========\n", 9, current);
								current = current + 9;
							}
							//========================================================================

						}

					};
					send(connect_sock, buffer_send, strlen(buffer_send), 0);
					if(isCacheNeeded == 1){
						if(umap.find(file) == umap.end()){
							printf("not found in the umap");
							while (read(fd,data,sizeof(data)) > 0)
							{
								send(connect_sock, data, strlen(data),0);
							}
							if(cache.size()<4){
								cache.push(file);
								umap.insert({file,data});
							}
							if(cache.size()>=4){
								std::string info = cache.front();
								cache.pop();
								cache.push(file);
								umap.erase(info);
								umap.insert({file,data});
							}
							close(fd);
							if (logOn == 1)
							{
								char container[100];
								int logfile = open(logName, O_CREAT | O_RDWR, 00777);
								sprintf(container, "%s %s contentLength %d [was not in cache]\n", command_a, file, 0);
								int current = offset;
								int totalLen = strlen(container);
								offset = offset + totalLen + 9;
								pwrite(logfile, container, strlen(container), current);
								current = current + strlen(container);
								pwrite(logfile, "========\n", 9, current);
								current = current + 9;
							}
						}
						else{
							printf("found in the umap");
							std::string fileContent = umap.at(file);
							int fileContentLen = fileContent.length();
							char buffer[fileContentLen+1];
							strcpy(buffer, fileContent.c_str());
							send(connect_sock,buffer, fileContentLen, 0);
							close(fd);
							if (logOn == 1)
							{
								char container[100];
								int logfile = open(logName, O_CREAT | O_RDWR, 00777);
								sprintf(container, "%s %s conLength %d [was in cache]\n", command_a, file, 0);
								int current = offset;
								int totalLen = strlen(container);
								offset = offset + totalLen + 9;
								pwrite(logfile, container, strlen(container), current);
								current = current + strlen(container);
								pwrite(logfile, "========\n", 9, current);
								current = current + 9;
							}
						}
					}
					else{
						while (read(fd, data, sizeof(data)) > 0)
						{
							send(connect_sock, data, strlen(data), 0);
						}
						close(fd);

					//========================================================================
						if (logOn == 1)
						{
							char container[100];
							int logfile = open(logName, O_CREAT | O_RDWR, 00777);
							sprintf(container, "%s %s conLength %d\n", command_a, file, 0);
							int current = offset;
							int totalLen = strlen(container);
							offset = offset + totalLen + 9;
							pwrite(logfile, container, strlen(container), current);
							current = current + strlen(container);
							pwrite(logfile, "========\n", 9, current);
							current = current + 9;
						}
					}
					//========================================================================

					send(connect_sock, "\r\n", strlen("\r\n"), 0);
					//ok
					send(connect_sock, error_200, strlen(error_200), 0);
				}
				//internal server error
				else
				{
					send(connect_sock, buffer, strlen(buffer), 0);
					send(connect_sock, error_500, strlen(error_500), 0);

					//========================================================================
					if (logOn == 1)
					{
						char container[100];
						int logfile = open(logName, O_CREAT | O_RDWR, 00777);
						sprintf(container, "FAIL: %s %s HTTP/1.1 --- response 500\n", command_a, file);
						int current = offset;
						int totalLen = strlen(container);
						offset = offset + totalLen + 9;
						pwrite(logfile, container, strlen(container), current);
						current = current + strlen(container);
						pwrite(logfile, "========\n", 9, current);
						current = current + 9;
					}
					//========================================================================

				}
			}
			//bad request
			else
			{
				send(connect_sock, buffer_send, strlen(buffer_send), 0);
				send(connect_sock, error_400, strlen(error_400), 0);
				//========================================================================
				if (logOn == 1)
				{
					char container[100];
					int logfile = open(logName, O_CREAT | O_RDWR, 00777);
					sprintf(container, "FAIL: %s %s HTTP/1.1 --- response 400\n", command_a, file);
					int current = offset;
					int totalLen = strlen(container);
					offset = offset + totalLen + 9;
					pwrite(logfile, container, strlen(container), current);
					current = current + strlen(container);
					pwrite(logfile, "========\n", 9, current);
					current = current + 9;
				}
				//========================================================================
			}
		}
		close(connect_sock);
	}
}