// GetEasyServer.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "Parameter.h"
#include "Wlan.h"
#include <iostream>

using namespace std;


int _tmain(int argc, _TCHAR* argv[])
{
	//��ȡ��������IP��ַ
	CParameter *para = new CParameter();
	para->GetIPList();

	//����һ��WiFi�ȵ�
	CWlan *wlan = new CWlan();
	//string name, passwd;
	//cin >> name >> passwd;
	wlan->SetSSID("hhcncx");
	wlan->SetKEY("mimadan1203");
	wlan->StartHostedNetWork();

	return 0;
}

