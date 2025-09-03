#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "db_structs.h"

User current_user;

int main(int argc, char *argv[]) {
    int client_socket = socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if(connect(client_socket,(struct sockaddr*)&server_address,sizeof(server_address))<0){
        perror("Server connection error: ");
    }

    //HANDLE CLIENT ACTIONS :- send(client_sd,ptr,len,0)
    close(client_socket);
}