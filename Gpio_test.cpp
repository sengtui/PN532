#include <stdio.h>
#include <mraa.hpp>
#include "gpio_7688.hpp"


mraa::Gpio* Buzzer, *Relay, *Led1, *DI0;
mraa::Pwm *PWM0,*PWM2, *PWM3;

main()
{
  int i=0;
  Buzzer = new mraa::Gpio(GPIO_BUZZER);
  Led1 = new mraa::Gpio(GPIO_LED1);
  Relay = new mraa::Gpio(GPIO_RELAY);
  PWM0 = new mraa::Pwm(GPIO_PWM0);
  PWM2 = new mraa::Pwm(GPIO_PWM2);
  DI0 = new mraa::Gpio(GPIO_DI0);

  Buzzer->dir(mraa::DIR_OUT_LOW);
  Led1->dir(mraa::DIR_OUT_LOW);
  Relay->dir(mraa::DIR_OUT_LOW);
  DI0->dir(mraa::DIR_IN);

  PWM2->pulsewidth_ms(1000);
  PWM2->write(0.50);
  PWM2->enable(true);

  PWM0->pulsewidth_ms(1000);
  PWM0->write(0.50);
  PWM0->enable(true);

  if(!Buzzer || !Led1 ||!Relay){
    fprintf(stderr, "GPIO issue error, check ownership please\n");
    return EXIT_FAILURE;
  } 

//Buzzer->write(1);
  Led1->write(1);
  usleep(500000);

  do{
    Led1->write(DI0->read());
    usleep(10);

  } while(++i<1000000);
  Led1->write(1);
  usleep(1000000);
  Led1->write(0);
  Buzzer->write(0);
  Led1->write(0);
  PWM0->write(0.0);
  PWM2->write(0.0);
  PWM0->enable(false);
  PWM2->enable(false);

  delete Buzzer, Led1, Relay, PWM0, PWM2, DI0;
}