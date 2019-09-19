// IDReaderService.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "mongoose.h"
#include "TaskHS300Reader.h"
#include <thread>
#include <sstream>
#include <mutex>

#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

HANDLE m_hMutex = NULL;//保证读卡线程唯一锁
HANDLE g_hSemaphore = NULL;//读卡回调信号
bool bReading = false;//判断是否持续读卡

SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE hStatus;
static const char *s_http_port = "9000";
static struct mg_serve_http_opts s_http_server_opts;
mg_connection * m_nc;

void			runWebSocketServer();
void			HS300ThreadRead();
void __stdcall  ProcMsg(int type, unsigned char *data, unsigned int length, unsigned long userdata);
void			ev_handler(mg_connection * nc, int ev, void * p);
void			sendMessage(std::string sInfo);
int				writeToLog(const char* str);
std::string		GBK2UTF8(const std::string& strGBK);

/// <summary>
/// 入口函数
/// 创建线程锁 用于控制读卡线程唯一
/// 读卡信号 用于读卡线程等待回调结束
/// 开启websocket监听
/// </summary>
/// <param name=""></param>
int main()
{
	m_hMutex = CreateMutex(NULL, false, NULL);
	g_hSemaphore = CreateSemaphore(NULL,0,1,NULL);

	while (true)
	{
		runWebSocketServer();
	}
}

/// <summary>
/// 启动websocket监听
/// 监听端口号9000
/// </summary>
/// <param name=""></param>
void runWebSocketServer()
{
	struct mg_mgr mgr;
	mg_mgr_init(&mgr, NULL);
	m_nc = mg_bind(&mgr, s_http_port, ev_handler);
	if (m_nc == NULL) {
		int i = GetLastError();
		writeToLog("开启服务端监听线程失败错误码为:");
		writeToLog(std::to_string(i).c_str());
		return;
	}

	// Set up HTTP server parameters
	mg_set_protocol_http_websocket(m_nc);
	s_http_server_opts.document_root = ".";  // Serve current directory
	s_http_server_opts.enable_directory_listing = "yes";

	writeToLog("开启服务端监听线程");
	for (;;) {
		mg_mgr_poll(&mgr, 1000);
	}
	mg_mgr_free(&mgr);
}

/// <summary>
/// websocket服务消息回调方法
/// 开启读卡线程
/// </summary>
/// <param name=""></param>
void ev_handler(mg_connection * nc, int ev, void * p) {
	m_nc = nc;

	if (ev == MG_EV_WEBSOCKET_FRAME)
	{
		struct websocket_message *hm = (struct websocket_message *) p;
		std::string sReq = (char *)hm->data;
		sReq = sReq.substr(0, hm->size);
		sendMessage("message on_message.....");

		if (sReq == "Read Base Msg")
		{
			std::thread t(HS300ThreadRead);
			t.detach();

		}
		else if (sReq == "Stop Read Msg") {
			bReading = false;
		}
	}
}

/// <summary>
/// 读卡线程
/// 申请锁,已存在未结束读卡线程则申请失败,忽略当前读卡请求,由未结束线程返回读卡信息
/// 线程结束释放锁
/// </summary>
/// <param name=""></param>
void HS300ThreadRead()
{
	DWORD dRes = -1;
	dRes = WaitForSingleObject(m_hMutex, 0);
	if (WAIT_OBJECT_0 != dRes)
	{
		writeToLog("waiting for reader callback");
		return;
	}

	bReading = true;
	writeToLog("开启HS300读卡线程");
	std::string sInfo, sError;
	TaskHS300Reader reader;

	while (bReading)
	{
		int iRet = reader.InitScan(ProcMsg, sError);

		if (iRet != 0)
		{
			writeToLog("开启HS300失败");
			sendMessage(sError);
			return;
		}
		writeToLog("开启HS300成功");

		//等待回调函数在主进程里释放信号
		WaitForSingleObject(g_hSemaphore, INFINITE);

		writeToLog("*******UnInitScan开始关闭...");
		if (reader.UnInitScan() == 0)
		{
			writeToLog("UnInitScan成功关闭");
		}
		else {
			writeToLog("调用UnInitScan接口失败");
		}
		ReleaseMutex(m_hMutex);
	}
	
	return;
}

/// <summary>
/// 读卡回调函数 
/// 处理信息后发送message到客户端
/// </summary>
/// <param name=""></param>
void __stdcall ProcMsg(int type, unsigned char *data, unsigned int length, unsigned long userdata)
{
	writeToLog("回调函数ProcMsg启动");
	writeToLog((char*)data);

	std::string sError = "";
	switch (type) {
	case DATA_DECODE:
		writeToLog("条码:");
		writeToLog((char*)data);
		sError.append("3|");
		sError.append((char*)data);
		break;
	case DATA_IDCARD:
		writeToLog("身份证:");
		writeToLog((char*)data);
		sError.append("1|");
		sError.append(GBK2UTF8((char*)data));
		break;
	case DATA_OFF_LINE:
		writeToLog("无设备连接:");
		sError.append("9| HS300 NOT CONNECTION");
		writeToLog("*******isHS300WaitCallBackEnd: false");
		//读卡故障停止读卡
		bReading = false;
		break;
	case DATA_SICARD:
		writeToLog("社保卡");
		sError.append("5|");
		sError.append(GBK2UTF8((char*)data));
		break;
	default:
		break;
	}
	sendMessage(sError);

	ReleaseSemaphore(g_hSemaphore, 1, NULL);
}

/// <summary>
/// 消息发送函数
/// </summary>
/// <param name=""></param>
void sendMessage(std::string sInfo)
{
	struct mg_connection *c;
	const struct mg_str msg = mg_mk_str(sInfo.c_str());
	char buf[500];
	char addr[32];
	mg_sock_addr_to_str(&m_nc->sa, addr, sizeof(addr),
		MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);

	snprintf(buf, sizeof(buf), "%s %.*s", addr, (int)msg.len, msg.p);
	for (c = mg_next(m_nc->mgr, NULL); c != NULL; c = mg_next(m_nc->mgr, c)) {
		mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, buf, strlen(buf));
	}
}

/// <summary>
/// 日志记录函数
/// </summary>
/// <param name=""></param>
int writeToLog(const char * str)
{
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	std::stringstream sTemp;
	sTemp << sys.wYear;
	sTemp << "年";
	sTemp << sys.wMonth;
	sTemp << "月";
	sTemp << sys.wDay;
	sTemp << "日";
	sTemp << sys.wHour;
	sTemp << ":";
	sTemp << sys.wMinute;
	sTemp << ":";
	sTemp << sys.wSecond;
	sTemp << "=====>";
	sTemp << str;

	char cLog[256];
	sTemp >> cLog;

	FILE* log;
	log = fopen(LOGFILE, "a+");
	if (log == NULL)
		return -1;
	fprintf(log, "%s\n", cLog);
	fclose(log);
	return 0;
}

/// <summary>
/// GBK编码数据转UTF8格式
/// </summary>
/// <param name=""></param>
std::string GBK2UTF8(const std::string& strGBK)
{
	std::string strOutUTF8 = "";
	WCHAR * str1;
	int n = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, NULL, 0);
	str1 = new WCHAR[n];
	MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, str1, n);
	n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
	char * str2 = new char[n];
	WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
	strOutUTF8 = str2;
	delete[]str1;
	str1 = NULL;
	delete[]str2;
	str2 = NULL;
	return strOutUTF8;
}