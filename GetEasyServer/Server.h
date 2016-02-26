#pragma once
//ADD

#include "afxinet.h"
#include "string.h"
#include "stdlib.h"

/*********************/
/*  extern data      */
/*********************/
#define WSA_RECV         0		//Socket状态
#define WSA_SEND         1
#define DATA_BUFSIZE    8192	//服务器能接收的 请求数据 的最大长度
#define MAX_NAME_LEN    128
#define MAX_PWD_LEN     128
#define MAX_RESP_LEN    1024
#define MAX_REQ_LEN     256
#define MAX_ADDR_LEN    80

#define FTP_PORT        21     // FTP 控制端口
#define DATA_FTP_PORT   20     // FTP 数据端口

#define OPENING_AMODE   150
#define CMD_OK          200
#define OS_TYPE         215
#define FTP_QUIT        221		//注销
#define TRANS_COMPLETE  226
#define PASSIVE_MODE    227
#define LOGGED_IN       230		//登录成功
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
typedef struct {					//记录socket信息
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

typedef struct {					//记录文件信息
	CHAR		szFileName[MAX_PATH];
	DWORD		dwFileAttributes; 
	FILETIME	ftCreationTime; 
	FILETIME	ftLastAccessTime; 
	FILETIME	ftLastWriteTime; 
	DWORD		nFileSizeHigh; 
	DWORD		nFileSizeLow; 
} FILE_INF, *LPFILE_INF;

struct CAccount{			//包含一个用户的基本信息
	CString username;
	CString password;
	CString directory;
};	

typedef CArray< CAccount,CAccount& > AccountArray;

/*********************/
/*  global function  */
/*********************/
//一个全局线程函数
extern UINT ServerThread(LPVOID lpParameter);
//ADD

class CServer
{
public:
	CServer(void);
	~CServer(void);
//ADD
public:
	AccountArray	m_RegisteredAccount;			//记录服务器注册用户
	BYTE            IpFild[4];                      //服务IP
	UINT			m_ServerPort;					//线程监听端口
	BOOL			m_bStopServer;					//记录服务器开启关闭信息
	SOCKET		    sListen;						//监听socket
	SOCKET		    sDialog;						//会话socket

public:
	void DivideRequest(char* request,char* command,char* cmdtail);	//分离请求信息
	void ServerConfigInfo( AccountArray* accountArray,BYTE nFild[],UINT port);	//服务器配置信息	
	BOOL SendWelcomeMsg( SOCKET s );								//当客户端连接服务器时，发送欢迎信息
	int Login( LPSOCKET_INF pSockInfo );							//客户端登录函数
	int DealWithCommand( LPSOCKET_INF socketInfo );					//命令处理函数
	int SendACK( LPSOCKET_INF socketInfo );							//发送相应消息到客户端
	int RecvRequest( LPSOCKET_INF socketInfo );						//接受请求函数
	//分隔的IP地址形式分解为一个DWORD形IP(32位Internet主机地址)和一个WORD（16位TCP端口地址）形端口分别由指针pdwIpAddr和pwPort返回
	int ConvertCommaAddrToDotAddr( char* commaAddr, LPDWORD pdwIpAddr, LPWORD pwPort );
	//将一个32位Internet主机地址和一个16位TCP端口地址转换为一个由六个被逗号隔开的数字组成的IP地址序列
	int ConvertDotAddrToCommaAddr( char* dotAddr, WORD wPort ,char* commaAddr );
	int DataConnect( SOCKET& s, DWORD dwIp, WORD wPort, int nMode );//数据连接函数
	char* GetLocalAddress();										//获取本地IP地址
	SOCKET AcceptConnectRequest( SOCKET& s );						//接受连接请求
	int FileListToString( char* buffer, UINT nBufferSize, BOOL isListCommand );//把文件列表信息转换成字符串，保存到buffer中
	int GetFileList( LPFILE_INF fileInfo, int arraySize, const char* searchPath );//获取文件列表
	int DataSend( SOCKET s, char* buff, int buffSize );				//数据发送
	int CombindFileNameSize(const char* filename,char* filnamesize);//把文件名及文件大小信息整合到一块
	int ReadFileToBuffer(const char* szFileName,char* buff,int fileSize );//把指定文件写入缓存
	int DataRecv( SOCKET s, const char* fileName );//数据接收，写入文件
	void GetRalativeDirectory(char *currDir,char*rootDir,char* ralaDir);//获得相对路径
	void GetAbsoluteDirectory(char* dir,char* currentDir,char* userCurrDir);//获得绝对路径
	void HostToNet(char* path);										//将\..\..格式转换为/../..格式
	void NetToHost(char* path);										//将/../..格式转换为\..\..格式
	int TryDeleteFile(char* deletedPath);							//删除文件
	BOOL IsPathExist(char* path);									//检测路径是否合法
//ADD
};
