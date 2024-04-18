#include "sht3xd.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <stdint.h>
#include <stdlib.h>

const struct device *const sht3xd_setup() {
  const struct device *const dev = DEVICE_DT_GET_ONE(sensirion_sht3xd);
  if (!device_is_ready(dev)) {
    printf("Device %s is not ready\n", dev->name);
    return NULL;
  }
  return dev;
}
void sht3xd_fetch(const struct device *const device, double *temperature,
                  double *humidity) {
  int rc;

  struct sensor_value temp, hum;

  rc = sensor_sample_fetch(device);
  if (rc == 0) {
    rc = sensor_channel_get(device, SENSOR_CHAN_AMBIENT_TEMP, &temp);
  }
  if (rc == 0) {
    rc = sensor_channel_get(device, SENSOR_CHAN_HUMIDITY, &hum);
  }
  if (rc != 0) {
    printf("SHT3XD: failed: %d\n", rc);
    return;
  }

  *temperature = sensor_value_to_double(&temp);
  *humidity = sensor_value_to_double(&hum);
}