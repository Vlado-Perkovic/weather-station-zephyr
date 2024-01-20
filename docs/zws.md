# Weather Station Zephyr


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

### Upravljanje dht11 senzorom

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

Isto tako, budući da upravljanje mnogim senzorima svodi se na iste radnje, postoji knjižnica (eng. *library*) sa korisnim funkcijama (https://docs.zephyrproject.org/latest/hardware/peripherals/sensor.html).


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

### Upravljanje senzorom svjetline

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

### WI-FI

Budući da razvojna pločica `esp32-devkitc-wrover` ima Wi-Fi modul, dobra je ideja integrirati mogućnosti te tehnologije u naš projekt.
Zephyr pruža podršku za Wi-Fi preko svojih [upravljačkih programa](https://docs.zephyrproject.org/latest/doxygen/html/group__wifi__mgmt.html).

Ovdje dolazimo do jedne zanimljive mogućnosti operacijskog sustava Zephyr, a to su interaktivne ljuske (eng. *interactive shells*).

#### ZEPHYR SHELL

Zephyr RTOS ima lagani podsustav ljuske koji nudi interaktivno sučelje naredbenog retka (eng. *Command Line Interface*). Jedna od dostupnih ljuski je [Wi-Fi ljuska](https://docs.zephyrproject.org/latest/samples/net/wifi/README.html) koja programerima omogućuje izvršavanje zadataka kao što su omogućavanje Wi-Fi-ja, povezivanje s mrežama i provjera statusa veze pomoću naredbi kao što su `wifi scan`, `wifi connect <SSID> <password>` i `wifi status`. To olakšava konfiguraciju i otklanjanje pogrešaka u stvarnom vremenu bez ponovnog prevođenja cijelog programa. Stvarne naredbe i funkcionalnosti mogu se razlikovati ovisno o Zephyr verziji, konfiguraciji i hardveru koji se koristi.

Najjednostavnije korištenje ljuske bilo bi pokrenuti Zephyr-ov uzorak za Wi-Fi naredbom: 
```bash
west build -b <ime-plocice> samples/net/wifi
```

Primjer ispisa konzole u serijskom terminalu (npr. minicom):
```bash
shell> wifi scan
Scan requested
shell>
Num  | SSID                             (len) | Chan | RSSI | Sec
1    | kapoueh!                         8     | 1    | -93  | WPA/WPA2
2    | mooooooh                         8     | 6    | -89  | WPA/WPA2
3    | Ap-foo blob..                    13    | 11   | -73  | WPA/WPA2
4    | gksu                             4     | 1    | -26  | WPA/WPA2
----------
Scan request done

shell> wifi connect "gksu" 4 SecretStuff
Connection requested
shell>
Connected
shell>
```

Upravo ovim načinom je istestirana komponenta Wi-Fi prije implementacije u projekt.

Da se ne ulazi u duboke detalje implementacije upravljačkog programa za Wi-Fi, opisat ću ukratko kako se to ostvarilo, dok se detalji mogu iščitati iz koda.
 
Upravljanje Wi-Fi mrežom s mehanizmom rukovatelja događajima (eng. *Event Handler*) uključuje korištenje programiranja vođenog događajima (eng. *Event-Driven Approach*) za rukovanje događajima povezanim s Wi-Fi mrežom na način da umjesto traženja promjena (eng. *polling*), sustav generira događaje kada se dogode određeni Wi-Fi događaji kao što su status veze, rezultati skeniranja ili prekid veze, a rukovatelj događajima odgovara na te događaje.

##### Semafori
Za sinkronizaciju unutar mehanizma korišteni su [semafori operacijskog sustava Zephyr](https://docs.zephyrproject.org/latest/kernel/services/synchronization/semaphores.html).
U Zephyr RTOS-u, semafori su sinkronizacijske primitive koje se koriste za kontrolu pristupa zajedničkim resursima među više zadataka ili dretvi. Oni rade održavajući broj, dopuštajući zadacima da ih dobiju i oslobode. Kada semafor nije dostupan (broj je nula), zadaci koji ga pokušavaju nabaviti blokiraju se dok ne postane dostupan. Zephyr podržava i binarne semafore (broj 0 ili 1) i semafore za brojanje (proizvoljni broj). Semaphore API uključuje funkcije kao što su `k_sem_take()` i `k_sem_give()` za dobivanje odnosno otpuštanje semafora. Semafori igraju ključnu ulogu u sprječavanju uvjeta utrke i osiguravanju urednog izvršavanja u multitasking okruženjima.

### MQTT
MQTT u Zephyr RTOS-u olakšava laganu i učinkovitu komunikaciju između uređaja pomoću modela objavljivanja i pretplate. [Zephyrova MQTT knjižnica](https://docs.zephyrproject.org/latest/connectivity/networking/api/mqtt.html) pruža funkcije za povezivanje s brokerom, objavljivanje poruka i pretplatu na teme, što je čini prikladnom za ugrađene sustave niske propusnosti, posebno u IoT aplikacijama.

Implementacija se zasniva na istom mehanizmu kao i Wi-Fi.

### GENERAL

#### dretve
U Zephyr RTOS-u [dretve](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html) su lagane, neovisne jedinice izvršenja koje dijele isti memorijski prostor. Zephyr podržava višedretvenost, dopuštajući istovremeno izvođenje više dretvi. dretve se stvaraju pomoću `k_thread_create()` API-ja, a *scheduler* upravlja njihovim izvršenjem na temelju prioriteta. Dretve mogu komunicirati putem zajedničke memorije, semafora i drugih mehanizama sinkronizacije. Oni prolaze kroz stanja kao što su `READY`, `RUNNING` i `SUSPENDED`, i mogu biti u stanju čekanja za događaje ili semafore. Zephyrove značajke u stvarnom vremenu čine ga prikladnim za vremenski osjetljive aplikacije, a dretve mogu rukovati prekidima i izvršavati rutine usluge prekida (ISR). U aplikacijama su dretve definirane za istovremeno obavljanje specifičnih zadataka i mogu komunicirati kroz primitive za sinkronizaciju kao što su semafori ili redovi poruka.

Primjer osnovnog korištenja dretvi:
```c
K_THREAD_STACK_DEFINE(zws_stack, THREAD_STACK_SIZE);
static struct k_thread zws_thread;
static char thread_stack[THREAD_STACK_SIZE];

int main() {
  ...
  k_tid_t my_tid = k_thread_create(&zws_thread, zws_stack,
                                  K_THREAD_STACK_SIZEOF(zws_stack),
                                  sensor_thread,
                                  NULL, NULL, NULL,
                                 THREAD_PRIORITY, 0, K_NO_WAIT);
}
```
#### logging
U Zephyr RTOS-u, sustav za bilježenje (eng. *logging*) nudi programerima API-je za prikaz dijagnostičkih informacija na različitim razinama važnosti. Ovaj sustav podržava dinamičku konfiguraciju, razne pozadine (uključujući izlaz konzole i datoteke) i prilagodljive formate dnevnika. Programeri koriste makronaredbe za bilježenje kao što su `LOG_ERR()` i `LOG_INF()` u svom kodu, s opcijama za uvjetno bilježenje na temelju specifičnih uvjeta. Konfiguracijske postavke omogućuju kontrolu nad ponašanjem zapisivanja, omogućujući programerima da prilagode sustav svojim potrebama.

Primjer inicijalizacije i korištenja:
```c
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zws, CONFIG_LOG_DEFAULT_LEVEL);

static int
mqtt_topic_callback(struct mqtt_service *service, const char *topic, size_t payload_len)
{

  LOG_ERR("topic: %s", topic);
  return 0;
}
```

### Troubleshooting 16x2 LCD display
Pri izradi projekta planirano je prikazivati podatke senzora na LCD prikaznik. Koristeći interaktivnu ljusku pokušao sam prikazati barem jedno slovo na zaslon preko I2C protokola kako bi utvrdio da su adrese uređaja dobre. I2C skeniranje bi prepoznalo prikaznik na popularnoj `0x27` adresi no upisivanje testnih naredbi nije vraćalo ispravne rezultate. Npr., upis slova "A" bi ugasilo ekran, dok neke druge naredbe bi ga upalile. Potrebno je dodatno provjeriti prikaznik.

### Troubleshooting MQTT vs dht11
Pojavio se problem gdje očitavanje senzora ne bi uspijevalo kada bi se inicijalizirao MQTT. Isti problem se pojavljivao i ranije kada bi interaktivna ljuska bila omogućena. Na Internetu ne pronalazim ništa relevantno.