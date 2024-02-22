#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <fcntl.h>

#include "types.h"
#include "sock.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct hashNode node_t;
typedef struct hashTable_struct hashNode_t;

struct hashNode
{
    char *key;
    char *value;
    node_t *next;
};

struct hashTable_struct
{
    node_t *next;
};

hashNode_t *hashTable[62];


unsigned int hashTable_hash_str(char *key)
{
    int index = key[0] - '0';
    if (index > 16 && index < 43)
        index = index - 7;
    else if (index > 48 && index < 75)
        index = index - 13;
    return index;
}

void hashTable_insert(char *key, char *value)
{
    node_t *head;
    unsigned int pos = hashTable_hash_str(key);
    if (hashTable[pos]->next)
    {
        head = hashTable[pos]->next;
        while(head->next)
            head = head->next;

        node_t *newNode = (node_t *)malloc(sizeof(node_t));
        newNode->key = (char *)malloc(100 * sizeof(char));
        newNode->value = (char *)malloc(100 * sizeof(char));
        strcpy(newNode->key, key);
        strcpy(newNode->value, value);
        newNode->next = NULL;
        head->next = newNode;
    }
    else
    {
        node_t *newNode = (node_t *)malloc(sizeof(node_t));
        newNode->key = (char *)malloc(100 * sizeof(char));
        newNode->value = (char *)malloc(100 * sizeof(char));
        strcpy(newNode->key, key);
        strcpy(newNode->value, value);
        newNode->next = NULL;

        hashTable[pos]->next = newNode;
    }
}

void hashTable_remove(char *key)
{
    unsigned int pos = hashTable_hash_str(key);

    if (hashTable[pos]->next)
    {
        node_t *cur = hashTable[pos]->next;
        if (strcmp(cur->key, key) == 0)
        {
            hashTable[pos]->next = cur->next;
            free(cur->key);
            free(cur->value);
            free(cur);
            return;
        }
        node_t *prev = cur;
        cur = cur->next;
        while(cur)
        {
            if (strcmp(cur->key, key) == 0)
            {
                prev->next = cur->next;
                free(cur->key);
                free(cur->next);
                free(cur);
                return;
            }
            else
            {
                cur = cur->next;
                prev = prev->next;
            }
        }
    }
}

node_t *hashTable_lookup(char *key)
{
    unsigned int pos = hashTable_hash_str(key);
    if (hashTable[pos]->next)
    {
        node_t *head = hashTable[pos]->next;
        while (head)
        {
            if (strcmp(key, head->key) == 0)
                return head;
            head = head->next;
        }
    }
    return NULL;
}

void hashTable_release()
{
    int i = 0;
    for (i = 0; i < 62; i++)
    {
        if (hashTable[i]->next)
        {
            node_t *head = hashTable[i]->next;
            hashTable[i]->next = NULL;
            while (head)
            {
                node_t *temp = head;
                head = head->next;
                if (temp)
                {
                    free(temp->key);
                    free(temp->value);
                    free(temp);
                }
            }
        }
    }
}



void *server_thread(void *argv)
{
    int connfd = *((int*)argv);
    pthread_detach(pthread_self());
    printf("[CLIENT CONNECT] Connected to client.\n");
    printf("[THREAD INFO] Thread %ld is created.\n", pthread_self());
    while (1)
    {
        pthread_mutex_lock(&lock);
        char recvBuffer[1024] = {};
        char sendBuffer[1024] = {};
        char *saveptr = NULL;
        char *option = NULL;
        char *key = NULL;
        char *value = NULL;
        char *arg_check = NULL;
        recv(connfd, recvBuffer, 1024, 0);
        option = strtok_r(recvBuffer, " ", &saveptr);
        if (strcmp(option, "SET") == 0)
        {
            key = strtok_r(NULL, " ", &saveptr);
            if (key == NULL)
            {
                strcpy(sendBuffer, "[ERROR] Invalid inputs.");
                send(connfd, sendBuffer, 1024, 0);
                pthread_mutex_unlock(&lock);
                continue;
            }
            node_t *node = hashTable_lookup(key);
            if (node)
            {
                strcpy(sendBuffer, "[ERROR] Key already exits.");
                send(connfd, sendBuffer, 1024, 0);
                pthread_mutex_unlock(&lock);
                continue;
            }
            else
            {
                value = strtok_r(NULL, " ", &saveptr);
                if (value == NULL)
                {
                    strcpy(sendBuffer, "[ERROR] Invalid inputs.");
                    send(connfd, sendBuffer, 1024, 0);
                    pthread_mutex_unlock(&lock);
                    continue;
                }
                arg_check = strtok_r(NULL, " ", &saveptr);
                if (arg_check)
                {
                    strcpy(sendBuffer, "[ERROR] Invalid inputs.");
                    send(connfd, sendBuffer, 1024, 0);
                    pthread_mutex_unlock(&lock);
                    continue;
                }
                else
                {
                    hashTable_insert(key, value);

                    write(connfd, "[OK] Key value pair (", 21);
                    write(connfd, key, strlen(key));
                    write(connfd, ", ", 2);
                    write(connfd, value, strlen(value));
                    write(connfd, ") is strored!", 14);

                    pthread_mutex_unlock(&lock);
                    continue;
                }
            }
        }
        else if (strcmp(option, "GET") == 0)
        {
            key = strtok_r(NULL, " ", &saveptr);
            if (key == NULL)
            {
                strcpy(sendBuffer, "[ERROR] Invalid inputs.");
                send(connfd, sendBuffer, 1024, 0);
                pthread_mutex_unlock(&lock);
                continue;
            }
            arg_check = strtok_r(NULL, " ", &saveptr);
            if (arg_check)
            {
                strcpy(sendBuffer, "[ERROR] Invalid inputs.");
                send(connfd, sendBuffer, 1024, 0);
                pthread_mutex_unlock(&lock);
                continue;
            }
            else
            {
                node_t *node = hashTable_lookup(key);
                if (node)
                {
                    value = node->value;
                    printf("get key:%s, value:%s\n", key, value);
                    write(connfd, "[OK] The value of ", 18);
                    write(connfd, key, strlen(key));
                    write(connfd, " is ", 4);
                    write(connfd, value, strlen(value));

                    pthread_mutex_unlock(&lock);
                    continue;
                }
                else
                {
                    strcpy(sendBuffer, "[ERROR] Key does not exist!");
                    send(connfd, sendBuffer, 1024, 0);
                    pthread_mutex_unlock(&lock);
                    continue;
                }
            }
        }
        else if (strcmp(option, "DELETE") == 0)
        {
            key = strtok_r(NULL, " ", &saveptr);
            if (key == NULL)
            {
                strcpy(sendBuffer, "[ERROR] Invalid inputs.");
                send(connfd, sendBuffer, 1024, 0);
                pthread_mutex_unlock(&lock);
                continue;
            }
            arg_check = strtok_r(NULL, " ", &saveptr);
            if (arg_check)
            {
                strcpy(sendBuffer, "[ERROR] Invalid inputs.");
                send(connfd, sendBuffer, 1024, 0);
                pthread_mutex_unlock(&lock);
                continue;
            }
            else
            {
                node_t *node = hashTable_lookup(key);
                if (node)
                {
                    value = node->value;
                    hashTable_remove(key);
                    printf("delete key: %s\n", key);
                    write(connfd, "[OK] Key \"", 10);
                    write(connfd, key, strlen(key));
                    write(connfd, "\" is removed!", 14);

                    pthread_mutex_unlock(&lock);
                    continue;
                }
                else
                {
                    strcpy(sendBuffer, "[ERROR] Key does not exist!");
                    send(connfd, sendBuffer, 1024, 0);
                    pthread_mutex_unlock(&lock);
                    continue;
                }
            }
        }

        else
        {
            if (strcmp(option, "EXIT") == 0)
            {
                printf("Exit.\n");
                printf("[THREAD INFO] Thread %ld exits.\n", pthread_self());
                pthread_mutex_unlock(&lock);
                close(connfd);
                pthread_exit(NULL);
                continue;
            }

            strcpy(sendBuffer, "[ERROR] Invalid inputs.");
            send(connfd, sendBuffer, 1024, 0);
            pthread_mutex_unlock(&lock);
        }
    }
    return NULL;
}


int main(int argc, char **argv)
{
    char *server_port = 0;
    int opt = 0;
    int *connfd;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    /* Parsing args */
    while ((opt = getopt(argc, argv, "p:")) != -1)
    {
        switch (opt)
        {
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

    if (!server_port)
    {
        fprintf(stderr, "Error! No port number provided!\n");
        exit(1);
    }

    /* Open a listen socket fd */
    int listenfd __attribute__((unused)) = open_listenfd(server_port);

    /* Start coding your server code here! */

    printf("[INFO] Start with a clean database...\n");
    printf("[INFO] Initializing the server...\n");
    printf("[INFO] Server initialized!\n");
    printf("[INFO] Listening to the port %s...\n", server_port);
    for (int i = 0; i < 62; i++)
    {
        hashTable[i] = (hashNode_t *)malloc(sizeof(hashNode_t));
        memset(hashTable[i], 0, sizeof(hashNode_t));
    }

    pthread_t tid = 0;
    while (1)
    {
        addr_size = sizeof(serverStorage);
        connfd = malloc(sizeof(int));
        *connfd = accept(listenfd, (struct sockaddr*)&serverStorage, &addr_size);
        pthread_create(&tid, NULL, server_thread, connfd);

    }
    return 0;
}

