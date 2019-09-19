#pragma once
#include <string>

#include <windows.h>

//读卡器方法集

/*
    hs_InitScan 读取数据的回调接口
        type        data 类型
        data        返回的数据空间
        length      数据长度
        userdata    用户空间参数
*/
#define DATA_DECODE     1  //条码数据
#define DATA_IDCARD     2  //身份证信息
#define DATA_SICARD     3  //社保卡信息
#define DATA_OFF_LINE   10 //无设备连接

typedef void (_stdcall *CALL_BACK_MSG)(int type, unsigned char *data,unsigned int length, unsigned long userdata);
/**
* @brief  扫描设备初始化
* @par    说明：
*         接口库内部自动完成设备连接数据读取，并通过回调返回读取数据
* @param[in] pfnCallBackMsg     数据输出回调函数
* @param[in] userdata           用户空间数据，在回调时返回
* @return <0 表示失败，=0 为成功
*/
typedef int(_stdcall *hs300_InitScan)(CALL_BACK_MSG pfnCallBackMsg,unsigned long userdata);
/**
* @brief  断开连接，接口库资源释放
* @par    说明：
*         
*
* @return <0 表示失败，=0 为成功
*/
typedef void(_stdcall *hs300_UninitScan)(void);



class TaskHS300Reader
{
public:
	TaskHS300Reader();
	~TaskHS300Reader();

	int InitScan(CALL_BACK_MSG pfnCallBackMsg, std::string &sError);
	int UnInitScan();
private:
	int LoadDLL(std::string &sError);

protected:
	HMODULE hDll;
	int iPort;

	hs300_InitScan hsInitScan;
	hs300_UninitScan hsUninitScan;
};