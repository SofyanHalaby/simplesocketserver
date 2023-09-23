/*
 * Copyright (C) 2023 Sofyan Gaber <sofyanhalaby@gmail.com>
 *
 * This code is for demonstration and educational purposes. It 
 * implements a simple server socket.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

//regular c libraries
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
//socket libraries
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//thread library
#include <pthread.h>
//error detection library
#include <errno.h>


//configurations
#define CONFIG_MAX_CONNECTIONS      10
#define CONFIG_IP                   "127.0.0.1"
#define CONFIG_PORT                 8080
#define BUFFER_SIZE                 16


//messages
#define MESSAGE_HELLO               "hello"
#define RESPONSE_HELLO              "hello"
#define MESSAGE_NEGOTIATE           "negotiate"
#define RESPONSE_NEGOTIATE          "negotiate back"
#define MESSAGE_BYE                 "bye"
#define RESPONSE_BYE                "bye"
#define RESPONSE_INVALID_START      "invalid start"
#define RESPONSE_INVALID_MESSAGE    "message not recognised"
#define RESPONSE_RECIEVE_ERROR      "recieve error"

//when i tried to test from telnet, trailing \n\r are included
//so here i exlcode them
void trim_end(char* buffer, int* length)
{
    while(*length > 0 && isspace(buffer[*length-1]))
    {
        -- *length;
    }
    buffer[*length] = 0;
}

char* msgck(char* msg)
{
    if (strcmp(msg, MESSAGE_HELLO) == 0)
    {
        return RESPONSE_HELLO;
    }
    else if (strcmp(msg, MESSAGE_BYE) == 0)
    {
        return RESPONSE_BYE;
    }
    else if (strcmp(msg, MESSAGE_NEGOTIATE) == 0)
    {
        return RESPONSE_NEGOTIATE;
    }
    return RESPONSE_INVALID_MESSAGE;
}

// this function is responsible for handling a single client connection
void* handle_client(void* arg)
{
    int* ptr_client_socket_fd = ((int*)arg);
    int client_socket_fd = *ptr_client_socket_fd;
    free(ptr_client_socket_fd);
    
    char buffer[BUFFER_SIZE];

    //stay waiting for hello message
    bool initiated = false;
    while (!initiated)
    {
        int length = recv(client_socket_fd, buffer, BUFFER_SIZE, 0);

        if (length < 0)
        {
            send(client_socket_fd, RESPONSE_RECIEVE_ERROR, sizeof(RESPONSE_RECIEVE_ERROR), 0);
            continue;
        }
        
        trim_end(buffer, &length);
        
        if(strcmp(buffer, MESSAGE_HELLO) == 0)
        {
            send(client_socket_fd, RESPONSE_HELLO, sizeof(RESPONSE_HELLO), 0);
            initiated = true;
        }
        else
        {
            send(client_socket_fd, RESPONSE_INVALID_START, sizeof(RESPONSE_INVALID_START), 0);
        }
    }
    
    
    bool bye = false;
    while (!bye)
    {
        int length = recv(client_socket_fd, buffer, BUFFER_SIZE, 0);
        
        if (length < 0)
        {
            send(client_socket_fd, RESPONSE_RECIEVE_ERROR, sizeof(RESPONSE_RECIEVE_ERROR), 0);
        }
        else
        {
            trim_end(buffer, &length);
            char* response = msgck(buffer);
            send(client_socket_fd, response, strlen(response), 0);
            //message bye indicate for end of communication
            if (strcmp(buffer, MESSAGE_BYE) == 0)
            {
                bye = true;
            }
            
        }
    }
    //close connection at the end of the function 
    close(client_socket_fd);

    return NULL;
    
}

int srvlstn(char* ip, int port)
{
    printf("srvlstn\n");
    int socked_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socked_fd == -1)
    {
        printf("failure socket: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);//convert from platform byte_order to network byte order
    server_addr.sin_family = AF_INET;

    if(bind(socked_fd, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in)))
    {
        printf("failure bind: %s\n", strerror(errno));
        close(socked_fd);
        return EXIT_FAILURE;
    }

    if (listen(socked_fd, CONFIG_MAX_CONNECTIONS) == -1)
    {
        printf("failure listen: %s\n", strerror(errno));
        close(socked_fd);
        return EXIT_FAILURE;
    }

    while (true)
    {
        printf("waiting for new connections ...\n");
        struct sockaddr_in client_addr;
        int address_size = sizeof(struct sockaddr_in);
        int* ptr_client_socket_fd = malloc(sizeof(int));
        *ptr_client_socket_fd = accept(socked_fd, (struct sockaddr*) &client_addr, &address_size);
        if (*ptr_client_socket_fd == -1)
        {
            printf("failure accept: %s\n", strerror(errno));
            //return EXIT_FAILURE;
            continue;
        }
        pthread_t thread;

        pthread_create(&thread, NULL, handle_client, ptr_client_socket_fd); 
    }

    return EXIT_SUCCESS;
}


int main()
{
    srvlstn(CONFIG_IP, CONFIG_PORT);
    printf("exiting ...\n");
    return 0;
}