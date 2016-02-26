#pragma once

//
//	该类封装一系列应用启动所需的准备工作
//	

class CParameter
{
private:
	union Data
	{
		int ip_count;//ip地址个数
		char *ip_addr;//ip地址
	}data;

	typedef struct _LNode
	{
		Data data;
		struct _LNode *next;
	}LNode;

private:
	void Init();


public:
	LNode *ip_addr_list;

public:
	CParameter();
	~CParameter();
	void GetIPList();//返回一个链表，存储的是本地所有网卡的IP地址
};

