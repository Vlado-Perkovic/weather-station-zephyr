#include "zws_photoresistor_sensor.h"
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>

#include <stdint.h>
#include <stdlib.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
    !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
    ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

#define MAX_PHOTORESISTOR_VALUE 1000.0
#define MIN_PHOTORESISTOR_VALUE 279.0

static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

static uint16_t buf;
static struct adc_sequence sequence = {
    .buffer = &buf,
    /* buffer size in bytes, not number of samples */
    .buffer_size = sizeof(buf),
};

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

        /* * If using differential mode, the 16 bit value */
        /* * in the ADC sample buffer should be a signed 2's */
        /*      * complement value. */
        /*      */
        if (adc_channels[i].channel_cfg.differential)
        {
            val_mv = (int32_t)((int16_t)buf);
            /* ------------------------------------------------------- */
        }
        else
        {
            val_mv = (int32_t)buf;
        }
    }

    float val_mv_scaled = (val_mv >= MAX_PHOTORESISTOR_VALUE)
                              ? 100.0
                              : (val_mv - MIN_PHOTORESISTOR_VALUE) * 100.0 /
                                    MAX_PHOTORESISTOR_VALUE;
    return val_mv_scaled;
}