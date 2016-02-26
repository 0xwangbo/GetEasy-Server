#pragma once
//ADD

#include "afxinet.h"
#include "string.h"
#include "stdlib.h"

/*********************/
/*  extern data      */
/*********************/
#define WSA_RECV         0		//Socket״̬
#define WSA_SEND         1
#define DATA_BUFSIZE    8192	//�������ܽ��յ� �������� ����󳤶�
#define MAX_NAME_LEN    128
#define MAX_PWD_LEN     128
#define MAX_RESP_LEN    1024
#define MAX_REQ_LEN     256
#define MAX_ADDR_LEN    80

#define FTP_PORT        21     // FTP ���ƶ˿�
#define DATA_FTP_PORT   20     // FTP ���ݶ˿�

#define OPENING_AMODE   150
#define CMD_OK          200
#define OS_TYPE         215
#define FTP_QUIT        221		//ע��
#define TRANS_COMPLETE  226
#define PASSIVE_MODE    227
#define LOGGED_IN       230		//��¼�ɹ�
#define DIR_CHANGED     250
#define CURR_DIR        257
#define USER_OK         331
#define FILE_EXIST		350
#define ACCESS_DENY		450
#define NOT_IMPLEMENT	502
#define REPLY_MARKER    504
#define LOGIN_FAILED    530
#define CANNOT_FIND     550
#define MAX_FILE_NUM    1024
#define MODE_PORT       0
#define MODE_PASV       1
#define PORT_BIND		1821

/*********************/
/* strct definition  */
/*********************/
typedef struct {					//��¼socket��Ϣ
   CHAR				buffRecv[DATA_BUFSIZE];
   CHAR				buffSend[DATA_BUFSIZE];
   WSABUF			wsaBuf;
   SOCKET			socket;
   WSAOVERLAPPED	wsaOverLapped;
   DWORD			dwBytesSend;
   DWORD			dwBytesRecv;
   int				SocketStatus;
   BOOL				userLoggedIn;
   CHAR				userCurrentDir[MAX_PATH];
   CHAR				userRootDir[MAX_PATH];
} SOCKET_INF, *LPSOCKET_INF;

typedef struct {					//��¼�ļ���Ϣ
	CHAR		szFileName[MAX_PATH];
	DWORD		dwFileAttributes; 
	FILETIME	ftCreationTime; 
	FILETIME	ftLastAccessTime; 
	FILETIME	ftLastWriteTime; 
	DWORD		nFileSizeHigh; 
	DWORD		nFileSizeLow; 
} FILE_INF, *LPFILE_INF;

struct CAccount{			//����һ���û��Ļ�����Ϣ
	CString username;
	CString password;
	CString directory;
};	

typedef CArray< CAccount,CAccount& > AccountArray;

/*********************/
/*  global function  */
/*********************/
//һ��ȫ���̺߳���
extern UINT ServerThread(LPVOID lpParameter);
//ADD

class CServer
{
public:
	CServer(void);
	~CServer(void);
//ADD
public:
	AccountArray	m_RegisteredAccount;			//��¼������ע���û�
	BYTE            IpFild[4];                      //����IP
	UINT			m_ServerPort;					//�̼߳����˿�
	BOOL			m_bStopServer;					//��¼�����������ر���Ϣ
	SOCKET		    sListen;						//����socket
	SOCKET		    sDialog;						//�Ựsocket

public:
	void DivideRequest(char* request,char* command,char* cmdtail);	//����������Ϣ
	void ServerConfigInfo( AccountArray* accountArray,BYTE nFild[],UINT port);	//������������Ϣ	
	BOOL SendWelcomeMsg( SOCKET s );								//���ͻ������ӷ�����ʱ�����ͻ�ӭ��Ϣ
	int Login( LPSOCKET_INF pSockInfo );							//�ͻ��˵�¼����
	int DealWithCommand( LPSOCKET_INF socketInfo );					//�������
	int SendACK( LPSOCKET_INF socketInfo );							//������Ӧ��Ϣ���ͻ���
	int RecvRequest( LPSOCKET_INF socketInfo );						//����������
	//�ָ���IP��ַ��ʽ�ֽ�Ϊһ��DWORD��IP(32λInternet������ַ)��һ��WORD��16λTCP�˿ڵ�ַ���ζ˿ڷֱ���ָ��pdwIpAddr��pwPort����
	int ConvertCommaAddrToDotAddr( char* commaAddr, LPDWORD pdwIpAddr, LPWORD pwPort );
	//��һ��32λInternet������ַ��һ��16λTCP�˿ڵ�ַת��Ϊһ�������������Ÿ�����������ɵ�IP��ַ����
	int ConvertDotAddrToCommaAddr( char* dotAddr, WORD wPort ,char* commaAddr );
	int DataConnect( SOCKET& s, DWORD dwIp, WORD wPort, int nMode );//�������Ӻ���
	char* GetLocalAddress();										//��ȡ����IP��ַ
	SOCKET AcceptConnectRequest( SOCKET& s );						//������������
	int FileListToString( char* buffer, UINT nBufferSize, BOOL isListCommand );//���ļ��б���Ϣת�����ַ��������浽buffer��
	int GetFileList( LPFILE_INF fileInfo, int arraySize, const char* searchPath );//��ȡ�ļ��б�
	int DataSend( SOCKET s, char* buff, int buffSize );				//���ݷ���
	int CombindFileNameSize(const char* filename,char* filnamesize);//���ļ������ļ���С��Ϣ���ϵ�һ��
	int ReadFileToBuffer(const char* szFileName,char* buff,int fileSize );//��ָ���ļ�д�뻺��
	int DataRecv( SOCKET s, const char* fileName );//���ݽ��գ�д���ļ�
	void GetRalativeDirectory(char *currDir,char*rootDir,char* ralaDir);//������·��
	void GetAbsoluteDirectory(char* dir,char* currentDir,char* userCurrDir);//��þ���·��
	void HostToNet(char* path);										//��\..\..��ʽת��Ϊ/../..��ʽ
	void NetToHost(char* path);										//��/../..��ʽת��Ϊ\..\..��ʽ
	int TryDeleteFile(char* deletedPath);							//ɾ���ļ�
	BOOL IsPathExist(char* path);									//���·���Ƿ�Ϸ�
//ADD
};
