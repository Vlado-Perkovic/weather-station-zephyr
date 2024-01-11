#ifndef PHOTORESISTOR_H
#define PHOTORESISTOR_H

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>

void zws_photoresistor_setup();
double zws_photoresistor_fetch();
#endif // !PHOTORESISTOR_H
