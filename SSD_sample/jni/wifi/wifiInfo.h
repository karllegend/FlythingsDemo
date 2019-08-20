/*
 * wifiInfo.h
 *
 *  Created on: 2019年8月15日
 *      Author: koda.xu
 */

#ifndef JNI_WIFIINFO_H_
#define JNI_WIFIINFO_H_

#include "mi_common_datatype.h"
#include "mi_wlan.h"


typedef struct
{
	char id[8];
	char ssid[128];
	char passwd[64];
} WIFI_CONFIG_T;

bool getWifiSupportStatus();
void setWifiSupportStatus(bool enable);
bool getWifiEnableStatus();
void setWifiEnableStatus(bool enable);
bool getSsidSavedStatus();
void setSsidSavedStatus(bool enable);
bool getConnectionStatus();
void setConnectionStatus(bool enable);
WLAN_HANDLE getWlanHandle();
void setWlanHandle(WLAN_HANDLE handle);
MI_WLAN_ConnectParam_t * getConnectParam();
void saveConnectParam(MI_WLAN_ConnectParam_t *pConnParam);
int initWifiConfig();
int saveWifiConfig();

#endif /* JNI_WIFIINFO_H_ */
