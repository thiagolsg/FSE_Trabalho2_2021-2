
#include <wiringPi.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <pthread.h>

#include "cJSON.h"
#include "gpio.h"
#include "dht22.h"


#define IP "192.168.0.53"
#define PORTA 10136
#define MAX_SIZE 5000
pthread_t SOCKET_GET, SOCKET_SEND;

char *message, *data;
int clientSocket;
int temperatura = 0, humidade = 0, pessoas = 0, toggle_value = 0;

typedef struct Server
{
    int porta;
    char *ip, *nome;
    cJSON *outputs, *inputs;
} Server;

void json_para_string(char *json_string, Server *server)
{
    cJSON *json = cJSON_Parse(json_string);
    cJSON *porta, *ip, *outputs, *output, *inputs, *input, *nome;
    server->nome = malloc(sizeof(char)*20);
    strcpy(server->nome, cJSON_GetObjectItemCaseSensitive(json, "nome")->valuestring);
    server->outputs = cJSON_GetObjectItemCaseSensitive(json, "outputs");
    server->inputs = cJSON_GetObjectItemCaseSensitive(json, "inputs");
}

void json_final(char **json_string, Server *server, int temperatura, int humidade, int *pessoas)
{
    cJSON *gpio_object = cJSON_CreateObject();
    int count = 2;
    for (int i = 0; i < count; i++)
    {
        char *gpio_type = i ? "output" : "input";
        cJSON *gpio = i ? server->outputs : server->inputs;

        cJSON *gpio_array = cJSON_CreateArray();
        int len = cJSON_GetArraySize(gpio);
        for (int j = 0; j < len; j++)
        {
            cJSON *item = cJSON_GetArrayItem(gpio, j);
            char *type = cJSON_GetObjectItemCaseSensitive(item, "type")->valuestring;
            if (strcmp(type, "contagem") != 0)
            {
                char *tag = cJSON_GetObjectItemCaseSensitive(item, "tag")->valuestring;
                int gpio = cJSON_GetObjectItemCaseSensitive(item, "gpio")->valueint;
                int value = read_gpio(gpio);
                cJSON *array_item = cJSON_CreateObject();
                cJSON_AddNumberToObject(array_item, "gpio", gpio);
                cJSON_AddStringToObject(array_item, "type", type);
                cJSON_AddStringToObject(array_item, "tags", tag);
                cJSON_AddNumberToObject(array_item, "value", value);
                cJSON_AddItemToArray(gpio_array, array_item);
            }
            else if(strcmp(type, "contagem") == 0){
                int gpio = cJSON_GetObjectItemCaseSensitive(item, "gpio")->valueint;
                if(gpio == 23){
                    *pessoas += 1;
                }
                else {
                    *pessoas -= 1;
                }
            }
        }
        cJSON_AddItemToObject(gpio_object, gpio_type, gpio_array);
    }
    cJSON_AddStringToObject(gpio_object,"nome", server->nome);
    cJSON_AddNumberToObject(gpio_object, "temperature", temperatura);
    cJSON_AddNumberToObject(gpio_object, "humidity", humidade);
    cJSON_AddNumberToObject(gpio_object, "total_people", *pessoas);
    *json_string = malloc(5000);
    strcpy(*json_string, cJSON_Print(gpio_object));
}

void recebe_requisicao()
{
    int i;
    char num[5];
    recv(clientSocket, data, MAX_SIZE, 0);
    for (i = 0; data[i] != '\0'; i++)
        if (i > 3)
            break;
    if (i < 3)
    {
        printf("Recebi porta: %d", atoi(num));
        toggle_value = 1;
        strcpy(num, data);
        int value = read_gpio(atoi(num));
        if (value == 0)
        {
            printf("Ligando %d\n", atoi(num));
            turn_on(atoi(num));
        }
        else
        {
            printf("Desligando%d\n", atoi(num));
            turn_off(atoi(num));
        }
    }
    else
    {
        toggle_value = 0;
        strcpy(message, data);
    }
    sleep(1);
}

void envia_dados()
{
    if (strlen(message) > 0 && !toggle_value)
    {
        char *final = malloc(MAX_SIZE);
        Server *server_config = malloc(sizeof(Server));
        json_para_string(message, server_config);
        read_dht_data(&temperatura, &humidade, 0);
        json_final(&final, server_config, temperatura, humidade, &pessoas);
        send(clientSocket, final, MAX_SIZE, 0);
        free(final);
        free(server_config);
    }
    sleep(1);
}

void main_socket()
{
    struct sockaddr_in serverAddr;
    unsigned short serverPort;
    char *IP_Server;
    message = calloc(MAX_SIZE, sizeof(char));
    data = calloc(MAX_SIZE, sizeof(char));
    IP_Server = inet_addr(IP);
    serverPort = PORTA;

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("Socket");
        exit(0);
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = IP_Server;
    serverAddr.sin_port = htons(serverPort);

    while (connect(clientSocket, (struct sockaddr *)&serverAddr,
                sizeof(serverAddr)) < 0);

    while (1)
    {
        pthread_create(&SOCKET_GET, NULL, recebe_requisicao, NULL);
        pthread_join(SOCKET_GET, NULL);
        pthread_create(&SOCKET_SEND, NULL, envia_dados, NULL);
        pthread_join(SOCKET_SEND, NULL);
    }
}




int main(int argc, char const *argv[])
{
    wiringPiSetup();
    main_socket();
}
