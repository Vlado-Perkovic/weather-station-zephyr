#ifndef SHT3XD_H
#define SHT3XD_H

#include <zephyr/init.h>
#include <zephyr/kernel.h>

const struct device *const sht3xd_setup();
void sht3xd_fetch(const struct device *const device, double *temperature,
                          double *humidity);
#endif // !SHT3XD_H
