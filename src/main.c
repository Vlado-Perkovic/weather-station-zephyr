#include "wifi/zws_wifi.h"
#include "sensors/zws_temp_and_humid_sensor.h"
#include "sensors/zws_photoresistor_sensor.h"
#include "zws_utils.h"

#include <stdio.h>

int main(void)
{

  const struct device *const dht11 = zws_setup_dht11();

  printk("WiFi: \n");
  wifi_setup();
  wifi_start();
  printk("Ready...\n\n");

  double temp, humid, light;

  while (true)
  {

    zws_dht11_fetch_data(dht11, &temp, &humid);

    light = zws_photoresistor_fetch();
    zws_utils_display(temp, humid, light);

    k_sleep(K_SECONDS(2));
  }
  return 0;
}
