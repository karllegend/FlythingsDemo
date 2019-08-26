#include "entry/EasyUIContext.h"
#include "uart/UartContext.h"
#include "manager/ConfigManager.h"
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void onEasyUIInit(EasyUIContext *pContext) {
	// 初始化时打开串口
	printf("init uart %s %d\n", CONFIGMANAGER->getUartName().c_str(), CONFIGMANAGER->getUartBaudRate());
	//UARTCONTEXT->openUart(CONFIGMANAGER->getUartName().c_str(), CONFIGMANAGER->getUartBaudRate());
	UARTCONTEXT->openUart("/dev/ttyS1", 115200);
}

void onEasyUIDeinit(EasyUIContext *pContext) {
	UARTCONTEXT->closeUart();
}

const char* onStartupApp(EasyUIContext *pContext) {
//	return "emptyActivity";
	return "mainActivity";
}


#ifdef __cplusplus
}
#endif  /* __cplusplus */

