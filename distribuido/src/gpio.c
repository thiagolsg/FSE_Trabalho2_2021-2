#include <gpio.h>
#include <wiringPi.h>

void turn_on(int gpio_port) {
  digitalWrite(gpio_port, HIGH);
  return;
}

void turn_off(int gpio_port) {
  digitalWrite(gpio_port, LOW);
  return;
}

int read_gpio(int gpio_port) {
  int value = digitalRead(gpio_port);
  return value;
}