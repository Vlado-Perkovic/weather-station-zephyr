#include "zws_utils.h"
#include <stdint.h>
#include <zephyr/kernel.h>

const char *now_str(void) {
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
void zws_utils_display(double temp, double humid, double light) {

  printk("[%s]: %.2f Cel ; %.2f %%RH ;  %.2f %%\n", now_str(), temp, humid,
         light);
}
