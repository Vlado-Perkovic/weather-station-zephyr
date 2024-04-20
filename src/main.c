#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ws, CONFIG_LOG_DEFAULT_LEVEL);

#include "sensors/photoresistor.h"
#include "sensors/sht3xd.h"
#include "wifi/wifi.h"
#include "telemetry/mqtt_service.h"
#include "utils.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


// Thread specifications
#define THREAD_PRIORITY 7
#define THREAD_STACK_SIZE 1024
#define THREAD_TIMESLICE K_MSEC(100)
K_THREAD_STACK_DEFINE(ws_stack, THREAD_STACK_SIZE);

static struct k_thread ws_thread;

static mqtt_service_t mqtt_service;
static const struct device *sht3xd;
 

/* ws_thread task function */
static void sensor_thread();

static int mqtt_topic_callback(struct mqtt_service *service, const char *topic,
                               size_t payload_len);




int main(void) {

  /* SETUP DEVICES */
  sht3xd = sht3xd_setup();
  photoresistor_setup();

  /* START WIFI */
  wifi_start(WIFI_SSID, WIFI_PSK);

  /* INIT AND START MQTT */
  mqtt_service_init(&mqtt_service, MQTT_CLIENTID, MQTT_BROKER_ADDR,
                    MQTT_BROKER_PORT, mqtt_topic_callback);
  mqtt_service_start(&mqtt_service);

  while (mqtt_service.state != MQTT_SERVICE_CONNECTED) {
    k_sleep(K_MSEC(1000));
  }

  /* CREATE THE MAIN THREAD */
  k_tid_t my_tid = k_thread_create(
      &ws_thread, ws_stack, K_THREAD_STACK_SIZEOF(ws_stack), sensor_thread,
      NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);

  return 0;
}

static void sensor_thread() {
  double light, temp, hum;
  char output[64];

  while (1) {
    sht3xd_fetch(sht3xd, &temp, &hum);
    light = photoresistor_fetch();

    snprintf(output, sizeof(output), "[%s]: %.1f Cel ; %.1f %%RH ; %.1f %%\n",
             now_str(), temp, hum, light);
    mqtt_service_publish(&mqtt_service, "test/topic", MQTT_QOS_0_AT_MOST_ONCE,
                         &output, sizeof(output));
    k_sleep(K_SECONDS(5));
  }
}

static int mqtt_topic_callback(struct mqtt_service *service, const char *topic,
                               size_t payload_len){

  LOG_ERR("topic: %s", topic);
  return 0;
}
