#pragma once
#include "uart/ProtocolSender.h"
#include "net/NetManager.h"
/*
*此文件由GUI工具生成
*文件功能：用于处理用户的逻辑相应代码
*功能说明：
*========================onButtonClick_XXXX
当页面中的按键按下后系统会调用对应的函数，XXX代表GUI工具里面的[ID值]名称，
如Button1,当返回值为false的时候系统将不再处理这个按键，返回true的时候系统将会继续处理此按键。比如SYS_BACK.
*========================onSlideWindowItemClick_XXXX(int index) 
当页面中存在滑动窗口并且用户点击了滑动窗口的图标后系统会调用此函数,XXX代表GUI工具里面的[ID值]名称，
如slideWindow1;index 代表按下图标的偏移值
*========================onSeekBarChange_XXXX(int progress) 
当页面中存在滑动条并且用户改变了进度后系统会调用此函数,XXX代表GUI工具里面的[ID值]名称，
如SeekBar1;progress 代表当前的进度值
*========================ogetListItemCount_XXXX() 
当页面中存在滑动列表的时候，更新的时候系统会调用此接口获取列表的总数目,XXX代表GUI工具里面的[ID值]名称，
如List1;返回值为当前列表的总条数
*========================oobtainListItemData_XXXX(ZKListView::ZKListItem *pListItem, int index)
 当页面中存在滑动列表的时候，更新的时候系统会调用此接口获取列表当前条目下的内容信息,XXX代表GUI工具里面的[ID值]名称，
如List1;pListItem 是贴图中的单条目对象，index是列表总目的偏移量。具体见函数说明
*========================常用接口===============
*LOGD(...)  打印调试信息的接口
*mTextXXXPtr->setText("****") 在控件TextXXX上显示文字****
*mButton1Ptr->setSelected(true); 将控件mButton1设置为选中模式，图片会切换成选中图片，按钮文字会切换为选中后的颜色
*mSeekBarPtr->setProgress(12) 在控件mSeekBar上将进度调整到12
*mListView1Ptr->refreshListView() 让mListView1 重新刷新，当列表数据变化后调用
*mDashbroadView1Ptr->setTargetAngle(120) 在控件mDashbroadView1上指针显示角度调整到120度
*
* 在Eclipse编辑器中  使用 “alt + /”  快捷键可以打开智能提示
*/
#include "mi_common_datatype.h"
#include "mi_wlan.h"
#include <algorithm>

#define TIMER_LOADING	0
#define TIMER_SCAN   	1

typedef struct
{
	char ssid[MI_WLAN_MAX_SSID_LEN];
	char mac[MI_WLAN_MAX_MAC_LEN];
	bool bEncrypt;
	bool bConnected;
	int signalSTR;
} ScanResult_t;

// loading timer
static bool isRegistered = false;
static bool isLoading = false;
static bool loadingStatus = false;	// save last loading status

// scanning timer
static bool isCreated = false;


extern bool isWifiSupport;
extern bool isWifiEnable;
extern bool isSsidSaved;	// if exsit saved ssid&passwd
extern bool isConnected;
extern WLAN_HANDLE wlanHdl;
extern MI_WLAN_ConnectParam_t stConnectParam;
static MI_WLAN_Status_t status;
static MI_WLAN_ScanResult_t scanResult;
static int namedAPNumber = 0;
static ScanResult_t *pScanResult = NULL;
static std::vector<ScanResult_t> sScanResult;
static Mutex lLock;

static bool bLogOn = true;
#define WIFI_LOG(fmt, args...) {if(bLogOn) {printf("\033[1;34m");printf("%s[%d]:", __FUNCTION__, __LINE__);printf(fmt, ##args);printf("\033[0m");}}

static void saveScanResult(MI_WLAN_ScanResult_t *pScanResult, std::vector<ScanResult_t>* scanInfo)
{
	int cnt = pScanResult->u8APNumber;

	scanInfo->clear();

	for(int i = 0; i < cnt;i++)
	{
		char *pSsid = (char*)pScanResult->stAPInfo[i].au8SSId;
		ScanResult_t stScanRes;
		memset(&stScanRes, 0, sizeof(ScanResult_t));
		if (pSsid && strcmp(pSsid, "\"\""))
		{
			int len = strlen(pSsid);
			strncpy(stScanRes.ssid, pSsid+1, len-2);
			strcpy(stScanRes.mac, (char*)pScanResult->stAPInfo[i].au8Mac);
			stScanRes.bEncrypt = pScanResult->stAPInfo[i].bEncryptKey;
			stScanRes.signalSTR = pScanResult->stAPInfo[i].stQuality.signalSTR;
			if (isConnected && !strcmp(stScanRes.ssid, (char*)stConnectParam.au8SSId))
				stScanRes.bConnected = true;
			scanInfo->push_back(stScanRes);
		}
	}
}

static bool sortDesc(ScanResult_t a, ScanResult_t b)
{
	return (a.signalSTR > b.signalSTR);
}

static void sortResultBySignalSTR(std::vector<ScanResult_t>* scanInfo)
{
	// disp connected ssid first
	if (isConnected)
	{
		for (vector<ScanResult_t>::iterator it = scanInfo->begin(); it != scanInfo->end(); it++){
			if (it->bConnected)
				swap(*(scanInfo->begin()), *it);
		}
		sort(scanInfo->begin()+1, scanInfo->end(), sortDesc);
	}
	else
	{
		sort(scanInfo->begin(), scanInfo->end(), sortDesc);
	}
}

static void registerPrivTimer(bool *pIsRegistered, int id, int time) {
     //如果没有注册才进行注册定时器
     if (!(*pIsRegistered)) {
         mActivityPtr->registerUserTimer(id, time);
         *pIsRegistered = true;
     }
 }

static void unregisterPrivTimer(bool *pIsRegistered, int id) {
    //如果已经注册了定时器，则取消注册
    if (*pIsRegistered) {
        mActivityPtr->unregisterUserTimer(id);
        *pIsRegistered = false;
    }
}

static void updateAnimation() {
    char path[50];
    static int animationIndex = 0;
    sprintf(path,"textview/loading_%d.png",animationIndex++ % 60);
    mTextview_loadingPtr->setBackgroundPic(path);
}

class WifiLoadThread : public Thread {
protected:
	virtual bool threadLoop() {
		if (isWifiSupport && isWifiEnable)
		{
			printf("load thread running\n");
			if (isLoading)
				updateAnimation();
			else
			{
				printf("scan done, isloading is %d\n", isLoading);
				return false;
			}
		}
		return true;
	}
};

class WifiScanThread : public Thread {
protected:
	virtual bool threadLoop() {
		int sleepTime = 5000;
		if (isWifiSupport && isWifiEnable)
		{
			WIFI_LOG("scan thread running\n");
			//scanResult.u8APNumber = MI_WLAN_MAX_APINFO_NUM;
			MI_WLAN_Scan(NULL, &scanResult);

			if (isLoading && scanResult.u8APNumber == 0)
			{
				sleepTime = 50;
			}
			else
			{
				isLoading = false;
			}

			if (!isLoading)
			{
				unregisterPrivTimer(&isRegistered, TIMER_LOADING);
				mTextview_loadingPtr->setVisible(FALSE);
				mListviewNetworkPtr->setVisible(TRUE);
				lLock.lock();
				saveScanResult(&scanResult, &sScanResult);
				sortResultBySignalSTR(&sScanResult);
				lLock.unlock();
			}

			if (mListviewNetworkPtr->isVisible())
			{
				mListviewNetworkPtr->refreshListView();
			}

			WIFI_LOG("scan done, isloading is %d\n", isLoading);
		}
		//return false;
		sleep(sleepTime);
		return true;
	}
};

class WifiConnectThread : public Thread {
protected:
	virtual bool threadLoop() {
		// get wifi status from config file
		// set isWifiSupport, isWifiEnable, isSsidSaved, stConnectParam
		MI_WLAN_Connect(&wlanHdl, &stConnectParam);
		MI_WLAN_GetStatus(&status);

		if(status.stStaStatus.state == WPA_COMPLETED)
		{
			WIFI_LOG("wifi connect success: %s %s\n", status.stStaStatus.ip_address, status.stStaStatus.ssid);
			isConnected = true;
			isSsidSaved = true;

			// check if status changed in ap list, if changed, refresh listview
			lLock.lock();
			for (vector<ScanResult_t>::iterator it = sScanResult.begin(); it != sScanResult.end(); it++){
				if (!strcmp((char*)status.stStaStatus.ssid, it->ssid))
				{
					it->bConnected = true;
					swap(*(sScanResult.begin()), *it);
					break;
				}
			}

			if (sScanResult.size() > 1)
				sort(sScanResult.begin()+1, sScanResult.end(), sortDesc);
			lLock.unlock();

			WIFI_LOG("get wifi info\n");
			if (mListviewNetworkPtr->isVisible())
			{
				mListviewNetworkPtr->refreshListView();
			}

			WIFI_LOG("update listview\n");
		}
		else
			WIFI_LOG("wifi connect failed\n");

		return false;
	}
};

static WifiScanThread wifiScanThread;
static WifiConnectThread wifiConnectThread;

/**
 * 注册定时器
 * 填充数组用于注册定时器
 * 注意：id不能重复
 */
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	//{0,  6000}, //定时器id=0, 时间间隔6秒
	//{TIMER_SCAN,  5000},
};


/**
 * 当界面构造时触发
 */
static void onUI_init(){
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");
	WIFI_LOG("onUI_init\n");
}

/**
 * 当切换到该界面时触发
 */
static void onUI_intent(const Intent *intentPtr) {
	WIFI_LOG("onUI_intent\n");

	// get wifi config setting

	// check if support wifi
//	if (isWifiSupport)
//	{
//		mTextviewNotSupportPtr->setVisible(FALSE);
//		mTextviewWifiPtr->setVisible(TRUE);
//		mButtonWifiswPtr->setVisible(TRUE);
//		mButtonWifiswPtr->setSelected(isWifiEnable);
//		mTextviewWifiListPtr->setVisible(isWifiEnable);
//		mTextview_loadingPtr->setVisible(isWifiEnable);
//
//		if (isWifiEnable)
//		{
//			isLoading = true;
//			loadingStatus = true;
//
//			printf("register loading timer\n");
//			registerPrivTimer(&isRegistered, TIMER_LOADING, 50);
//			//registerPrivTimer(&isCreated, TIMER_SCAN, 50);
//
//			if (!wifiScanThread.isRunning())
//			{
//				printf("start new thread\n");
//				wifiScanThread.run("wifiScan");
//			}
//		}
//	}
//	else
//	{
//		mTextviewNotSupportPtr->setVisible(TRUE);
//		mTextviewWifiPtr->setVisible(FALSE);
//		mButtonWifiswPtr->setVisible(FALSE);
//		mTextviewWifiListPtr->setVisible(FALSE);
//		mTextview_loadingPtr->setVisible(FALSE);
//	}
//
//	mListviewNetworkPtr->setVisible(FALSE);
}

/*
 * 当界面显示时触发
 */
static void onUI_show() {
	WIFI_LOG("onUI_show\n");

	if (isWifiSupport)
	{
		mTextviewNotSupportPtr->setVisible(FALSE);
		mTextviewWifiPtr->setVisible(TRUE);
		mButtonWifiswPtr->setVisible(TRUE);
		mButtonWifiswPtr->setSelected(isWifiEnable);
		mTextviewWifiListPtr->setVisible(isWifiEnable);
		mTextview_loadingPtr->setVisible(isWifiEnable);

		if (isWifiEnable)
		{
			isLoading = true;
			loadingStatus = true;

			WIFI_LOG("register loading timer\n");
			registerPrivTimer(&isRegistered, TIMER_LOADING, 50);
			//registerPrivTimer(&isCreated, TIMER_SCAN, 50);

			if (!wifiScanThread.isRunning())
			{
				WIFI_LOG("start new thread\n");
				wifiScanThread.run("wifiScan");
			}
		}
	}
	else
	{
		mTextviewNotSupportPtr->setVisible(TRUE);
		mTextviewWifiPtr->setVisible(FALSE);
		mButtonWifiswPtr->setVisible(FALSE);
		mTextviewWifiListPtr->setVisible(FALSE);
		mTextview_loadingPtr->setVisible(FALSE);
	}

	mListviewNetworkPtr->setVisible(FALSE);

	lLock.lock();
	sScanResult.clear();
	lLock.unlock();
}

/*
 * 当界面隐藏时触发
 */
static void onUI_hide() {
	WIFI_LOG("onUI_hide\n");
}

/*
 * 当界面完全退出时触发
 */
static void onUI_quit() {
	WIFI_LOG("onUI_quit\n");
	if (wifiScanThread.isRunning())
	{
		WIFI_LOG("wait thread exit\n");
		wifiScanThread.requestExitAndWait();
	}

	unregisterPrivTimer(&isRegistered, TIMER_LOADING);
	//unregisterPrivTimer(&isCreated, TIMER_SCAN);
	lLock.lock();
	sScanResult.clear();
	lLock.unlock();
}

/**
 * 串口数据回调接口
 */
static void onProtocolDataUpdate(const SProtocolData &data) {

}

/**
 * 定时器触发函数
 * 不建议在此函数中写耗时操作，否则将影响UI刷新
 * 参数： id
 *         当前所触发定时器的id，与注册时的id相同
 * 返回值: true
 *             继续运行当前定时器
 *         false
 *             停止运行当前定时器
 */
static bool onUI_Timer(int id){
	switch (id) {
		case TIMER_LOADING:
			{
				WIFI_LOG("loading timer tick, preLoadStatus=%d, loadStatus=%d\n", loadingStatus, isLoading);
				if (isLoading)
				{
					if (loadingStatus != isLoading)
					{
						mListviewNetworkPtr->setVisible(FALSE);
						mTextview_loadingPtr->setVisible(TRUE);
					}

					// load pic
					updateAnimation();
				}
//				else
//				{
//					if (loadingStatus != isLoading)
//					{
//						mTextview_loadingPtr->setVisible(FALSE);
//						mListviewNetworkPtr->setVisible(TRUE);
//					}
//				}

				loadingStatus = isLoading;
			}
			break;
		case TIMER_SCAN:
			if (!wifiScanThread.isRunning())
			{
				WIFI_LOG("start scan...\n");
				if (!isLoading)
				{
					mTextview_loadingPtr->setVisible(FALSE);
					mListviewNetworkPtr->setVisible(TRUE);
				}
				WIFI_LOG("refresh listview\n");
				mListviewNetworkPtr->refreshListView();

				WIFI_LOG("scan %d signals:\n", scanResult.u8APNumber);
				for (int i = 0; i < scanResult.u8APNumber; i++)
				{
					WIFI_LOG("ssid: %s, level: %d\n", (char *)scanResult.stAPInfo[i].au8SSId,
							scanResult.stAPInfo[i].stQuality.signalSTR);
				}

				WIFI_LOG("wait thread exit\n");
				wifiScanThread.requestExitAndWait();

				WIFI_LOG("start new thread\n");
				wifiScanThread.run("wifiScan");
			}
			break;
		default:
			break;
	}
    return true;
}

/**
 * 有新的触摸事件时触发
 * 参数：ev
 *         新的触摸事件
 * 返回值：true
 *            表示该触摸事件在此被拦截，系统不再将此触摸事件传递到控件上
 *         false
 *            触摸事件将继续传递到控件上
 */
static bool onnetworkSettingActivityTouchEvent(const MotionEvent &ev) {
    switch (ev.mActionStatus) {
		case MotionEvent::E_ACTION_DOWN://触摸按下
			//LOGD("时刻 = %ld 坐标  x = %d, y = %d", ev.mEventTime, ev.mX, ev.mY);
			break;
		case MotionEvent::E_ACTION_MOVE://触摸滑动
			break;
		case MotionEvent::E_ACTION_UP:  //触摸抬起
			break;
		default:
			break;
	}
	return false;
}

static bool onButtonClick_ButtonWifisw(ZKButton *pButton) {
    //LOGD(" ButtonClick ButtonWifisw !!!\n");
	WIFI_LOG("onButtonClick_ButtonWifisw\n");

	//BOOL bWifiEnable = FALSE;
	pButton->setSelected(!pButton->isSelected());
	isWifiEnable = pButton->isSelected();
	mTextviewWifiListPtr->setVisible(isWifiEnable);
	mTextview_loadingPtr->setVisible(isWifiEnable);
	mListviewNetworkPtr->setVisible(FALSE);

	if (isWifiEnable)
	{
		if (!isConnected && isSsidSaved)
		{
			// try to connect saved ssid
			WIFI_LOG("start conn thread\n");
			wifiConnectThread.run("wifiConnect");
		}

		isLoading = true;
		loadingStatus = true;

		WIFI_LOG("register loading timer\n");
		registerPrivTimer(&isRegistered, TIMER_LOADING, 50);
		//registerPrivTimer(&isCreated, TIMER_SCAN, 50);

		if (!wifiScanThread.isRunning())
		{
			WIFI_LOG("start new thread\n");
			wifiScanThread.run("wifiScan");
		}
	}
	else
	{
		if (wifiScanThread.isRunning())
		{
			WIFI_LOG("wait thread exit\n");
			wifiScanThread.requestExitAndWait();
		}

		unregisterPrivTimer(&isRegistered, TIMER_LOADING);

		if (wlanHdl != -1)
		{
			MI_WLAN_Disconnect(wlanHdl);
			wlanHdl = -1;
			isConnected = false;
		}

		lLock.lock();
		sScanResult.clear();
		lLock.unlock();
	}

    return false;
}

static int getListItemCount_ListviewNetwork(const ZKListView *pListView) {
    //LOGD("getListItemCount_ListviewNetwork !\n");
//	printf("getListItemCount_ListviewNetwork\n");
//	printf("scan result number is %d\n", scanResult.u8APNumber);
	int size = 0;
	lLock.lock();
	size = sScanResult.size();
	lLock.unlock();

	return size;
}

static void obtainListItemData_ListviewNetwork(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ ListviewNetwork  !!!\n");
	//printf("obtainListItemData_ListviewNetwork\n");

	ZKListView::ZKListSubItem *pLevelItem = pListItem->findSubItemByID(ID_NETWORKSETTING_SubItemSignal);
	ZKListView::ZKListSubItem *pNameItem = pListItem->findSubItemByID(ID_NETWORKSETTING_SubItemNetworkID);
	ZKListView::ZKListSubItem *pEncryItem = pListItem->findSubItemByID(ID_NETWORKSETTING_SubItemEncry);
	ZKListView::ZKListSubItem *pConnectStatusItem = pListItem->findSubItemByID(ID_NETWORKSETTING_SubItemConnected);
	lLock.lock();
	const ScanResult_t &scanRes = sScanResult.at(index);
	lLock.unlock();

	pNameItem->setText(scanRes.ssid);
	pEncryItem->setVisible(scanRes.bEncrypt);

	if (scanRes.signalSTR > -50) {
		pLevelItem->setBackgroundPic("wifi/wifi_signal_4.png");
	} else if (scanRes.signalSTR > -75) {
		pLevelItem->setBackgroundPic("wifi/wifi_signal_3.png");
	} else if (scanRes.signalSTR > -90) {
		pLevelItem->setBackgroundPic("wifi/wifi_signal_2.png");
	} else {
		pLevelItem->setBackgroundPic("wifi/wifi_signal_1.png");
	}

	if (scanRes.bConnected) {
		pConnectStatusItem->setVisible(TRUE);
	} else {
		pConnectStatusItem->setVisible(FALSE);
	}
}

static void onListItemClick_ListviewNetwork(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ ListviewNetwork  !!!\n");
	lLock.lock();
	const ScanResult_t &scanRes = sScanResult.at(index);
	lLock.unlock();

	if (wifiScanThread.isRunning())
	{
		WIFI_LOG("wait thread exit\n");
		wifiScanThread.requestExitAndWait();
	}

	unregisterPrivTimer(&isRegistered, TIMER_LOADING);

	if (isConnected && !index)
	{
		// show connected info
		if (!strcmp(scanRes.ssid, (char*)stConnectParam.au8SSId))
			EASYUICONTEXT->openActivity("networkSetting2Activity");
		else
		{
			Intent* intent = new Intent();
			intent->putExtra("ssid", string(scanRes.ssid));
			EASYUICONTEXT->openActivity("networkSetting3Activity", intent);
		}
	}
	else
	{
		// show select ssid info
		Intent* intent = new Intent();
		intent->putExtra("ssid", string(scanRes.ssid));
		EASYUICONTEXT->openActivity("networkSetting3Activity", intent);
	}
	// get item ssid, compare with saved ssid

	// if equal, open page to see info of connecting ssid

	// else , open page to connect new ssid

}

static bool onButtonClick_sys_back(ZKButton *pButton) {
    //LOGD(" ButtonClick sys_back !!!\n");
    return false;
}
