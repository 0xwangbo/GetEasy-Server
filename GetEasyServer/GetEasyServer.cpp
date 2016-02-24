// GetEasyServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Parameter.h"
#include "Wlan.h"
#include <iostream>

using namespace std;


int _tmain(int argc, _TCHAR* argv[])
{
	CParameter *para = new CParameter();
	para->GetIPList();

	CWlan *wlan = new CWlan();
	string name, passwd;
	cin >> name >> passwd;
	wlan->SetSSID(name);
	wlan->SetKEY(passwd);
	wlan->StartHostedNetWork();

	return 0;
}

