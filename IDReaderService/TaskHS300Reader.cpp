#include "pch.h"
#include "TaskHS300Reader.h"
#include <sstream>


#pragma comment(lib,"crypt32")


TaskHS300Reader::TaskHS300Reader()
{
}


TaskHS300Reader::~TaskHS300Reader()
{
}

/// <summary>
/// ���ض����豸��̬�⼰���ú���
/// </summary>
/// <param name=""></param>
int TaskHS300Reader::LoadDLL(std::string & sError)
{
	hDll = LoadLibrary(L"HS_Reader.dll");
	if (hDll == NULL)
	{
		int iCode = GetLastError();
		sError = "0|Load HS300 Dll Error Code: ";
		std::stringstream sTemp;
		sTemp << iCode;
		sError.append(sTemp.str());
		return -1;
	}
	else
	{
		hsInitScan = (hs300_InitScan)::GetProcAddress(hDll, "hs_InitScan");
		hsUninitScan = (hs300_UninitScan)::GetProcAddress(hDll, "hs_UninitScan");
		if (hsInitScan == NULL || hsUninitScan == NULL)
		{
			sError = "0|Load HS300 Dll Function Faild";
			return -1;
		}
	}
	return 0;
}

/// <summary>
/// �򿪶����豸�����Ͷ�������
/// �ȴ���Ϣ�ص�
/// </summary>
/// <param name=""></param>
int TaskHS300Reader::InitScan(CALL_BACK_MSG pfnCallBackMsg, std::string &sError){
	if (LoadDLL(sError) != 0)
		return -1;

	if(hsInitScan){
		return hsInitScan(pfnCallBackMsg, 0);
	}
	return -1;
}

/// <summary>
/// �ر��豸
/// </summary>
/// <param name=""></param>
int TaskHS300Reader::UnInitScan(){
	if(hsUninitScan != NULL){
		hsUninitScan();
		//::FreeLibrary(hDll);
		return 0;
	}
	return -1;
}