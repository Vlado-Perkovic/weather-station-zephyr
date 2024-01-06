#include <sys/_stdint.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <stdio.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) ||                                   \
    !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx)                                  \
  ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

#define MAX_PHOTORESISTOR_VALUE 1000.0
#define MIN_PHOTORESISTOR_VALUE 279.0

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

static const char *now_str(void) {
  static char buf[16]; /* ...HH:MM:SS.MMM */
  uint32_t now = k_uptime_get_32();
  unsigned int ms = now % MSEC_PER_SEC;
  unsigned int s;
  unsigned int min;
  unsigned int h;

  now /= MSEC_PER_SEC;
  s = now % 60U;
  now /= 60U;
  min = now % 60U;
  now /= 60U;
  h = now;

  snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u", h, min, s, ms);
  return buf;
}

int main(void) {
  const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);

  if (!device_is_ready(dht11)) {
    printf("Device %s is not ready\n", dht11->name);
    return 0;
  }

  /* ------------------------------------------------------- */
  int err;
  uint32_t count = 0;
  uint16_t buf;
  struct adc_sequence sequence = {
      .buffer = &buf,
      /* buffer size in bytes, not number of samples */
      .buffer_size = sizeof(buf),
  };

  /* Configure channels individually prior to sampling. */
  for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
    if (!adc_is_ready_dt(&adc_channels[i])) {
      printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
      return 0;
    }

    err = adc_channel_setup_dt(&adc_channels[i]);
    if (err < 0) {
      printk("Could not setup channel #%d (%d)\n", i, err);
      return 0;
    }
  }

  /* ------------------------------------------------------- */

  while (true) {
    int rc = sensor_sample_fetch(dht11);

    if (rc != 0) {
      printf("Sensor fetch failed: %d\n", rc);
      /* break; */
    }

    struct sensor_value temperature;
    struct sensor_value humidity;

    rc = sensor_channel_get(dht11, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
    if (rc == 0) {
      rc = sensor_channel_get(dht11, SENSOR_CHAN_HUMIDITY, &humidity);
    }
    if (rc != 0) {
      printf("get failed: %d\n", rc);
      break;
    }

    /* --------------------------------------------------------------- */

    int32_t val_mv;
    for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {

      (void)adc_sequence_init_dt(&adc_channels[i], &sequence);

      err = adc_read_dt(&adc_channels[i], &sequence);
      if (err < 0) {
        printk("Could not read (%d)\n", err);
        continue;
      }

      /*
       * If using differential mode, the 16 bit value
       * in the ADC sample buffer should be a signed 2's
       * complement value.
       */
      if (adc_channels[i].channel_cfg.differential) {
        val_mv = (int32_t)((int16_t)buf);
      } else {
        val_mv = (int32_t)buf;
      }
    }

    float val_mv_scaled = (val_mv >= MAX_PHOTORESISTOR_VALUE)
                              ? 100.0
                              : (val_mv - MIN_PHOTORESISTOR_VALUE) * 100.0 /
                                    MAX_PHOTORESISTOR_VALUE;
    printk("[%s]: %.1f Cel ; %.1f %%RH ;  %.1f %%\n", now_str(),
           sensor_value_to_double(&temperature),
           sensor_value_to_double(&humidity), val_mv_scaled);

    k_sleep(K_SECONDS(2));
  }
  return 0;
}
