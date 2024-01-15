#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zws, CONFIG_LOG_DEFAULT_LEVEL);

#include "wifi/zws_wifi.h"
#include "sensors/zws_temp_and_humid_sensor.h"
#include "sensors/zws_photoresistor_sensor.h"
#include "zws_utils.h"
#include "telemetry/mqtt_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// MQTT client
#define MQTT_CLIENTID "zws"

// MQTT broker address information
#define MQTT_BROKER_ADDR "172.20.10.3"
#define MQTT_BROKER_PORT 1883

#define THREAD_PRIORITY 7
#define THREAD_STACK_SIZE 1024
#define THREAD_TIMESLICE K_MSEC(100)

static mqtt_service_t mqtt_service;
static const struct device *dht11;

static int
mqtt_topic_callback(struct mqtt_service *service, const char *topic, size_t payload_len)
{

  LOG_ERR("topic: %s", topic);
  return 0;
}

K_THREAD_STACK_DEFINE(zws_stack, THREAD_STACK_SIZE);
static struct k_thread zws_thread;
static char thread_stack[THREAD_STACK_SIZE];

static void sensor_thread(void *arg1, void *arg2, void *arg3)
{
  float temp, humid;
  float light;
  char output[64];

  while (1)
  {
    zws_dht11_fetch_data(dht11, &temp, &humid);
    light = zws_photoresistor_fetch();

    zws_utils_display(temp, humid, light);

    snprintf(output, sizeof(output), "[%s]: %.1f Cel ; %.1f %%RH ; %.1f %%\n", now_str(), temp, humid, light);
    mqtt_service_publish(&mqtt_service, "test/topic", MQTT_QOS_0_AT_MOST_ONCE, &output, sizeof(output));

    k_sleep(K_SECONDS(3));
  }
}

int main(void)
{

  dht11 = zws_setup_dht11();
  zws_photoresistor_setup();

  printk("WiFi: \n");
  wifi_setup();
  wifi_start();
  printk("Ready...\n\n");

  mqtt_service_init(&mqtt_service,
                    MQTT_CLIENTID,
                    MQTT_BROKER_ADDR, MQTT_BROKER_PORT,
                    mqtt_topic_callback);
  mqtt_service_start(&mqtt_service);

  while (mqtt_service.state != MQTT_SERVICE_CONNECTED)
  {
    k_sleep(K_MSEC(1000));
  }

  k_tid_t my_tid = k_thread_create(&zws_thread, zws_stack,
                                   K_THREAD_STACK_SIZEOF(zws_stack),
                                   sensor_thread,
                                   NULL, NULL, NULL,
                                   THREAD_PRIORITY, 0, K_NO_WAIT);

  return 0;
}
