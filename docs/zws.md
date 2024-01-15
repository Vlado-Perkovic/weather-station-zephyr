#Weather Station Zephyr


Projekt "Weather Station Zephyr" obuhvaća aplikaciju i prototip uređaja za mjerenje temperature, vlage i svjetlosti. Cilj projekta je upoznati se sa Zephyr RTOS-om.


Razvojna pločica: `esp32-devkitc-wrover`: https://docs.zephyrproject.org/latest/boards/xtensa/esp32_devkitc_wrover/doc/index.html

Senzor temperature i vlage: `dht11`: https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf

Senzor svjetlosti: `GL5516`:




Pregled direktorija
```css
├── app.overlay
├── build
│   ├── app
│   ├── build.ninja
│   ├── CMakeCache.txt
│   ├── CMakeFiles
│   ├── cmake_install.cmake
│   ├── compile_commands.json
│   ├── esp-idf
│   ├── Kconfig
│   ├── modules
│   ├── sysbuild_modules.txt
│   ├── zephyr
│   ├── zephyr_modules.txt
│   └── zephyr_settings.txt
├── CMakeLists.txt
├── docs
│   ├── hello_world.md
│   └── zws.md
├── prj.conf
├── README.md
└── src
    ├── main.c
    ├── sensors
    ├── telemetry
    ├── wifi
    ├── zws_utils.c
    └── zws_utils.h
```

###Upravljanje dht11 senzorom

Zephyr RTOS ima bogatu podršku upravljačkih programa za razne senzore pa tako već postoji podrška za senzor dht11 (https://docs.zephyrproject.org/latest/build/dts/api/bindings/sensor/aosong,dht.html).
Pri korištenju postojećih upravljačkih programa, moramo ih navesti kao `compatible = "<ime upravljačkog programa>"`.

Primjer kako to izgleda za dht11.
```css
/ {
	dht11 {
		compatible = "aosong,dht";
		status = "okay";
		dio-gpios = <&gpio0 25 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
	};
```

Isto tako, budući da upravljanje mnogim senzorima svodi se na iste radnje, postoji knjižnica (eng. library) sa korisnim funkcijama (https://docs.zephyrproject.org/latest/hardware/peripherals/sensor.html).


Potrebno je dohvatiti preko macro naredbe pokazivač na uređaj definiran u DT i dalje se koristi apstrahirani API od senzora.
```c
const struct device *const <varijabla> = DEVICE_DT_GET_ONE(<ime drivera>);
```

Isječak programskog koda vezan uz postavljanje senzora dht11 (ili bilo kojeg drugog podržanog senzora).
```c
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
```

```c
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
```
napomena: u ovom projektu pri korištenju Wi-Fija i protokola MQTT uz očitavanje senzora dolazi do pogreške.

###Upravljanje senzorom svjetline

Senzor svjetline `GL5516` zapravo je otpornik koji mijenja svoj otpor promjenom izloženosti svjetlini. Što je veća svjetlina to je manji otpor. Kako bi se razina svjetla izmjerila, napravi se razdjelnik napona te tako preko ADC-a (Analog to Digital Converter) kvantificira se svjetlina preko ulaznog napona.

Kako Zephyr nema posvećeni pokretački program za ovakav tip senzora svjetline, bilo je potrebno koristiti API za ADC.


```c
#include <zephyr/drivers/adc.h>

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
    ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

static uint16_t buf;
static struct adc_sequence sequence = {
    .buffer = &buf,
    /* buffer size in bytes, not number of samples */
    .buffer_size = sizeof(buf),
};
```

```c
void zws_photoresistor_setup()
{
    int err;

    /* Configure channels individually prior to sampling. */
    for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++)
    {
        if (!adc_is_ready_dt(&adc_channels[i]))
        {
            printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
            exit(1);
        }

        err = adc_channel_setup_dt(&adc_channels[i]);
        if (err < 0)
        {
            printk("Could not setup channel #%d (%d)\n", i, err);
            exit(1);
        }
    }
}
```

```c
double zws_photoresistor_fetch()
{
    int32_t val_mv;
    int err;

    for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++)
    {

        (void)adc_sequence_init_dt(&adc_channels[i], &sequence);

        err = adc_read_dt(&adc_channels[i], &sequence);
        if (err < 0)
        {
            printk("Could not read (%d)\n", err);
            continue;
        }

            val_mv = (int32_t)buf;
    }

    float val_mv_scaled = (val_mv >= MAX_PHOTORESISTOR_VALUE)
                              ? 100.0
                              : (val_mv - MIN_PHOTORESISTOR_VALUE) * 100.0 /
                                    MAX_PHOTORESISTOR_VALUE;
    return val_mv_scaled;
}
```

Vrijednost koju pročitamo zatim skaliramo da dobijemo postotak.

###WI-FI
