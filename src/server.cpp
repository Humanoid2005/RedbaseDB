#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#include "db_structs.h"

void * handle_client(void * nsd){
    int client_sd = *((int*)nsd);
    int bytes_read;

    while(true){
        //RECEIVE REQUEST
        //HANDLE CLIENT CODE
    }

    //SEND REQUEST
}

int main(){
    int server_socket = socket(AF_INET,SOCK_STREAM,0);
    int new_socket;
    sockaddr_in server_address,client_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_socket,(struct sockaddr*)&server_address,sizeof(server_address))<0){
        perror("Binding error: ");
    }

    if(listen(server_socket,5)<0){
        perror("Listening error: ");
    }

    std::cout<<"Server listening on port "<<PORT<<"\n";

    while(true){
        socklen_t c_size = sizeof(client_address);
        if((new_socket = accept(server_socket,(struct sockaddr*)&client_address,&c_size))<0){
            perror("Accept failed");
            continue;
        }
        int * client_sd = (int*)malloc(sizeof(int));
        *client_sd = new_socket;
        char client_addr[100];
        inet_ntop(AF_INET,&client_address.sin_addr,client_addr,INET_ADDRSTRLEN);
        std::cout<<"Received connection request from : "<<client_addr<<":"<<ntohs(client_address.sin_port)<<std::endl;
        pthread_t thread_id;
        if(pthread_create(&thread_id,NULL,handle_client,(void*)client_sd) !=0){
            perror("Thread creation failed: ");
            free(client_sd);
            close(new_socket);
        }
        pthread_detach(thread_id);
    }
    close(server_socket);
}