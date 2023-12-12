#Weather Station Zephyr


Projekt "Weather Station Zephyr" obuhvaća aplikaciju i prototip uređaja za mjerenje temperature, vlage i svjetlosti. Cilj projekta je upoznati se sa Zephyr RTOS-om.


Razvojna pločica: `esp32-devkitc-wrover`: https://docs.zephyrproject.org/latest/boards/xtensa/esp32_devkitc_wrover/doc/index.html

Senzor temperature i vlage: `dht11`: https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf

Senzor svjetlosti: ``:

Lcd zaslon: ``:



Pregled direktorija
```css
├── app.overlay
├── build
│   ├── app
│   ├── build.ninja
│   ├── CMakeCache.txt
│   ├── CMakeFiles
│   ├── cmake_install.cmake
│   ├── compile_commands.json
│   ├── esp-idf
│   ├── Kconfig
│   ├── modules
│   ├── sysbuild_modules.txt
│   ├── zephyr
│   ├── zephyr_modules.txt
│   └── zephyr_settings.txt
├── CMakeLists.txt
├── docs
│   └── test_komponenti.md
├── prj.conf
├── README.md
├── src
│   └── main.c
└── weather_station_todo.txt
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



Isječak programskog koda vezan uz upravljanje senzorom dht11.
```c
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>


int main(void)
{
  const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);

  if (!device_is_ready(dht11))
  {
    printf("Device %s is not ready\n", dht11->name);
    return 0;
  }

  while (true)
  {
    int rc = sensor_sample_fetch(dht11);

    if (rc != 0)
    {
      printf("Sensor fetch failed: %d\n", rc);
      break;
    }

    struct sensor_value temperature;
    struct sensor_value humidity;

    rc = sensor_channel_get(dht11, SENSOR_CHAN_AMBIENT_TEMP,
                            &temperature);
    if (rc == 0)
    {
      rc = sensor_channel_get(dht11, SENSOR_CHAN_HUMIDITY,
                              &humidity);
    }
    if (rc != 0)
    {
      printf("get failed: %d\n", rc);
      break;
    }

    printf("[%s]: %.1f Cel ; %.1f %%RH\n",
           now_str(),
           sensor_value_to_double(&temperature),
           sensor_value_to_double(&humidity));

    k_sleep(K_SECONDS(2));

  }
  return 0;
}
```

###Upravljanje senzorom svjetline
\# todo