#ifndef TEMP_AND_HUMID_SENSOR_H
#define TEMP_AND_HUMID_SENSOR_H

const struct device *const zws_setup_dht11();

void zws_dht11_fetch_data(const struct device *const device, double *temp,
                          double *humid);

#endif // !TEMP_AND_HUMID_SENSOR_H
