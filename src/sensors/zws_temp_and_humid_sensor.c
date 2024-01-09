
#include "zws_temp_and_humid_sensor.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

const struct device *const zws_setup_dht11() {
  const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);

  if (!device_is_ready(dht11)) {
    printf("Device %s is not ready\n", dht11->name);
    exit(1);
  }
  return dht11;
}

void zws_dht11_fetch_data(const struct device *const device, double *temp,
                          double *humid) {
  int rc = sensor_sample_fetch(device);

  if (rc != 0) {
    printf("Sensor fetch failed: %d\n", rc);
  }

  struct sensor_value temperature;
  struct sensor_value humidity;

  rc = sensor_channel_get(device, SENSOR_CHAN_AMBIENT_TEMP, &temperature);

  if (rc == 0) {
    rc = sensor_channel_get(device, SENSOR_CHAN_HUMIDITY, &humidity);
  }
  if (rc != 0) {
    printf("get failed: %d\n", rc);
  }
  *temp = sensor_value_to_double(&temperature);
  *humid = sensor_value_to_double(&humidity);
}
