#include "wifi/zws_wifi.h"
#include "sensors/zws_temp_and_humid_sensor.h"
#include "sensors/zws_photoresistor_sensor.h"
#include "zws_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(void)
{

  const struct device *const dht11 = zws_setup_dht11();
  zws_photoresistor_setup();

  printk("WiFi: \n");
  wifi_setup();
  wifi_start();
  printk("Ready...\n\n");

  double temp = 0.0f, humid = 0.0f, light = 0.0f;

  int err;

  while (true)
  {

    zws_dht11_fetch_data(dht11, &temp, &humid);

    light = zws_photoresistor_fetch();

    zws_utils_display(temp, humid, light);

    k_sleep(K_SECONDS(5));
  }
  return 0;
}
