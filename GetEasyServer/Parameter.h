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

private:
	void Init();


public:
	LNode *ip_addr_list;

public:
	CParameter();
	~CParameter();
	void GetIPList();//����һ�������洢���Ǳ�������������IP��ַ
};

