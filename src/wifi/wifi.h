#ifndef WIFI_H
#define WIFI_H

#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PSK CONFIG_WIFI_PSK

void handle_wifi_connect_result(struct net_mgmt_event_callback *cb);

void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb);

void handle_ipv4_result(struct net_if *iface);

void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                             uint32_t mgmt_event, struct net_if *iface);

void wifi_setup(char* ssid, char* psk);

void wifi_start(char* ssid, char* psk);

void wifi_connect(void);

void wifi_status(void);

void wifi_disconnect(void);
#endif // !WIFI_H