#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *ler_arquivo(int file_id)
{
  char *fp = file_id == 2 ? "./config/configuracao_andar_1.json" : "./config/configuracao_andar_terreo.json";
  long length;
  FILE *file = fopen(fp, "r");
  if (file)
  {
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(length + 1);
    if (buffer)
    {
      fread(buffer, 1, length, file);
    }
    fclose(file);
    return buffer;
  }
  else
  {
    exit(0);
  }
}