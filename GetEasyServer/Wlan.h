#pragma once
#include <windows.h>
#include <wlanapi.h>
#include <string>

using namespace std;

#pragma comment(lib,"wlanapi")

class CWlan
{
//	DECLARE_DYNAMIC(CWlan)
//
//protected:
//	DECLARE_MESSAGE_MAP()

public:
	CWlan();
	~CWlan();

private:
	bool allow;
	bool start;
	HANDLE hClient;

public:
	int Init(void);
	int AllowHostedNetWork(void);
	int DisallowHostedNetWork(void);

	int StartHostedNetWork(void);
	int StopHostedNetWork(void);

	bool isStart(void);
	bool isAllow(void);

	int Resume(void);
	int getpeernumber(void);

	int SetKEY(string key);
	int SetSSID(string ssidname);
};

