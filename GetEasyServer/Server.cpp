#include "StdAfx.h"
#include "Server.h"


//ADD
//#include "ftpSrver.h"

/*********************/
/*  global data      */
/*********************/
DWORD				g_dwTotalEventAmount = 0;//总事件数
DWORD				g_index;
WSAEVENT			g_EventArray[WSA_MAXIMUM_WAIT_EVENTS];//手动重置对象
LPSOCKET_INF		g_SocketArray[WSA_MAXIMUM_WAIT_EVENTS];//新SOCKET_INF结构
CRITICAL_SECTION	g_CriticalSection;	//临界区

//处理线程函数声明
UINT ProcessThreadIO( LPVOID lpParam ) ;
//ADD

CServer::CServer(void)
{
//ADD
	m_bStopServer = FALSE;
//ADD
}

CServer::~CServer(void)
{
}

//ADD
UINT ServerThread(LPVOID lpParameter)
{
	CServer *	server = (CServer*)lpParameter;//服务器
	SOCKADDR_IN addrInfo;			//设置监听地址、端口
	DWORD		dwFlags;			//无实际意义，只是函数使用过程中的空白参数
	DWORD		dwRecvBytes;		//无实际意义，只是函数使用过程中的空白参数
	char		errorMsg[128];		//错误提示消息
	SOCKADDR_IN ClientAddr;			//或取客户端地址信息
	int			ClientAddrLength;	//客户端地址信息Size
	LPCTSTR		ClientIP;			//客户端IP
	UINT		ClientPort;			//客户端Port

	//初始化临界区
   InitializeCriticalSection( &g_CriticalSection );

   // 创建监听socket ：sListen
   if ( ( server->sListen = WSASocket(AF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED) ) == INVALID_SOCKET ) 
   {
	   sprintf_s( errorMsg,"ERROR：Failed to get a socket %d.POS:ServerThread()\n", WSAGetLastError() );
	   ////AfxMessageBox( errorMsg,MB_OK,0 );
	   WSACleanup();
       return 0;
   }

   //配置IP及端口
   CString sIP;
   sIP.Format("%d.%d.%d.%d",server->IpFild[0],server->IpFild[1],server->IpFild[2],server->IpFild[3]);
   addrInfo.sin_family = AF_INET;
   addrInfo.sin_addr.S_un.S_addr = inet_addr(sIP);
   addrInfo.sin_port = htons(server->m_ServerPort);		//FTP_PORT);

   //绑定监听端口sListen，指定通信对象
   if ( bind(server->sListen, (PSOCKADDR)&addrInfo, sizeof(addrInfo) ) == SOCKET_ERROR )
   {
	   sprintf_s( errorMsg,"ERROR：bind() failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
	   //AfxMessageBox( errorMsg,MB_OK,0 );
       return 0;
   }

   //设置sListen到等待连接状态
   if( listen( server->sListen, SOMAXCONN ) )
   {
	   sprintf_s( errorMsg,"ERROR：listen() failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
	   //AfxMessageBox( errorMsg,MB_OK,0 );
       return 0;
   }

   //创建会话socket
   if( ( server->sDialog = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED) ) == INVALID_SOCKET ) 
   {
	   sprintf_s( errorMsg,"ERROR：Failed to get a dialog socket %d.POS:ServerThread()\n", WSAGetLastError() );
	   //AfxMessageBox( errorMsg,MB_OK,0 );
	   return 0;
   }

   //创建第一个手动重置对象
   if ( ( g_EventArray[0] = WSACreateEvent() ) == WSA_INVALID_EVENT )
   {
	   sprintf_s( errorMsg,"ERROR：WSACreateEvent() failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
	   //AfxMessageBox( errorMsg,MB_OK,0 );
	   return 0;
   }

   //现在开始，已经创建了一个事件
   g_dwTotalEventAmount = 1;

   //创建一个线程处理请求
   AfxBeginThread( ProcessThreadIO,(LPVOID)server );

   //创建循环，不停的等待用户的连接
   while( !server->m_bStopServer )
   {
	   //处理入站连接,接受一个连接请求
	   ClientAddrLength = sizeof(ClientAddr);
	   if ( (server->sDialog = accept(server->sListen,(sockaddr*)&ClientAddr, &ClientAddrLength)) == INVALID_SOCKET )
	   {
		   return 0;
	   }

	   //获取客户端地址端口信息
	   ClientIP		= inet_ntoa( ClientAddr.sin_addr );
	   ClientPort	= ClientAddr.sin_port;

      //回传欢迎消息
	  if( !server->SendWelcomeMsg( server->sDialog ) )
	  {
		  break;
	  }

	  //开始进入临界区，防止出错
      EnterCriticalSection( &g_CriticalSection );

      //创建一个新的SOCKET_INF结构处理接受的数据socket
      if( (g_SocketArray[g_dwTotalEventAmount] = (LPSOCKET_INF)GlobalAlloc(GPTR,sizeof(SOCKET_INF))) == NULL )
      {
		  sprintf_s( errorMsg,"ERROR：GlobalAlloc() failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
		  //AfxMessageBox( errorMsg,MB_OK,0 );
		  return 0;
      } 

      //初始化新的SOCKET_INF结构
	  char buffer[DATA_BUFSIZE];
	  ZeroMemory(buffer, DATA_BUFSIZE);

	  g_SocketArray[g_dwTotalEventAmount]->wsaBuf.len = DATA_BUFSIZE;						//初始化wsaBuff
	  g_SocketArray[g_dwTotalEventAmount]->wsaBuf.buf = buffer;

      g_SocketArray[g_dwTotalEventAmount]->socket = server->sDialog;						//初始化socket

      memset( &(g_SocketArray[g_dwTotalEventAmount]->wsaOverLapped),0,sizeof(OVERLAPPED) ); //初始化wsaOverLapped

	  memset( &(g_SocketArray[g_dwTotalEventAmount]->buffRecv), 0, DATA_BUFSIZE );			//初始化buffRecv
	  memset( &(g_SocketArray[g_dwTotalEventAmount]->buffSend), 0, DATA_BUFSIZE );			//初始化buffSend
      g_SocketArray[g_dwTotalEventAmount]->dwBytesSend = 0;
      g_SocketArray[g_dwTotalEventAmount]->dwBytesRecv = 0;

	  g_SocketArray[g_dwTotalEventAmount]->userLoggedIn = FALSE;							//用户登录状态

	  g_SocketArray[g_dwTotalEventAmount]->SocketStatus = WSA_RECV;							//初始化预设状态
   
     //创建事件
      if ( ( g_SocketArray[g_dwTotalEventAmount]->wsaOverLapped.hEvent = 
		     g_EventArray[g_dwTotalEventAmount] = WSACreateEvent() ) == WSA_INVALID_EVENT )
      {
		  sprintf_s( errorMsg,"WSACreateEvent() failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
		  //AfxMessageBox( errorMsg,MB_OK,0 );
		  return 0;
      }

      //发出接收请求
      dwFlags = 0;
      if( WSARecv( g_SocketArray[g_dwTotalEventAmount]->socket, 
					&(g_SocketArray[g_dwTotalEventAmount]->wsaBuf), 
					1,
					&dwRecvBytes,
					&dwFlags,
					&(g_SocketArray[g_dwTotalEventAmount]->wsaOverLapped),
					NULL) == SOCKET_ERROR )
      {
         if ( WSAGetLastError() != WSA_IO_PENDING )
         {
			 //关闭socket
			 closesocket( g_SocketArray[g_dwTotalEventAmount]->socket );

			 //关闭事件
			 WSACloseEvent( g_EventArray[g_dwTotalEventAmount] );

			 //弹出提示消息
			 sprintf_s( errorMsg,"ERROR：WSARecv() failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
			 //AfxMessageBox( errorMsg,MB_OK,0 );

			 return 0;
         }
      }

	  //更新g_EventArray[]事件总数目
      g_dwTotalEventAmount++;

	  //离开临界区
      LeaveCriticalSection( &g_CriticalSection );

	  //使第一个事件有信号，使工作者线程处理其他的事件
      if ( WSASetEvent( g_EventArray[0] ) == FALSE )
      {
		  sprintf_s( errorMsg,"ERROR：WSASetEvent failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
		  //AfxMessageBox( errorMsg,MB_OK,0 );
		  return 0;
      }
   }

   //关闭监听线程
   server->m_bStopServer = FALSE;

   //退出监听线程
   return 0;
}

//工作者线程处理函数
UINT ProcessThreadIO( LPVOID lpParameter )
{
   DWORD		dwFlags;
   LPSOCKET_INF pSocketInfo;					//单个LPSOCKET_INF结构
   DWORD		dwBytesTransferred;				//数据传输字节数
   DWORD		i;								//index
   CServer *	server = (CServer*)lpParameter;	//服务器
   char			errorMsg[128];					//错误提示消息

   //处理异步的WSASend, WSARecv等请求等
   while( TRUE )
   {
	   //调用重叠操作（WSARecv()、 WSARecvFrom()、WSASend()、WSASendTo() 或 WSAIoctl()），等待事件通知
      if ( (g_index = WSAWaitForMultipleEvents(g_dwTotalEventAmount, g_EventArray, FALSE, WSA_INFINITE, FALSE)) == WSA_WAIT_FAILED )
      {
		  sprintf_s( errorMsg, "Error WSAWaitForMultipleEvents() failed with error:%d\n",WSAGetLastError() );
		  //AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );
		  return 0;
      }
      
	  //对第一个事件，不做处理
      if ( (g_index - WSA_WAIT_EVENT_0) == 0 )
      {
		  //将第一个数据设置为无信号状态
          WSAResetEvent(g_EventArray[0]);
          continue;
      }

	  //取出有信号状态的事件对应的SOCKET_INF结构
      pSocketInfo = g_SocketArray[g_index - WSA_WAIT_EVENT_0];

	  //将此事件设为无信号状态
      WSAResetEvent(g_EventArray[g_index - WSA_WAIT_EVENT_0]);

	  //dwBytesTransferred == 0表示通信对方是否已经关闭连接
      if ( WSAGetOverlappedResult( pSocketInfo->socket, 
									&(pSocketInfo->wsaOverLapped), 
									&dwBytesTransferred,
									FALSE, 
									&dwFlags ) == FALSE || dwBytesTransferred == 0 )
      {
		  ////重叠操作失败
		  //关闭套接口
		 closesocket( pSocketInfo->socket );

		 //释放套接口资源
         GlobalFree( pSocketInfo );

		 //关闭事件
         WSACloseEvent( g_EventArray[g_index - WSA_WAIT_EVENT_0] );

         // Cleanup g_SocketArray and g_EventArray by removing the socket event handle
         // and socket information structure if they are not at the end of the
         // arrays.
		 //进入临界区
         EnterCriticalSection( &g_CriticalSection );

		 //数组移动，删除事件
         if ( (g_index - WSA_WAIT_EVENT_0) + 1 != g_dwTotalEventAmount )
		 {
            for ( i=(g_index - WSA_WAIT_EVENT_0); i < g_dwTotalEventAmount; i++ ) 
			{
				g_EventArray[i] = g_EventArray[i+1];
			    g_SocketArray[i] = g_SocketArray[i+1];
            }
		}

		 //事件总数减一
         g_dwTotalEventAmount--;

		 //离开临界区
         LeaveCriticalSection(&g_CriticalSection);

		 //事件不可用，接下来的代码不用执行
         continue;
      }

	  // 前面事件可用，而且已经有数据传递
	  if( pSocketInfo->SocketStatus == WSA_RECV )
	  {
		  //数据拷贝
		  memcpy( &pSocketInfo->buffRecv[pSocketInfo->dwBytesRecv],pSocketInfo->wsaBuf.buf,dwBytesTransferred );
		  
		  //接受字节数
		  pSocketInfo->dwBytesRecv = dwBytesTransferred;	//更改：原来是pSocketInfo->dwBytesRecv += dwBytesTransferred;

		  //检测接收到的数据
		  if( pSocketInfo->dwBytesRecv > 2 
			  && pSocketInfo->buffRecv[pSocketInfo->dwBytesRecv-2] == '\r'      // 要保证最后是\r\n
			  && pSocketInfo->buffRecv[pSocketInfo->dwBytesRecv-1] == '\n' )  
		  {
			  //检测用户是否已经登录
			  if( !pSocketInfo->userLoggedIn )//-----------------------未登录:用户登录
			 {
				 if( server->Login(pSocketInfo) == LOGGED_IN )
				 {
					 //更新用户登录状态
					 pSocketInfo->userLoggedIn = TRUE;
					 
					 //设置ftp根目录 \ Nonzero if successful; otherwise 0
					 if( SetCurrentDirectory( pSocketInfo->userCurrentDir )==0 )
					 {
						 sprintf_s( errorMsg,"ERROR：POS:ServerThread()\nSetCurrentDirectory() failed with error: %d.", GetLastError() );
						 //AfxMessageBox( errorMsg,MB_OK|MB_ICONERROR );
					 }
				 }
			 }
			 else//--------------------------------------------------已经登录：根据命令干事情
			 {
				 if( server->DealWithCommand( pSocketInfo )==FTP_QUIT )
				 {
					 break;
				 }
			 }

			 //缓冲区清除
			 memset( pSocketInfo->buffRecv,0,sizeof(pSocketInfo->buffRecv) );
			 pSocketInfo->dwBytesRecv = 0;
		  }
	  }
	  else//如果Socket处在发送状态
	  {
		  pSocketInfo->dwBytesSend += dwBytesTransferred;
	  }
	  
 	  // 继续接收以后到来的数据
	  if( server->RecvRequest( pSocketInfo ) == -1 )
	  {
		  return -1;
	  }
   }

   //结束线程
   return 0;
}


//服务器配置
void CServer::ServerConfigInfo( AccountArray* accountArray,BYTE nFild[],UINT port)
{
	//账户设置
	int size = (int)(*accountArray).GetCount();
	for( int i = 0 ; i < size ; i ++ )
	{
		m_RegisteredAccount.Add( (*accountArray)[i] );
	}

	//IP设置
	for( int i = 0;i < 4;i ++)
	{
		IpFild[i] = nFild[i];
	}

	//端口设置
	m_ServerPort = port;

	//设置服务器开启状态
	m_bStopServer = FALSE;
}

//发送欢迎消息
BOOL CServer::SendWelcomeMsg( SOCKET s )
{
	char* welcomeInfo = "Welcome to use GetEasy-Server...\nServer ready..\r\n";

	if( send( s, welcomeInfo, (int)strlen(welcomeInfo), 0 )==SOCKET_ERROR ){
		//AfxMessageBox(_T("Failed in sending welcome msg. POS:CServer::SendWelcomeMsg"));
		return FALSE;
	}
	return TRUE;
}

//用户登录函数
int CServer::Login( LPSOCKET_INF socketInfo )
{
	//静态变量
	static char username[MAX_NAME_LEN];
	static char password[MAX_PWD_LEN];

	//本地变量
	int loginResult = 0;

	//获取用户名
	if( strstr( strupr(socketInfo->buffRecv),"USER" )||strstr( strlwr(socketInfo->buffRecv),"user" ) )
	{
		sprintf_s( username, "%s", socketInfo->buffRecv+strlen("user")+1 );
		strtok( username, "\r\n" );
		//ACK
		sprintf_s( socketInfo->buffSend, "%s", "331 User name ok,need password.\r\n" );
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("Failed in sending ACK. POS:CServer::Login()") );
			return -1;
		}
		//successed in getting username
		return USER_OK;
	}
	//获取用户登录密码
	if( strstr( strupr(socketInfo->buffRecv),"PASS" )||strstr( strlwr(socketInfo->buffRecv),"pass") )
	{
		sprintf_s( password,"%s",socketInfo->buffRecv+strlen("pass")+1 );
		strtok( password, "\r\n" );
		//判断用户名密码正确性
		int size = (int)m_RegisteredAccount.GetCount();
		BOOL isAccountExisted = FALSE;
		CString user = (CString)username;
		////AfxMessageBox(user);
		CString pwd = (CString)password;
		for( int i=0 ; i<size ; i++ )
		{
			if( (m_RegisteredAccount[i].username.CompareNoCase(user)==0 && m_RegisteredAccount[i].password.CompareNoCase(pwd)==0)||
				(m_RegisteredAccount[i].username.CompareNoCase(user)==0 && user == "ANONYMOUS" ))
			{
				//找到符合的用户帐号
				isAccountExisted = TRUE;
				//获得当前用户预设的FTP根目录
				strcpy( socketInfo->userCurrentDir,m_RegisteredAccount[i].directory );
				strcpy( socketInfo->userRootDir,m_RegisteredAccount[i].directory );
				break;
			}
		}
		//账户存在与否相应的反馈信息
		if( isAccountExisted )
		{
			sprintf_s( socketInfo->buffSend,"230 User:%s logged in.proceed.\r\n",username );
			loginResult = LOGGED_IN;
		}
		else{
			sprintf_s( socketInfo->buffSend,"530 User:%s cannot logged in, Wrong username or password.Try again.\r\n",username );
			loginResult = LOGIN_FAILED;
		}
		//发送ACK
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("Failed in sending ACK. POS:CServer::Login()") );
			return -1;
		}
	}
	//user logged in successfully
	return loginResult;
}


//发送响应消息到客户端
int CServer::SendACK( LPSOCKET_INF socketInfo )
{
	//一个指向所发送数据字节数的变量
	static DWORD dwSendBytes = 0;
	char errorMsg[128];

	//设置socket结构状态为发送状态
	socketInfo->SocketStatus = WSA_SEND;

	//初始化wasOverLapped结构
	memset( &(socketInfo->wsaOverLapped), 0, sizeof(WSAOVERLAPPED) );
	socketInfo->wsaOverLapped.hEvent = g_EventArray[g_index - WSA_WAIT_EVENT_0];

	//初始化要发送的内容
    socketInfo->wsaBuf.buf = socketInfo->buffSend + socketInfo->dwBytesSend;
    socketInfo->wsaBuf.len = (int)strlen( socketInfo->buffSend ) - socketInfo->dwBytesSend;

	//发送ACK
	if ( WSASend(socketInfo->socket,&(socketInfo->wsaBuf),1,&dwSendBytes,0,&(socketInfo->wsaOverLapped),NULL) == SOCKET_ERROR ) 
	{
        if ( WSAGetLastError() != ERROR_IO_PENDING )
		{
			sprintf_s( errorMsg,"WSASend() failed with error :%d.POS:CServer::SendACK()\n", WSAGetLastError() );
			//AfxMessageBox( errorMsg );
			return -1;
        }
    }

	//成功返回
	return 0;
}

//接受客户端请求
int CServer::RecvRequest( LPSOCKET_INF socketInfo )
{
	//本地变量声明
	DWORD dwRecvBytes = 0;		//更改属性，原来是static DWORD dwRecvBytes = 0;
	DWORD dwFlags = 0;
	char errorMsg[128];

	//设置socket结构状态
	socketInfo->SocketStatus = WSA_RECV;
	
	//初始化wsaOverLapped结构
	memset( &(socketInfo->wsaOverLapped), 0,sizeof(WSAOVERLAPPED) );
	socketInfo->wsaOverLapped.hEvent = g_EventArray[g_index - WSA_WAIT_EVENT_0];

	//设置接收请求数据长度
	socketInfo->wsaBuf.len = DATA_BUFSIZE;

	//开始接受请求数据
	if ( WSARecv(socketInfo->socket,&(socketInfo->wsaBuf),1,&dwRecvBytes,&dwFlags,&(socketInfo->wsaOverLapped),NULL) == SOCKET_ERROR )
	{
		if ( WSAGetLastError() != ERROR_IO_PENDING )
		{
			sprintf_s( errorMsg,"WSARecv() failed with error: %d.POS:CServer::RecvRequest()\n", WSAGetLastError() );
			//AfxMessageBox( errorMsg,MB_OK,MB_ICONERROR );
		    return -1;
		}
	}

	//接收正常完成
	return 0;
}

//分离请求信息
void CServer::DivideRequest(char *request, char *command, char *cmdtail)
{
	int i = 0;
	int len = (int)strlen( request );
	command[0] = '\0';
	for( i=0 ; i<len && request[i]!=' ' && request[i]!='\r' && request[i]!='\n'; i++ )
	{
		command[i] = request[i];
	}
	command[i] = '\0';	

	//将命令全部转换为大写形式，便于处理
	strupr( command );

	//开始分离，有可能为空
	int index = 0;
	for( i++ ;  i<len && request[i]!='\r' && request[i]!='\n' ; i++ )
	{
		cmdtail[index] = request[i];
		index++;
	}
	cmdtail[index] = '\0';
}

//命令处理函数
int CServer::DealWithCommand( LPSOCKET_INF socketInfo )
{
	//variable difinition
	static SOCKET sDialog		= INVALID_SOCKET;
	static SOCKET sAccept		= INVALID_SOCKET;
	static BOOL   isPasvMode	= FALSE;				//判断是否被动传输模式
		
	int   operationResult		= 0;					//记录某操作过程结果
	char  command[20];									//记录命令
	char  cmdtail[MAX_REQ_LEN];							//记录命令后面的内容
	char  currentDir[MAX_PATH];							//记录当前FTP目录
	char  relativeDir[128];								//记录当前目录的相对目录

	const char*  szOpeningAMode = "150 Opening ASCII mode data connection for ";
	static DWORD  dwIpAddr		= 0;					//Client指定通信IP(四个'.'分隔的ip转换为相应的无符号长整型)
	static WORD   wPort			= 0;					//Client指定通信端口

	//解析请求，分离出命令存储到cammand \ 命令后面跟随的内容,存储到cmdtail
	DivideRequest( socketInfo->buffRecv,command,cmdtail );
	
	//检测是否为空
	if( command[0] ==  '\0' )
	{
		return -1;
	}

	//---------------------------//
	//------开始处理命令---------//
	//---------------------------//
	//为数据连接指定一个IP地址和本地端口,PORT—data port（改变默认数据端口）
	if( strstr(command,"PORT") )
	{
		// 0：成功//-1：失败
		//将','分隔的IP地址形式分解为一个DWORD形IP(32为Internet主机地址)0(dwIpAddr)和一个WORD（16为TCP端口地址）形端口(wPort)
		if( ConvertCommaAddrToDotAddr( socketInfo->buffRecv+strlen("PORT")+1,&dwIpAddr,&wPort) == -1 )
		{
			//AfxMessageBox( _T("Failed in ConvertCommaAddrToDotAddr().CMD:PORT") );
			return -1;
		}

		//发送消息响应客户端
		sprintf_s( socketInfo->buffSend,"%s","200 PORT Command successful.\r\n" );
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:PORT"),MB_OK|MB_ICONERROR );
			return -1;
		}

		//更新传输模式
		isPasvMode = FALSE;

		return CMD_OK;
	}

	//告诉服务器在一个非标准端口上收听数据连接
	//PASV—passive (将server_DTP设置为’被动’状态，使其在数据端口进行侦听数据连接，而不是启动一个数据连接)
	if( strstr( command,"PASV") )
	{
		if( DataConnect( sAccept, htonl(INADDR_ANY), PORT_BIND, MODE_PASV ) == -1 )  //0==成功，-1==失败
		{
			//AfxMessageBox( "DataConnect() failed.CMD:PASV",MB_OK|MB_ICONERROR );
			return -1;
		}

		//获取六个逗号隔开的数字组成的IP序列
		char szCommaAddress[40];
		if( ConvertDotAddrToCommaAddr( GetLocalAddress(),PORT_BIND,szCommaAddress )==-1 )
		{
			//AfxMessageBox( _T("Failed in ConvertCommaAddrToDotAddr().CMD:PASV") );
			return -1;
		}

		//设置ACK
		sprintf_s( socketInfo->buffSend,"227 Entering Passive Mode (%s).\r\n",szCommaAddress );

		//发送ACK
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:PASV"),MB_OK|MB_ICONERROR );
			return -1;
		}

		//更新传输模式
    	isPasvMode = TRUE;

		return PASSIVE_MODE;		
	}

	//让服务器给客户端发送一份目录列表;NLST—name list;LIST—list 
	if( strstr( command, "NLST") || strstr( command,"LIST") )
	{
		if( isPasvMode )
		{
			//接受连接请求，并新建会话socket
			sDialog = AcceptConnectRequest( sAccept );
		}

		if( !isPasvMode )	//主动传输
		{
			sprintf_s( socketInfo->buffSend,"%s/bin/ls.\r\n",szOpeningAMode );
		}
		else				//被动传输
		{
			strcpy( socketInfo->buffSend,"125 Data connection already open; Transfer starting.\r\n" );
		}
		
		//send ACK
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:NLST/LIST"),MB_OK|MB_ICONERROR );
			return -1;
		}

		// 取得文件列表信息，并转换成字符串
		BOOL isListCommand = strstr( command,"LIST" ) ? TRUE : FALSE;
		char buffer[DATA_BUFSIZE];

		UINT nStrLen = FileListToString( buffer,sizeof(buffer),isListCommand);

		//主动被动传输模式之间的差别
		if( !isPasvMode )				//主动传输模式
		{
			if( DataConnect( sAccept,dwIpAddr,wPort,MODE_PORT ) == -1 )
			{
				//AfxMessageBox( _T("DataConnect() failed.CMD:NLST/LIST"),MB_OK|MB_ICONERROR );
				return -1;
			}
			if( DataSend( sAccept, buffer,nStrLen ) == -1 )
			{
				//AfxMessageBox( _T("DataSend() failed.CMD:NLST/LIST"),MB_OK|MB_ICONERROR );
				return -1;
			}

			//关闭socket
			closesocket(sAccept);
		}
		else							//被动传输模式
		{
			DataSend( sDialog,buffer,nStrLen );
			closesocket( sDialog );
		}

		//send ACK
		sprintf_s( socketInfo->buffSend,"%s","226 Transfer complete.\r\n" );
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:NLST/LIST"),MB_OK|MB_ICONERROR );
			return -1;
		}

		return TRANS_COMPLETE;
	}

	//让服务器给客户端传送一份在路径名中指定的文件的副本(其实就是FTP下载功能实现)
	//RETR—retrieve：该命令使服务器_DTP传输路径名指定的文件副本到数据连接另一端的服务器或用户DTP
	if( strstr( command, "RETR") )
	{
		if( isPasvMode )
		{
			//接受连接请求，并新建会话socket
			sDialog = AcceptConnectRequest( sAccept );
		}

		char fileNameSize[MAX_PATH];
		int nFileSize = CombindFileNameSize( cmdtail,fileNameSize ); //返回获取文件大小，求文件名及大小信息保存在fileNameSize中

		//ACK
		sprintf_s( socketInfo->buffSend,"%s%s.\r\n",szOpeningAMode,fileNameSize );
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:RETR"),MB_OK|MB_ICONERROR );
			return -1;
		}

		char* buffer = new char[nFileSize];
		if( buffer == NULL )
		{
			//AfxMessageBox( _T("分配缓存失败!\n") );
			return -1;
		}

		////把指定文件写入缓存 / 返回总共读取字节数
		if( ReadFileToBuffer( cmdtail,buffer, nFileSize ) == nFileSize )
		{
			// 处理Data FTP连接
			Sleep( 10 );

			if( isPasvMode )
			{
				DataSend( sDialog,buffer,nFileSize );
				closesocket( sDialog );
			}
			else
			{
				//创建连接
				if( DataConnect( sAccept,dwIpAddr,wPort,MODE_PORT ) == -1 )
				{
					return -1;
				}

				//发送数据
				DataSend( sAccept, buffer, nFileSize );
				closesocket( sAccept );
			}
		}

		//清除缓存
		if( buffer != NULL )
		{
			delete[] buffer;
		}

		//ACK
		sprintf_s( socketInfo->buffSend,"%s","226 Transfer complete.\r\n" );
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:RETR"),MB_OK|MB_ICONERROR );
			return -1;
		}
				
		return TRANS_COMPLETE;
	}

	//STOR---store：该命令使服务器DTP接收经数据联接传输过来的数据，并将其作为文件存在服务器上(实际就是FTP上传功能)
	if( strstr( command, "STOR") )
	{
		//如果是被动模式，首先要接受连接，创建会话socket
		if( isPasvMode )
		{
			sDialog = AcceptConnectRequest( sAccept );
		}

		if( cmdtail[0]=='\0' )
		{
			return -1;
		}

		//ACK
		sprintf_s( socketInfo->buffSend,"%s%s.\r\n",szOpeningAMode,cmdtail );
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:STOR"),MB_OK|MB_ICONERROR );
			return -1;
		}

		// 处理Data FTP连接
		if( isPasvMode )
		{
			//接收数据
			DataRecv( sDialog,cmdtail );
		}
		else
		{
			if( DataConnect( sAccept,dwIpAddr,wPort,MODE_PORT ) == -1 )
			{
				return -1;
			}

			//接收数据
			DataRecv( sAccept, cmdtail );
		}
		
		//ACK
		sprintf_s( socketInfo->buffSend,"%s","226 Transfer complete.\r\n" );
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:STOR"),MB_OK|MB_ICONERROR );
			return -1;
		}
				
		return TRANS_COMPLETE;
	}

	//终止USER。如果没有正在进行文件传输，服务器将关闭控制连接；如果文件传输
	//正在进行，该连接仍然打开直至结束响应，然后服务器将其关闭;QUIT---logout
	if( strstr( command,"QUIT" ) )
	{
		//ACK
		sprintf_s( socketInfo->buffSend,"%s","221 Goodbye.\r\n" );
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:QUIT"),MB_OK|MB_ICONERROR );
			return -1;
		}
		
		return FTP_QUIT;
	}

	//PWD:在应答中返回当前工作目录的名称;PWD---print working directory
	if( strstr( command,"XPWD" ) || strstr( command,"PWD") ) 
	{
		//获取当前目录
		GetCurrentDirectory( MAX_PATH,currentDir );

		//获取当前目录的相对目录
		char tempStr[128];
		GetRalativeDirectory(currentDir,socketInfo->userRootDir,tempStr);
		HostToNet( tempStr );

		//发送ACK
		sprintf_s( socketInfo->buffSend,"257 \"%s\" is current directory.\r\n",tempStr );
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:XPWD/PWD"),MB_OK|MB_ICONERROR );
			return -1;
		}

		return CURR_DIR;
	}

	//CDUP---change to parent directory
	if( strstr(command,"CDUP") )
	{
		//检测是否根目录
		if( cmdtail[0]=='\0' )
		{
			strcpy(cmdtail,"\\");
		}

		//获得应该转变到的目录
		char szSetDir[MAX_PATH];
		strcpy( szSetDir,".." );

		//设置当前目录 \ nonzero is reasonable
		if( SetCurrentDirectory( szSetDir )==0 )
		{
			//set ACK
			sprintf_s( socketInfo->buffSend,"550 '%s' No such file or Directory.\r\n",szSetDir );

			//记录处理结果
			operationResult = CANNOT_FIND;
		} 
		else
		{
			//获得当前目录
			GetCurrentDirectory( MAX_PATH,currentDir );

			//获得相对路径
			relativeDir[0] = '\0';
			GetRalativeDirectory(currentDir,socketInfo->userRootDir,relativeDir);

			//将\..\..格式转换为/../..格式,以便响应客户端
			HostToNet( relativeDir );

			//set ACK
			sprintf_s( socketInfo->buffSend,"250 'CDUP' command successful,'/%s' is current directory.\r\n",relativeDir );

			//记录处理结果
			operationResult = DIR_CHANGED;
		}

		//send ACK
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD: CDUP"),MB_OK|MB_ICONERROR );
			return -1;
		}

		return operationResult;
	}

	//CWD:把当前目录改为远程文件系统的指定路径，而无需改变登录、帐号信息或传输参数
	//CWD---change working directory
	if( strstr( command,"CWD" ) )
	{
		//检测是否根目录
		if( cmdtail=='\0' )
		{
			strcpy( cmdtail,"\\" );
		}

		//获得应该转变到的目录
		GetAbsoluteDirectory( cmdtail,socketInfo->userCurrentDir,currentDir );

		//检测当前目录是否在根目录之上
		if( strlen(currentDir) < strlen(socketInfo->userRootDir) )
		{
			//设置响应消息
			sprintf_s( socketInfo->buffSend,"550 'CWD' command failed,'%s' User no power.\r\n",cmdtail );
		}		
		else if( SetCurrentDirectory( currentDir )==0 )  //设置当前目录 \ nonzero is reasonable
		{
			//设置响应消息
			sprintf_s( socketInfo->buffSend,"550 'CWD' command failed,'%s': No such file or Directory.\r\n",cmdtail );

			//记录处理结果
			operationResult = CANNOT_FIND;
		}
		else
		{
			//更改用户当前目录信息
			strcpy( socketInfo->userCurrentDir,currentDir );

			//获得相对路径
			relativeDir[0] = '\0';
			GetRalativeDirectory(currentDir,socketInfo->userRootDir,relativeDir);

			//将\..\..格式转换为/../..格式,以便响应客户端
			HostToNet( relativeDir );

			//设置响应消息
			sprintf_s( socketInfo->buffSend,"250 'CWD' command successful,'/%s' is current directory.\r\n",relativeDir );

			//记录处理结果
			operationResult = DIR_CHANGED;
		}

		//send ACK
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:CWD"),MB_OK|MB_ICONERROR );
			return -1;
		}

		return operationResult;
	}

	//删除文件 ;DELE---delete, XRMD child删除名为“child”的目录;RMD--Remove Directory
	if( strstr( command,"DELE") )
	{
		//获得应该转变到的目录
		GetAbsoluteDirectory( cmdtail,socketInfo->userCurrentDir,currentDir );

		//获得相对路径
		GetRalativeDirectory( currentDir,socketInfo->userRootDir,relativeDir );

		//将\..\..格式转换为/../..格式,以便响应客户端
		HostToNet( relativeDir );

		//尝试删除文件 或 文件夹
		BOOL isDELE = TRUE;
			operationResult = TryDeleteFile( currentDir );

		//根据删除结果，设置响应消息ACK
		if( operationResult==DIR_CHANGED )
		{
			sprintf_s( socketInfo->buffSend,"250 File '/%s' has been deleted.\r\n",relativeDir );
		}
		else if( operationResult==ACCESS_DENY )
		{
			sprintf_s( socketInfo->buffSend,"450 File '/%s' can't be deleted.\r\n",relativeDir );
		}

		//send ACK
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:DELE"),MB_OK|MB_ICONERROR );
			return -1;
		}
		return operationResult;
	}

	//确定数据的传输方式
	if( strstr( command,"TYPE") ) 
	{
		if( cmdtail[0] == '\0' )
		{
			strcpy( cmdtail,"A" );
		}

		//ACK
		sprintf_s(socketInfo->buffSend,"200 Type set to %s.\r\n",cmdtail );
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:TYPE"),MB_OK|MB_ICONERROR );
			return -1;
		}

		return CMD_OK;		
	}

	//Restart命令：标志文件内的数据点，将从这个点开始继续传送文件
	if( strstr( command,"REST" ) )
	{
		sprintf_s( socketInfo->buffSend,"504 Reply marker must be 0.\r\n");
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:REST"),MB_OK|MB_ICONERROR );
			return -1;
		}

		return REPLY_MARKER;
	}

	//其余都是无效的命令
	sprintf_s(socketInfo->buffSend,"500 '%s' Unknown command.\r\n",command );
	if( SendACK( socketInfo ) == -1 )
	{
		//AfxMessageBox( _T("SendACK() failed.CMD:otherwise"),MB_OK|MB_ICONERROR );
		return -1;
	}

	return operationResult;
}


//
int CServer::ConvertCommaAddrToDotAddr( char* commaAddr, LPDWORD pdwIpAddr, LPWORD pwPort )
{
	//variable definition
	int index	= 0;
	int i		= 0;
	int commaAmout	= 0;			//计数逗号个数
	char dotAddr[MAX_ADDR_LEN];		//用于提取四个逗号相隔的IP地址“a.b.c.d”
	char szPort[MAX_ADDR_LEN];		//用于提取ip地址后面的端口号

	//初始化为0
	memset( dotAddr,0,sizeof(dotAddr) );
	memset( szPort, 0, sizeof(szPort)   );
	*pdwIpAddr	= 0;
	*pwPort		= 0;

	//找出所有的','并改为'.'
	while( commaAddr[index] )
	{
		if( commaAddr[index] == ',' )
		{
			commaAmout++;
			commaAddr[index] = '.';
		}

		if( commaAmout < 4 )
		{
			dotAddr[index] = commaAddr[index];
		}
		else
		{
			szPort[i++] =   commaAddr[index];
		}

		index++;
	}

	//检测合法性
	if( commaAmout != 5 )
	{
		return -1;
	}

	//将得到的四个'.'分隔的ip转换为相应的无符号长整型表示
	(*pdwIpAddr) = inet_addr( dotAddr );
	if( (*pdwIpAddr)  == INADDR_NONE )
	{
		return -1;
	}

	char *temp = strtok( szPort+1,"." );	//前部分
	if( temp == NULL )
	{
		return -1;
	}
	(*pwPort) = (WORD)( atoi(temp) * 256 );
	
	temp = strtok(NULL,".");				//后部分
	if( temp == NULL )
	{
		return -1;
	}
	(*pwPort) += (WORD)atoi( temp );
		
	return 0;
}

//将一个32位Internet主机地址和一个16位TCP端口地址转换为一个由六个被逗号隔开的数字组成的IP地址序列
int CServer::ConvertDotAddrToCommaAddr(char *dotAddr, WORD wPort, char* commaAddr )
{
	//处理端口
	char szPort[10];
	sprintf_s( szPort,"%d,%d",wPort/256,wPort%256 );

	//处理标准Internet IP
	sprintf( commaAddr,"%s,",dotAddr );

	//将commaAddr中的'.'转换为','
	int index = 0;
	while( commaAddr[index] ) 
	{
		if( commaAddr[index] == '.' )
		{
			commaAddr[index] = ',';
		}

		index++;
	}

	//将IP、端口拼接起来
	sprintf( commaAddr,"%s%s",commaAddr,szPort );

	return 0;
}

//数据连接函数--设置连接状态：如果是主动模式，服务器connect主动到客户端；如果是被动模式，服务器开始监听
int CServer::DataConnect(SOCKET &s, DWORD dwIp, WORD wPort, int nMode)
{
	char errorMsg[128];
	//创建一个socket
	s = socket( AF_INET,SOCK_STREAM,0 );
	if( s == INVALID_SOCKET )
	{
		sprintf_s( errorMsg,"socket() failed to get a socket with error: %d.POS:CServer::DataConnect()\n", WSAGetLastError() );
		 //AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );
		 return -1;
	}

	//配置地址信息
	struct sockaddr_in addrInfo;

	//配置地址簇
	addrInfo.sin_family = AF_INET;

	//配置端口和IP
	if( nMode == MODE_PASV )
	{
		addrInfo.sin_port = htons( wPort );
		addrInfo.sin_addr.s_addr = dwIp;
	}
	else
	{
		addrInfo.sin_port = htons( DATA_FTP_PORT );					//DATA_FTP_PORT==20
		addrInfo.sin_addr.s_addr = inet_addr( GetLocalAddress() );  //调用函数获取本机地址
	}

	//设置closesocket（一般不会立即关闭而经历TIME_WAIT的过程）后想继续重用该socket s
	BOOL bReuseAddr = TRUE;
	if( setsockopt( s,SOL_SOCKET,SO_REUSEADDR,(char*)&bReuseAddr,sizeof(BOOL) ) == SOCKET_ERROR )
	{
		//关闭socket
		closesocket(s);

		sprintf_s( errorMsg,"setsockopt() failed to setsockopt with error: %d.POS:CServer::DataConnect()\n",WSAGetLastError() );
		//AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );

		return -1;
	}

	//绑定socket,指定通信对象
	if( bind( s,(struct sockaddr*)&addrInfo,sizeof(addrInfo)) == SOCKET_ERROR )
	{
		//关闭socket
		closesocket(s);

		sprintf_s( errorMsg,"bind() failed with error: %d.\n",WSAGetLastError() );
		//AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );

		return -1;
	}

	//主动与被动传输的不同操作
	if( nMode == MODE_PASV )
	{
		//开始监听
		if( listen( s,SOMAXCONN ) == SOCKET_ERROR )
		{
			//关闭socket
			closesocket(s);

			sprintf_s(errorMsg,"listen() failed with error: %d.\n",WSAGetLastError() );
			//AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );

			return -1;
		}
	} 
	else if( nMode == MODE_PORT )
	{
		struct sockaddr_in addr;

		addr.sin_family			= AF_INET;
		addr.sin_port			= htons( wPort );
		addr.sin_addr.s_addr	= dwIp;

		//连接到客户端
		if( connect( s, (const sockaddr*)&addr,sizeof(addr) ) == SOCKET_ERROR )
		{
			//关闭socket
			closesocket(s);

			sprintf_s(errorMsg,"connect() failed with error: %d\n",WSAGetLastError() );
			//AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );

			return -1;
		}
	}

	//正常结束
	return 0;
}

//获取本地IP地址
char* CServer::GetLocalAddress()
{
	char hostname[128];
	char errorMsg[50];

	//初始化置0
	memset( hostname,0,sizeof(hostname) );

	//获取主机名称
	if( gethostname( hostname,sizeof(hostname) ) == SOCKET_ERROR )
	{
		sprintf_s( errorMsg,"gethostname() failed with error: %d.POS:CServer::GetLocalAddress()",WSAGetLastError() );
		//AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );

		return NULL;
	}

	//gethostbyname()返回对应于给定主机名的包含主机名字和地址信息的hostent结构指针/失败返回一个空指针
    LPHOSTENT lpHostEnt;
	lpHostEnt = gethostbyname( hostname );
    if ( lpHostEnt == NULL )
	{
		sprintf_s( errorMsg,"gethostbyname() failed with error: %d.POS:CServer::GetLocalAddress()",WSAGetLastError() );
		//AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );
		return NULL;						
	}										

    // Format first address in the list
	struct in_addr* pinAddr;
	pinAddr = ((LPIN_ADDR)lpHostEnt->h_addr);

	//将网络地址转换成“.”点隔的字符串格式
	int length = (int)strlen( inet_ntoa(*pinAddr) );
	if ( (DWORD)length > sizeof(hostname) )
	{
		WSASetLastError(WSAEINVAL);
		return NULL;
	}

	return inet_ntoa(*pinAddr);
}

//接受连接请求
SOCKET CServer::AcceptConnectRequest(SOCKET &s)
{
	SOCKET sDialog = accept( s ,NULL,NULL );
	if( sDialog == INVALID_SOCKET ) 
	{
		//AfxMessageBox( _T("accept() failed.POS:CServer::AcceptConnectRequest()"),MB_OK|MB_ICONERROR );
		return NULL;
	}

	return sDialog;
}

//把文件列表信息转换成字符串，保存到buffer中,并返回buffer长度信息
int CServer::FileListToString(char *buffer, UINT nBufferSize, BOOL isListCommand)
{
	//定义一个文件信息数组用于暂存当前目录下找到的文件信息
	FILE_INF fileInfo[MAX_FILE_NUM];

	//调用函数获取文件列表信息保存到数组 fileInfo 中 / 返回文件数目，保存至fileAmmount
	int fileAmmount = GetFileList( fileInfo, MAX_FILE_NUM, "*.*" );

	//初始化
	sprintf( buffer,"%s","" );

	if( isListCommand )//------------------- LIST command,不仅仅需要文件名，还要文件大小，创建时间等信息
	{
		SYSTEMTIME systemTime;			//用于暂存文件时间转换过来的系统时间
		char tempTimeFormat[128]="";	//配置fileString中文件时间格式
		char timeBlock[20];				//配置fileString中文件时间格式中的AM、PM

		//开始把所有文件的文件名、创建时间信息集合到buffer中
		for( int i=0; i<fileAmmount; i++ )
		{
			//检测buffer是否有足够空间
			if( strlen(buffer)>nBufferSize-128 )
			{
				break;
			}

			//检测是否是文件
			if( strcmp(fileInfo[i].szFileName,".")==0 || strcmp(fileInfo[i].szFileName,"..")==0 )
			{
				continue;
			}

			//Converts a file time to system time format and save in systemTime 
			// success : return nonezero, failed : 0
			if( FileTimeToSystemTime( &(fileInfo[i].ftLastWriteTime), &systemTime)==0 )
			{
				char errorMsg[128];
				sprintf_s( errorMsg,"POS:CServer::FileListToString()\nFailed eo convert filetime to systemtime with error: %d.",GetLastError());
				//AfxMessageBox( errorMsg,MB_OK|MB_ICONERROR );
			}

			//配置fileString中文件时间格式中的AM、PM
			if( systemTime.wHour <= 12 ){
				strcpy( timeBlock,"AM" );
			}
			else
			{ 
				systemTime.wHour -= 12;
				strcpy( timeBlock,"PM" ); 
			}

			//配置fileString中文件时间格式中只用两位数来表示
			if( systemTime.wYear >= 2000 )
			{
				systemTime.wYear -= 2000;
			}
			else
			{
				systemTime.wYear -= 1900;
			}

			//配置时间格式
			ZeroMemory( tempTimeFormat,sizeof(tempTimeFormat) );
			sprintf_s( tempTimeFormat,"%02u-%02u-%02u  %02u:%02u%s   ",
						systemTime.wMonth,systemTime.wDay,systemTime.wYear,systemTime.wHour,systemTime.wMonth,timeBlock );

			//把当前文件时间保存至buffer数组中
			strcat( buffer,tempTimeFormat );

			//检查找到的结果是否目录
			if( fileInfo[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )	//是目录，不是文件
			{
				strcat( buffer,"<DIR>" );
				strcat( buffer,"                 " );
			}
			else															//不是目录，是文件
			{
				strcat( buffer,"     " );

				// 文件大小
				sprintf_s( tempTimeFormat,"%9d Bytes  ",fileInfo[i].nFileSizeLow );
			}

			// 文件名
			strcat( buffer,fileInfo[i].szFileName );
			strcat( buffer,"\r\n");
		}
	} 
	else//--------------------------------------------------NLIST command 只需要文件名
	{
		//把当前目录下的文件的名称集合到buffer中
		for( int i=0; i<fileAmmount; i++ )
		{
			//检测是否足够空间
			if( strlen(buffer) + strlen( fileInfo[i].szFileName ) + 2 < nBufferSize )
			{
				strcat( buffer, fileInfo[i].szFileName );
				strcat( buffer, "\r\n");
			} 
			else
			{
				break;
			}
		}
	}

	return (int)strlen( buffer );
}

//获取文件列表 / return 列表项数
int CServer::GetFileList(LPFILE_INF fileInfo, int arraySize, const char *searchPath)
{
	int index = 0;

	WIN32_FIND_DATA  wfd;
	ZeroMemory( &wfd,sizeof(WIN32_FIND_DATA) );

	char fileName[MAX_PATH];
	ZeroMemory( fileName,MAX_PATH );

	//获取当前目录
	GetCurrentDirectory( MAX_PATH,fileName );

	//获取当前目录的下一级目录
	if( fileName[ strlen(fileName)-1 ] != '\\' )
	{
		strcat( fileName,"\\" );
	}
	strcat( fileName, searchPath );

	//查找第一个文件（夹）
	HANDLE fileHandle = FindFirstFile( (LPCTSTR)fileName, &wfd );

	//获得文件属性
	if ( fileHandle != INVALID_HANDLE_VALUE ) 
	{
		lstrcpy( fileInfo[index].szFileName, wfd.cFileName );
		fileInfo[index].dwFileAttributes = wfd.dwFileAttributes;
		fileInfo[index].ftCreationTime	 = wfd.ftCreationTime;
		fileInfo[index].ftLastAccessTime = wfd.ftLastAccessTime;
		fileInfo[index].ftLastWriteTime  = wfd.ftLastWriteTime;
		fileInfo[index].nFileSizeHigh    = wfd.nFileSizeHigh;
		fileInfo[index].nFileSizeLow	 = wfd.nFileSizeLow;

		//index加一
		index++;

		//继续查找其他文件Long，非零表示成功，零表示失败
		while( FindNextFile( fileHandle,&wfd )!= 0 && index < arraySize ) 
		{
			lstrcpy( fileInfo[index].szFileName, wfd.cFileName );
			fileInfo[index].dwFileAttributes = wfd.dwFileAttributes;
			fileInfo[index].ftCreationTime	 = wfd.ftCreationTime;
			fileInfo[index].ftLastAccessTime = wfd.ftLastAccessTime;
			fileInfo[index].ftLastWriteTime  = wfd.ftLastWriteTime;
			fileInfo[index].nFileSizeHigh    = wfd.nFileSizeHigh;
			fileInfo[index++].nFileSizeLow   = wfd.nFileSizeLow;
		}

		//查找完毕，关闭文件句柄
		FindClose( fileHandle );
	}
	return index;
}

//
int CServer::DataSend(SOCKET s, char *buff, int buffSize)
{
	int nBytesLeft = buffSize;
	int index = 0;
	int sendBytes = 0;
	char errorMsg[128];

	while( nBytesLeft > 0 ) {
		sendBytes = send( s,&buff[index],nBytesLeft,0);
		if( sendBytes == SOCKET_ERROR )
		{
			closesocket( s );

			sprintf_s( errorMsg,"send() failed to send with error: %d.POS:CServer::DataSend()\n",WSAGetLastError() );
			//AfxMessageBox( errorMsg,MB_OK|MB_ICONERROR );

			return -1;
		}
		nBytesLeft -= sendBytes;
		index += sendBytes;
	}

	return index;
}

//把文件名及文件大小信息整合到一块,并保存到filenamesize中 / return: 文件的大小

int CServer::CombindFileNameSize(const char *filename, char *filnamesize)
{
	//假定文件的大小不超过4GB，只处理低位
	int fileSize = -1;
	FILE_INF fileInfo[1];

	//获取文件列表信息，并保存至fileInfo中，fileAmount记录文件数目
	int fileAmount = GetFileList( fileInfo,1,filename );
	if( fileAmount != 1 )
	{
		return -1;
	}

	//把文件名和大小信息整合到一块
	sprintf( filnamesize, "%s<%d bytes>",filename,fileInfo[0].nFileSizeLow );

	//获得文件大小
	fileSize = fileInfo[0].nFileSizeLow;

	//返回文件大小
	return fileSize;
}


////把指定文件写入缓存 / 返回总共读取字节数
int CServer::ReadFileToBuffer(const char *szFileName, char *buff, int fileSize)
{
	//local variable definition
	int  index		= 0;
	int  bytesLeft	= fileSize;
	int  bytesRead	= 0;

	//获取文件的完整路径
	char fileAbsolutePath[MAX_PATH];
	GetCurrentDirectory( MAX_PATH,fileAbsolutePath );
	if( fileAbsolutePath[ strlen(fileAbsolutePath)-1 ]!= '\\' )
	{
		strcat( fileAbsolutePath,"\\" );
	}
	strcat(fileAbsolutePath,szFileName );

	HANDLE hFile = CreateFile( fileAbsolutePath,
							   GENERIC_READ,
							   FILE_SHARE_READ,
							   NULL,
							   OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL,
							   NULL );
	if( hFile == INVALID_HANDLE_VALUE )
	{
		//AfxMessageBox( _T("Filed in CreateFile(),POS:CServer::ReadFileToBuffer()") );
	}
	else
	{
		while( bytesLeft > 0 )
		{
			if( !ReadFile( hFile,&buff[index],bytesLeft,(LPDWORD)&bytesRead,NULL ) )
			{
				//关闭句柄
				CloseHandle( hFile );
				//AfxMessageBox( _T("Filed in ReadFile(),POS:CServer::ReadFileToBuffer()") );

				return 0;
			}

			index += bytesRead;
			bytesLeft -= bytesRead;
		}

		//关闭句柄
		CloseHandle( hFile );
	}

	//返回总共读取字节数
	return index;
}

//数据接收，写入文件 / 只接收4M以下的文件
int CServer::DataRecv(SOCKET s, const char *fileName)
{
	DWORD  index		= 0;
	DWORD  bytesWritten = 0;
	DWORD  buffBytes	= 0;
	char   buff[DATA_BUFSIZE];
	int    totalRecvBytes = 0;

	//获取文件绝对路径
	char   fileAbsolutePath[MAX_PATH];
	GetCurrentDirectory( MAX_PATH,fileAbsolutePath );
	strcat( fileAbsolutePath,"\\" );
	strcat(fileAbsolutePath,fileName );

	//创建文件句柄
	HANDLE hFile = CreateFile( fileAbsolutePath,
							   GENERIC_WRITE,
							   FILE_SHARE_WRITE,
							   NULL,
							   OPEN_ALWAYS,
							   FILE_ATTRIBUTE_NORMAL,
							   NULL );
	if( hFile == INVALID_HANDLE_VALUE ) 
	{ 
		//AfxMessageBox( _T("CreateFile() failed.POS:CServer::DataRecv()") );
		return -1;
	}
	
	while( TRUE )
	{
		//初始化设置，准备接收数据
		int nBytesRecv	= 0;
		index			= 0; 
		buffBytes		= DATA_BUFSIZE;

		//开始接收数据
		while( buffBytes > 0 )
		{
			nBytesRecv = recv( s,&buff[index],buffBytes,0 );
			if( nBytesRecv == SOCKET_ERROR ) 
			{
				//AfxMessageBox( _T("Failed in recv().POS:CServer::DataRecv()") );
				return -1;
			}

			//接收为空或已经接收完
			if( nBytesRecv == 0 )
			{
				break;
			}
		
			index += nBytesRecv;
			buffBytes -= nBytesRecv;
		}

		//总共接收字节数增加
		totalRecvBytes += index;

		//要写入文件的字节数以接收到的字节数为准
		buffBytes = index;

		//索引清0指向开始位置
		index = 0;

		//开始写入文件
		while( buffBytes > 0 ) 
		{
			//移动文件指针到文件末尾
			if( !SetEndOfFile(hFile) )
			{
				return 0;
			}
			//
			if( !WriteFile( hFile,&buff[index],buffBytes,&bytesWritten,NULL ) ) 
			{
				CloseHandle( hFile );
				//AfxMessageBox( _T("写入文件出错.") );
				return 0;
			}

			index += bytesWritten;
			buffBytes -= bytesWritten;
		}

		//判断是否接收完，有可能文件大于定义的buff容量大小
		if( nBytesRecv == 0 )
		{
			break;
		}
	}

	//关闭文件句柄
	CloseHandle( hFile );

	//返回总共接收字节数
	return totalRecvBytes;
}

//获得相对路径
void CServer::GetRalativeDirectory(char *currDir,char*rootDir,char* ralaDir)
{
	int nStrLen = (int)strlen( rootDir );

	//比较字符串currDir和rootDir的前nStrLen个字符但不区分大小写
	if( strnicmp( currDir,rootDir, nStrLen )==0 )
	{
		strcpy( ralaDir,currDir+nStrLen );
	}

	//判断是否是根目录
	if( ralaDir!=NULL && ralaDir[0] == '\0' )
	{
		strcpy( ralaDir,"/" );
	}
}

////将\..\..格式转换为/../..格式
void CServer::HostToNet(char *path)
{
	int index = 0;
	while( path[index] ) 
	{ 
		if( path[index] == '\\' )
		{
			path[index] = '/';
		}

		index++;
	}
}

////获得绝对路径
void CServer::GetAbsoluteDirectory(char* dir,char* currentDir,char* userCurrDir )
{
	//local var
	char szTemp[MAX_PATH];
	int len = 0;

	//获得用户当前目录
	strcpy( userCurrDir,currentDir );

	//回到上一目录
	if( strcmp( dir,".." )==0 )
	{
		int flag = 0;
		len = (int)strlen( userCurrDir );
		for( int i=0 ; i<len ; i++ )
		{
			if( userCurrDir[i] == '\\' )
			{
				flag = i;
			}
		}

		//从最后一个'\'处截断
		if( flag>2 )
		{
			userCurrDir[flag] = '\0';
		}
		else if( flag==2 )
		{
			userCurrDir[flag+1] = '\0';
		}

		return;
	}

	//走到当前目录的下一个目录
	len = (int)strlen( userCurrDir );
	if( userCurrDir[len-1]=='\\' )
	{
		if( dir[0]=='/' || dir[0]=='\\' )
		{
			strcpy( szTemp,dir+1 );
		}
		else
		{
			strcpy( szTemp,dir );
		}
	}
	else
	{
		if( dir[0] != '/' && dir[0] != '\\' )
		{
			strcpy( szTemp,"\\" );
			strcat( szTemp, dir );
		}
		else
		{
			strcpy( szTemp,dir );
		}
	}

	//默认此文件夹在当前目录下
	strcat( userCurrDir,szTemp );

	////将/../..格式转换为\..\..格式
	NetToHost( userCurrDir );
}

////将/../..格式转换为\..\..格式
void CServer::NetToHost(char *path)
{
	int index = 0;
	while( path[index] ) 
	{ 
		if( path[index] == '/' )
		{
			path[index] = '\\';
		}

		index++;
	}
}

//删除文件
int CServer::TryDeleteFile(char *deletedPath)
{
	//定义一个CFileFind类对象用于查找
	CFileFind fileFinder;

	if( !fileFinder.FindFile( deletedPath ) )//----路径不合法
	{
		return CANNOT_FIND;
	}
	else if( DeleteFile( deletedPath ) )//---------文件能删除
	{
		return DIR_CHANGED;
	}
	else//-----------------------------------------文件暂时不能删除
	{
		return ACCESS_DENY;
	}
}

////检测路径是否合法
BOOL CServer::IsPathExist(char *path)
{
	if( GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES || GetLastError() != ERROR_FILE_NOT_FOUND )
	{
		return TRUE;
	}
		
	return FALSE;
}

//ADD