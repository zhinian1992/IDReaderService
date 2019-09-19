#pragma once
#include <string>

#include <windows.h>

//������������

/*
    hs_InitScan ��ȡ���ݵĻص��ӿ�
        type        data ����
        data        ���ص����ݿռ�
        length      ���ݳ���
        userdata    �û��ռ����
*/
#define DATA_DECODE     1  //��������
#define DATA_IDCARD     2  //���֤��Ϣ
#define DATA_SICARD     3  //�籣����Ϣ
#define DATA_OFF_LINE   10 //���豸����

typedef void (_stdcall *CALL_BACK_MSG)(int type, unsigned char *data,unsigned int length, unsigned long userdata);
/**
* @brief  ɨ���豸��ʼ��
* @par    ˵����
*         �ӿڿ��ڲ��Զ�����豸�������ݶ�ȡ����ͨ���ص����ض�ȡ����
* @param[in] pfnCallBackMsg     ��������ص�����
* @param[in] userdata           �û��ռ����ݣ��ڻص�ʱ����
* @return <0 ��ʾʧ�ܣ�=0 Ϊ�ɹ�
*/
typedef int(_stdcall *hs300_InitScan)(CALL_BACK_MSG pfnCallBackMsg,unsigned long userdata);
/**
* @brief  �Ͽ����ӣ��ӿڿ���Դ�ͷ�
* @par    ˵����
*         
*
* @return <0 ��ʾʧ�ܣ�=0 Ϊ�ɹ�
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