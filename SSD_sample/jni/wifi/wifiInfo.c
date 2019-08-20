/*
 * wifiInfo.c
 *
 *  Created on: 2019年8月15日
 *      Author: koda.xu
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wifiInfo.h"
#include "cJSON.h"


#define WIFI_SETTING_CFG	"/config/wifisetting.json"

static bool isWifiSupport = true;
static bool isWifiEnable = true;
static bool isSsidSaved = false;	// if exsit saved ssid&passwd
static bool isConnected = false;
static WLAN_HANDLE wlanHdl = -1;
static MI_WLAN_ConnectParam_t stConnectParam;	// abandon
static WIFI_CONFIG_T config = {"0", "SKY", "12345678"};

static void *malloc_fn(size_t sz)
{
	return malloc(sz);
}

static void free_fn(void *ptr)
{
	free(ptr);
}

bool getWifiSupportStatus()
{
	return isWifiSupport;
}

void setWifiSupportStatus(bool enable)
{
	isWifiSupport = enable;
}

bool getWifiEnableStatus()
{
	return isWifiEnable;
}

void setWifiEnableStatus(bool enable)
{
	isWifiEnable = enable;
}

bool getSsidSavedStatus()
{
	return isSsidSaved;
}

void setSsidSavedStatus(bool enable)
{
	isSsidSaved = enable;
}

bool getConnectionStatus()
{
	return isConnected;
}

void setConnectionStatus(bool enable)
{
	isConnected = enable;
}

WLAN_HANDLE getWlanHandle()
{
	return wlanHdl;
}

void setWlanHandle(WLAN_HANDLE handle)
{
	wlanHdl = handle;
}

MI_WLAN_ConnectParam_t * getConnectParam()
{
	return &stConnectParam;
}

void saveConnectParam(MI_WLAN_ConnectParam_t *pConnParam)
{
	memset(&stConnectParam, 0, sizeof(MI_WLAN_ConnectParam_t));
	memcpy(&stConnectParam, pConnParam, sizeof(MI_WLAN_ConnectParam_t));
}

int initWifiConfig()
{
	FILE* fp = NULL;
	long long len = 0;
	char * pConfData = NULL;
	cJSON * root;
	cJSON * obj;
	cJSON * item;
	cJSON_Hooks hooks;

	fp = fopen(WIFI_SETTING_CFG,"r+");
	if (!fp)
	{
		printf("should open json file first\n");
		return -1;
	}

	hooks.free_fn = free_fn;
	hooks.malloc_fn = malloc_fn;
	cJSON_InitHooks(&hooks);

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	pConfData = (char *)malloc(len + 1);
	fseek(fp, 0, SEEK_SET);
	fread(pConfData, len, 1, fp);
	fclose(fp);
	fp = NULL;

	// read config
	cJSON_Minify(pConfData);
	root = cJSON_Parse(pConfData);
	obj = cJSON_GetObjectItem(root, "wifi");
	item = cJSON_GetObjectItem(obj, "id");
	if(item)
	{
		strcpy(config.id, cJSON_GetStringValue(item));
		item = cJSON_GetObjectItem(obj, "ssid");
		strcpy(config.ssid, cJSON_GetStringValue(item));
		item = cJSON_GetObjectItem(obj, "passwd");
		strcpy(config.passwd, cJSON_GetStringValue(item));
	}

	printf("get id:%s ssid:%s passwd:%s\n", config.id, config.ssid, config.passwd);
	return 0;
}

int saveWifiConfig()
{
	FILE* fp = NULL;
	cJSON * root;
	cJSON * obj;
	cJSON * item;

	fp = fopen(WIFI_SETTING_CFG,"w+");
	if (!fp)
	{
		printf("should open json file first\n");
		return -1;
	}

	root = cJSON_CreateObject();
	obj = cJSON_AddObjectToObject(root, "wifi");
	item = cJSON_AddStringToObject(obj, "id", "100");
	item = cJSON_AddStringToObject(obj, "ssid", "test");
	item = cJSON_AddStringToObject(obj, "passwd", "ABCD");
	printf("%s %d %s \n",__FUNCTION__,__LINE__,cJSON_Print(root));

	fseek(fp, 0, SEEK_SET);
	fwrite(cJSON_Print(root),strlen(cJSON_Print(root)),1,fp);
	//fdatasync(fileno(fp));
	fclose(fp);
	fp = NULL;

	return 0;
}
