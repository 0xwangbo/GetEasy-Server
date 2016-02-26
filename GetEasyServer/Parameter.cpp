#include "stdafx.h"
#include "Parameter.h"
#include<winsock2.h>
#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>

//#pragma warning(disable:4996)

using namespace std;;

#pragma comment(lib,"ws2_32.lib") // ¾²Ì¬¿â 

CParameter::CParameter()
{
	Init();
}

CParameter::~CParameter()
{
}

void CParameter::Init()
{
	ip_addr_list = (LNode *)malloc(sizeof(LNode));
}

void CParameter::GetIPList()
{
	WSADATA wsaData;
	// ¼ÓÔØÌ×½Ó×Ö¿â  
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "Winsock load failed: " << GetLastError() << endl;
	}
	hostent *phostinfo = gethostbyname("");

	LNode *p1, *p2;
	p2 = ip_addr_list;
	for (int num = 0; phostinfo != NULL && phostinfo->h_addr_list[num] != NULL ; num++)
	{
		char *pszAddr = inet_ntoa( *(struct in_addr *)phostinfo->h_addr_list[num]);
		cout << pszAddr << endl;
		ip_addr_list->data.ip_count = num+1;
		p1 = (LNode *)malloc(sizeof(LNode));
		p1->data.ip_addr = pszAddr;
		p2->next = p1;
		p2 = p1;
	}
	p2->next = NULL;
	WSACleanup();
}

