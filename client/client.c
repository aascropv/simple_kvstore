#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>

#include "sock.h"

int main(int argc, char **argv)
{
    int opt;
    char *server_host_name = NULL, *server_port = NULL;

    /* Parsing args */
    while ((opt = getopt(argc, argv, "h:p:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            server_host_name = malloc(strlen(optarg) + 1);
            strncpy(server_host_name, optarg, strlen(optarg));
            break;
        case 'p':
            server_port = malloc(strlen(optarg) + 1);
            strncpy(server_port, optarg, strlen(optarg));
            break;
        case '?':
            fprintf(stderr, "Unknown option \"-%c\"\n", isprint(optopt) ?
                    optopt : '#');
            return 0;
        }
    }

    if (!server_host_name)
    {
        fprintf(stderr, "Error!, No host name provided!\n");
        exit(1);
    }

    if (!server_port)
    {
        fprintf(stderr, "Error!, No port number provided!\n");
        exit(1);
    }

    /* Open a client socket fd */
    int clientfd __attribute__((unused)) = open_clientfd(server_host_name, server_port);

    /* Start your coding client code here! */
    char sendBuffer[1024];
    char recvBuffer[1024];

    printf("[INFO] Connected to %s: %s\n", server_host_name, server_port);
    printf("[INFO] Welcome! Please type \"HELP\" for available commands\n");
    while (fgets(sendBuffer, 1024, stdin))
    {
        char *src, *dst;
        for (src = dst = sendBuffer; *src != '\0'; src++)
        {
            *dst = *src;
            if (*dst != '\n')
                dst++;
        }
        *dst = '\0';
        if (strcmp(sendBuffer, "EXIT") == 0)
        {
            if (send(clientfd, sendBuffer, 1024, 0) < 0)
                printf("Send failed!\n");
            printf("Client exits.\n");
            break;
        }
        else if (strcmp(sendBuffer, "HELP") == 0)
        {
            printf("Commands\t\tDescription\n");
            printf("SET [Key] [Value]\tStore the key value pair into the database.\n");
            printf("GET [Key]\t\tGet the value of [Key] from the database.\n");
            printf("DELETE [Key]\t\tDelete [Key] and its value from the database.\n");
            printf("EXIT\t\t\tExit\n");
        }
        else
        {
            if (send(clientfd, sendBuffer, 1024, 0) < 0)
                printf("Send failed!\n");
            if (recv(clientfd, recvBuffer, 1024, 0) < 0)
                printf("Receive failed!\n");
            printf("%s\n", recvBuffer);
        }
        bzero(sendBuffer, 1024);
        bzero(recvBuffer, 1024);
    }
    close(clientfd);
    return 0;
}

