/*
 * UartContext.cpp
 *
 *  Created on: Sep 5, 2017
 *      Author: guoxs
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <termio.h>
#include <sys/ioctl.h>

#include "uart/UartContext.h"
#include "utils/Log.h"

#define UART_DATA_BUF_LEN			16384	// 16KB

/* SynchFrame SlaveAddr MasterAddr DataLen Data CheckSum */
/*     2Byte     2Byte      2Byte   2Byte  N Byte  1Byte */
// 最小长度: 2 + 2 + 2 + 2 + 1 = 9
#define DATA_PACKAGE_MIN_LEN		9

extern void procParse(const BYTE *pData, UINT len);

/**
 * 获取校验码
 */
BYTE getCheckSum(const BYTE *pData, int len) {
	int sum = 0;
	for (int i = 0; i < len; ++i) {
		sum += pData[i];
	}

	return (BYTE) (~sum + 1);
}

UartContext::UartContext() :
	mIsOpen(false),
	mUartID(0),
	mDataBufPtr(NULL),
	mDataBufLen(0),
	mHook(NULL){

}

UartContext::~UartContext() {
	delete[] mDataBufPtr;
	closeUart();
}

bool UartContext::openUart(const char *pFileName, UINT baudRate) {
	LOGD("openUart pFileName = %s, baudRate = %d\n", pFileName, baudRate);
	mUartID = open(pFileName, O_RDWR|O_NOCTTY);
	printf("open uart id = %d\n", mUartID);
	if (mUartID <= 0) {
		mIsOpen = false;
	} else {
	    fcntl(mUartID, F_SETFL, 0);
	    struct termios opts;
	    tcgetattr(mUartID, &opts);

	    switch(baudRate)
	    {
	        case 4800:
	            cfsetispeed(&opts,B4800);
	            cfsetospeed(&opts,B4800);
	            break;
	        case 9600:
	            cfsetispeed(&opts,B9600);
	            cfsetospeed(&opts,B9600);
	            break;
	        case 19200:
	            cfsetispeed(&opts,B19200);
	            cfsetospeed(&opts,B19200);
	            break;
	        case 38400:
	            cfsetispeed(&opts,B38400);
	            cfsetospeed(&opts,B38400);
	            break;
	        case 115200:
	            cfsetispeed(&opts,B115200);
	            cfsetospeed(&opts,B115200);
	            break;
	        default:
	            fprintf(stderr,"Unkown baude!\n");
	            return -1;
	    }
	    opts.c_cflag |= CLOCAL|CREAD;

	    // 8N1
	    opts.c_cflag &= ~PARENB;
	    opts.c_cflag &= ~CSTOPB;
	    opts.c_cflag |= CS8;

	    opts.c_cflag &= ~CRTSCTS;

	    opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //raw input
	    opts.c_oflag &= ~OPOST; // raw output

	    tcsetattr(mUartID, TCSANOW, &opts);

		mIsOpen = run("uart");
		if (!mIsOpen) {
			close(mUartID);
			mUartID = 0;
		}

		LOGD("openUart mIsOpen = %d\n", mIsOpen);
	}

	return mIsOpen;
}

void UartContext::closeUart() {
	LOGD("closeUart mIsOpen: %d...\n", mIsOpen);
	if (mIsOpen) {
		requestExit();

		close(mUartID);
		mUartID = 0;
		mIsOpen = false;
	}
}

bool UartContext::send(const BYTE *pData, UINT len) {
	if (!mIsOpen) {
		return false;
	}

	if (write(mUartID, pData, len) != len) {	// fail
		LOGD("send Fail\n");
		return false;
	}

	// success
//	LOGD("send Success\n");

	return true;
}

UartContext* UartContext::getInstance() {
	static UartContext sUC;
	return &sUC;
}

bool UartContext::readyToRun() {
	if (mDataBufPtr == NULL) {
		mDataBufPtr = new BYTE[UART_DATA_BUF_LEN];
	}

	if (mDataBufPtr == NULL) {
		closeUart();
	}

	return (mDataBufPtr != NULL);
}

bool UartContext::threadLoop() {
	if (mIsOpen) {
		// 可能上一次解析后有残留数据，需要拼接起来
		int readNum = read(mUartID, mDataBufPtr + mDataBufLen, UART_DATA_BUF_LEN - mDataBufLen);

		if (readNum > 0) {
		    if (mHook) {
		        mHook(mDataBufPtr + mDataBufLen, readNum);
		    }
			const BYTE *pData = mDataBufPtr;
			mDataBufLen += readNum;

			int dataLen;	// 数据包长度
			int frameLen;	// 帧长度
			// FF FF cmd data data AA AA
			while (mDataBufLen >= 7) {
				while ((mDataBufLen >= 2) && ((pData[0] != 0xFF) || (pData[1] != 0xFF))) {
					pData++;
					mDataBufLen--;
					continue;
				}

				if (mDataBufLen < 7) {
					break;
				}
				frameLen = 7;

				for (int i = 0; i < frameLen; ++i) {
					LOGD("%x ", pData[i]);
				}

				if ((pData[5] == 0xAA) && (pData[6] == 0xAA)) {
					procParse(pData, frameLen);
				} else {
					LOGE("CheckSum error!!!!!!\n");
				}

				pData += frameLen;
				mDataBufLen -= frameLen;
			}

			// 数据未解析完
			if ((mDataBufLen > 0) && (mDataBufPtr != pData)) {
				LOGD("mDataBufLen: %d\n", mDataBufLen);

				// 将数据移到头部，方便下一包数据拼接
				memcpy(mDataBufPtr, pData, mDataBufLen);
			}
		} else {
			Thread::sleep(50);
		}

		return true;
	}

	return false;
}

void UartContext::addReadHook(void (*hook)(const unsigned char*, int)) {
    mHook = hook;
}

void UartContext::removeReadHook() {
    mHook = NULL;
}
