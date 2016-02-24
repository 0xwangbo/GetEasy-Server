#pragma once

//
//	该类封装一系列应用启动所需的准备方法
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

	void Init();


public:
	LNode *ip_addr_list;

	CParameter();
	~CParameter();
	void GetIPList();
};

