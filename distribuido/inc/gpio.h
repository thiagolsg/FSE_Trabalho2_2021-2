#ifndef GPIO_H_
#define GPIO_H_
#include "cJSON.h"

void turn_on(int gpio_port);
void turn_off(int gpio_port);
int read_gpio(int gpio_port);

#endif /* GPIO_H_ */