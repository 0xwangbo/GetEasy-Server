#pragma once

//
//	�����װһϵ��Ӧ�����������׼������
//	

class CParameter
{
private:
	union Data
	{
		int ip_count;//ip��ַ����
		char *ip_addr;//ip��ַ
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

