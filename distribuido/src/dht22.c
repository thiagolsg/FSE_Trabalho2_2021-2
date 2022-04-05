#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_TIMINGS 100
#define MAX_STACK 5
#define DHT_PIN 29 

void read_dht_data(int *temperature, int *humidity, int count)
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    int data[5] = {0, 0, 0, 0, 0};
    data[0] = data[1] = data[2] = data[3] = data[4] = 0;
    pinMode(DHT_PIN, OUTPUT);
    digitalWrite(DHT_PIN, LOW);
    delay(18);
    pinMode(DHT_PIN, INPUT);
    for (i = 0; i < MAX_TIMINGS; i++)
    {
        counter = 0;
        while (digitalRead(DHT_PIN) == laststate)
        {
            counter++;
            delayMicroseconds(1);
            if (counter == 255)
            {
                break;
            }
        }
        laststate = digitalRead(DHT_PIN);
        if (counter == 255)
            break;
        if ((i >= 4) && (i % 2 == 0))
        {
            data[j / 8] <<= 1;
            if (counter > 16)
                data[j / 8] |= 1;
            j++;
        }
    }

    if ((j >= 39) &&
        (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)))
    {
        float h = (float)((data[0] << 8) + data[1]) / 10;
        if (h > 100)
        {
            h = data[0]; 
        }
        float c = (float)(((data[2] & 0x7F) << 8) + data[3]) / 10;
        if (c > 125)
        {
            c = data[2]; 
        }
        if (data[2] & 0x80)
        {
            c = -c;
        }
        float f = c * 1.8f + 32;
        *humidity = h;
        *temperature = c;
    }
    else
    {

        printf("Erro ao ler temperatura e humidade. Realizando nova leitura\n");
        delay(100);
        if (count != MAX_STACK)
            read_dht_data(temperature, humidity, count + 1);
        else
        {
            *humidity *= 1;
            *temperature *= 1;
            printf("Não é possível ler o sensor DHT22\nUtlizando valor da leitura anterior\n");
        }
    }
}