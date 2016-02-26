// GetEasyServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Parameter.h"
#include "Wlan.h"
#include <iostream>

using namespace std;


int _tmain(int argc, _TCHAR* argv[])
{
	//获取本地网卡IP地址
	CParameter *para = new CParameter();
	para->GetIPList();

	//开启一个WiFi热点
	CWlan *wlan = new CWlan();
	//string name, passwd;
	//cin >> name >> passwd;
	wlan->SetSSID("hhcncx");
	wlan->SetKEY("mimadan1203");
	wlan->StartHostedNetWork();

	return 0;
}

