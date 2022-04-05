#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "ler_arquivo.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#define IP "192.168.0.53"
#define PORTA 10136
#define MAX_SIZE 5000
#define true 1
#define false 0

int menu()
{
    system("clear");
    int id;
    while (1)
    {
        printf("\n\nQual andar do prédio você deseja verificar?\n\t1)Terreo\n\t2)Primeiro\n\n");
        scanf("%d", &id);
        if (id == 1 || id == 2)
        {
            return id;
        }
        else
            printf("\nID inválido\n");
    }
}

void resultados(char *file)
{
    if (strlen(file) > 0)
    {
        cJSON *json = cJSON_Parse(file);
        system("clear");
        int temperatura = cJSON_GetObjectItemCaseSensitive(json, "temperature")->valueint;
        int humidade = cJSON_GetObjectItemCaseSensitive(json, "humidity")->valueint;
        int pessoas = cJSON_GetObjectItemCaseSensitive(json, "total_people")->valueint;
        int *nome = cJSON_GetObjectItemCaseSensitive(json, "nome")->valuestring;

        cJSON *saidas = cJSON_GetObjectItemCaseSensitive(json, "output");
        cJSON *entradas = cJSON_GetObjectItemCaseSensitive(json, "input");
        cJSON *pin = NULL;
        printf("Monitorando: %s\n", nome);
        printf("Temperatura: %d⁰C\t Úmidade: %d%%\t Pessoas no Prédio: %d\n", temperatura, humidade, pessoas);
        printf("\n\nOutput:\n\n       Pino\t\tValor\t\t\tTag\n");
        cJSON_ArrayForEach(pin, saidas)
        {
            int gpio = cJSON_GetObjectItemCaseSensitive(pin, "gpio")->valueint;
            char *tag = cJSON_GetObjectItemCaseSensitive(pin, "tags")->valuestring;
            int value = cJSON_GetObjectItemCaseSensitive(pin, "value")->valueint;
            char *status = value ? "ON " : "OFF";
            printf("|\t%02d\t|\t%s\t|\t%s\n", gpio, status, tag);
        }
        printf("\n\nInput:\n\n       Pino\t\tValor\t\t\tTag\n");
        cJSON_ArrayForEach(pin, entradas)
        {
            int gpio = cJSON_GetObjectItemCaseSensitive(pin, "gpio")->valueint;
            char *tag = cJSON_GetObjectItemCaseSensitive(pin, "tags")->valuestring;
            int value = cJSON_GetObjectItemCaseSensitive(pin, "value")->valueint;
            char *status = value ? "ON " : "OFF";
            printf("|\t%02d\t|\t%s\t|\t%s\n", gpio, status, tag);
        }
    }
}


pthread_t SOCKET_GET, SOCKET_SEND, MENU_ID;
int socketCliente, file_id;
char *file, *json_string, toggle_gpio_value = false;

void recebe_dados()
{
  char *data = malloc(MAX_SIZE);
  int buffer_size = recv(socketCliente, data, 5000, 0);
  if (buffer_size)
  {
    strcpy(json_string, data);
  }
  free(data);
  sleep(1);
}

void envia_dados()
{
  int value = 0;
  int file_size = strlen(file);
  if (file_size > 0)
  {
    if (!toggle_gpio_value)
      send(socketCliente, file, file_size, 0);
    else
    {
      char *pin[5];
      printf("\nQual pino você deseja mudar o valor: ");
      scanf("%s", pin);
      send(socketCliente, pin, sizeof(char) * 5, 0);
      toggle_gpio_value = false;
    }
  }
  sleep(1);
}

void trata_sinal(int signal)
{
  printf("\nAguarde...\n");
  if (signal == SIGQUIT)
  {
    toggle_gpio_value = true;
  }
  if (signal == SIGTSTP)
  {
    file_id = file_id == 1 ? 2 : 1;
    strcpy(file, ler_arquivo(file_id));
  }
}

void central_socket(int id)
{
  signal(SIGQUIT, trata_sinal);
  signal(SIGTSTP, trata_sinal);
  int servidorSocket;
  int option = 1;
  struct sockaddr_in servidorAddr;
  struct sockaddr_in clienteAddr;
  unsigned int clienteLength;

  if ((servidorSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    perror("Socket");

  memset(&servidorAddr, 0, sizeof(servidorAddr));
  servidorAddr.sin_family = AF_INET;
  servidorAddr.sin_addr.s_addr = inet_addr(IP);
  servidorAddr.sin_port = htons(PORTA);

  setsockopt(servidorSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
  if (bind(servidorSocket, (struct sockaddr *)&servidorAddr, sizeof(servidorAddr)) < 0)
    perror("Bind");

  if (listen(servidorSocket, 10) < 0)
    perror("Listen");

  file = malloc(MAX_SIZE);
  json_string = malloc(MAX_SIZE);
  strcpy(file, ler_arquivo(id));

  clienteLength = sizeof(clienteAddr);
  if ((socketCliente = accept(servidorSocket,
                              (struct sockaddr *)&clienteAddr,
                              &clienteLength)) < 0)
    perror("Accept");
  while (1)
  {
    pthread_create(&MENU_ID, NULL, resultados, json_string);
    pthread_join(MENU_ID, NULL);
    pthread_create(&SOCKET_GET, NULL, recebe_dados, NULL);
    pthread_create(&SOCKET_SEND, NULL, envia_dados, NULL);
    pthread_join(SOCKET_GET, NULL);
    pthread_join(SOCKET_SEND, NULL);
    sleep(1);
  }
  close(servidorSocket);
}



int main()
{
    int id = menu();
    central_socket(1);
    return 0;
}
