#ifndef __WIFI_H__
#define __WIFI_H__

#include <string>

void wifi_sta_init();
void wifi_sta_config(const std::string& s_ssid, const std::string& s_password);

#endif /* __WIFI_H__ */