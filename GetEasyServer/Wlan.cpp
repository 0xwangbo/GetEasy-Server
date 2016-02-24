#include "stdafx.h"

#include "Wlan.h"
#include <wlanapi.h>

#pragma comment(lib,"wlanapi")


//IMPLEMENT_DYNAMIC(CWlan, CWnd)


CWlan::CWlan() : allow(false), start(false)//这里对两个布尔变量付初值
{
	Init();
}

CWlan::~CWlan()
{
	StopHostedNetWork();
	WlanCloseHandle(hClient, NULL);
}


//BEGIN_MESSAGE_MAP(CWlan, CWnd)
//END_MESSAGE_MAP()


int CWlan::Init(void)
{
	DWORD dwCurVersion = 0;
	DWORD dwResult = 0;

	dwResult = WlanOpenHandle(WLAN_API_VERSION, NULL, &dwCurVersion, &hClient);
	if (ERROR_SUCCESS != dwResult)
	{
		return -1;
	}

	BOOL bIsAllow = true;
	WLAN_HOSTED_NETWORK_REASON dwFailedReason;
	dwResult = WlanHostedNetworkSetProperty(hClient, wlan_hosted_network_opcode_enable, sizeof(BOOL), &bIsAllow, &dwFailedReason, NULL);
	if (ERROR_SUCCESS != dwResult)
	{
		return -2;
	}
	return 0;
}


int CWlan::AllowHostedNetWork(void)
{
	PWLAN_HOSTED_NETWORK_REASON pFailReason = NULL;
	DWORD dwResult = 0;
	dwResult = WlanHostedNetworkForceStart(hClient, pFailReason, NULL);
	if (dwResult != ERROR_SUCCESS)
	{
		return -1;
	}
	allow = true;
	return 0;
}


int CWlan::DisallowHostedNetWork(void)
{
	PWLAN_HOSTED_NETWORK_REASON pFailReason = NULL;
	DWORD dwResult = 0;
	dwResult = WlanHostedNetworkForceStop(hClient, pFailReason, NULL);
	if (dwResult != ERROR_SUCCESS)
	{
		return -1;
	}
	allow = false;
	return 0;
}


int CWlan::StartHostedNetWork(void)
{
	PWLAN_HOSTED_NETWORK_REASON pFailReason = NULL;
	DWORD dwResult = 0;
	dwResult = WlanHostedNetworkStartUsing(hClient, pFailReason, NULL);
	if (dwResult != ERROR_SUCCESS)
	{
		return -1;
	}
	start = true;
	return 0;
}


int CWlan::StopHostedNetWork(void)
{
	PWLAN_HOSTED_NETWORK_REASON pFailReason = NULL;
	DWORD dwResult = 0;
	dwResult = WlanHostedNetworkStopUsing(hClient, pFailReason, NULL);
	if (dwResult != ERROR_SUCCESS)
	{
		return -1;
	}
	start = false;
	return 0;
}



bool CWlan::isStart(void)
{
	return start;
}


bool CWlan::isAllow(void)
{
	return allow;
}


int CWlan::Resume(void)
{
	DWORD dwResult = 0;

	BOOL bIsAllow = false;
	WLAN_HOSTED_NETWORK_REASON dwFailedReason;
	dwResult = WlanHostedNetworkSetProperty(hClient, wlan_hosted_network_opcode_enable, sizeof(BOOL), &bIsAllow, &dwFailedReason, NULL);
	if (ERROR_SUCCESS != dwResult)
	{
		return -2;
	}
	return 0;
}


int CWlan::getpeernumber(void)
{
	PWLAN_HOSTED_NETWORK_STATUS ppWlanHostedNetworkStatus = NULL;
	int retval = WlanHostedNetworkQueryStatus(hClient, &ppWlanHostedNetworkStatus, NULL);
	if (retval != ERROR_SUCCESS){
		return -1;
	}
	return ppWlanHostedNetworkStatus->dwNumberOfPeers;
}


int CWlan::SetKEY(string key)
{
	char chkey[64];
	int index;
	for (index = 0; index < key.length(); index++)
	{
		chkey[index] = key[index];
	}
	chkey[index] = '\0';


	WLAN_HOSTED_NETWORK_REASON dwFailedReason;

	DWORD dwResult = 0;
	dwResult = WlanHostedNetworkSetSecondaryKey(hClient, strlen((const char*)chkey) + 1, (PUCHAR)chkey, TRUE, FALSE, &dwFailedReason, NULL);
	if (ERROR_SUCCESS != dwResult)
	{
		return -1;
	}

	return 0;
}


int CWlan::SetSSID(string ssidname)
{

	char chname[64];
	int index;
	for (index = 0; index < ssidname.length(); index++)
	{
	chname[index] = ssidname[index];
	}
	chname[index] = '\0';

	DWORD dwResult = 0;
	WLAN_HOSTED_NETWORK_REASON dwFailedReason;
	WLAN_HOSTED_NETWORK_CONNECTION_SETTINGS cfg;
	memset(&cfg, 0, sizeof(WLAN_HOSTED_NETWORK_CONNECTION_SETTINGS));

	memcpy(cfg.hostedNetworkSSID.ucSSID, chname, strlen(chname));
	cfg.hostedNetworkSSID.uSSIDLength = strlen((const char*)cfg.hostedNetworkSSID.ucSSID);
	cfg.dwMaxNumberOfPeers = 20;		//最大连接数    

	dwResult = WlanHostedNetworkSetProperty(hClient,wlan_hosted_network_opcode_connection_settings,sizeof(WLAN_HOSTED_NETWORK_CONNECTION_SETTINGS),&cfg,&dwFailedReason,NULL);
	if (ERROR_SUCCESS != dwResult)
	{
		return -1;
	}

	return 0;
}
