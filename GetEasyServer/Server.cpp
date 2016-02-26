#include "StdAfx.h"
#include "Server.h"


//ADD
//#include "ftpSrver.h"

/*********************/
/*  global data      */
/*********************/
DWORD				g_dwTotalEventAmount = 0;//���¼���
DWORD				g_index;
WSAEVENT			g_EventArray[WSA_MAXIMUM_WAIT_EVENTS];//�ֶ����ö���
LPSOCKET_INF		g_SocketArray[WSA_MAXIMUM_WAIT_EVENTS];//��SOCKET_INF�ṹ
CRITICAL_SECTION	g_CriticalSection;	//�ٽ���

//�����̺߳�������
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
	CServer *	server = (CServer*)lpParameter;//������
	SOCKADDR_IN addrInfo;			//���ü�����ַ���˿�
	DWORD		dwFlags;			//��ʵ�����壬ֻ�Ǻ���ʹ�ù����еĿհײ���
	DWORD		dwRecvBytes;		//��ʵ�����壬ֻ�Ǻ���ʹ�ù����еĿհײ���
	char		errorMsg[128];		//������ʾ��Ϣ
	SOCKADDR_IN ClientAddr;			//��ȡ�ͻ��˵�ַ��Ϣ
	int			ClientAddrLength;	//�ͻ��˵�ַ��ϢSize
	LPCTSTR		ClientIP;			//�ͻ���IP
	UINT		ClientPort;			//�ͻ���Port

	//��ʼ���ٽ���
   InitializeCriticalSection( &g_CriticalSection );

   // ��������socket ��sListen
   if ( ( server->sListen = WSASocket(AF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED) ) == INVALID_SOCKET ) 
   {
	   sprintf_s( errorMsg,"ERROR��Failed to get a socket %d.POS:ServerThread()\n", WSAGetLastError() );
	   ////AfxMessageBox( errorMsg,MB_OK,0 );
	   WSACleanup();
       return 0;
   }

   //����IP���˿�
   CString sIP;
   sIP.Format("%d.%d.%d.%d",server->IpFild[0],server->IpFild[1],server->IpFild[2],server->IpFild[3]);
   addrInfo.sin_family = AF_INET;
   addrInfo.sin_addr.S_un.S_addr = inet_addr(sIP);
   addrInfo.sin_port = htons(server->m_ServerPort);		//FTP_PORT);

   //�󶨼����˿�sListen��ָ��ͨ�Ŷ���
   if ( bind(server->sListen, (PSOCKADDR)&addrInfo, sizeof(addrInfo) ) == SOCKET_ERROR )
   {
	   sprintf_s( errorMsg,"ERROR��bind() failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
	   //AfxMessageBox( errorMsg,MB_OK,0 );
       return 0;
   }

   //����sListen���ȴ�����״̬
   if( listen( server->sListen, SOMAXCONN ) )
   {
	   sprintf_s( errorMsg,"ERROR��listen() failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
	   //AfxMessageBox( errorMsg,MB_OK,0 );
       return 0;
   }

   //�����Ựsocket
   if( ( server->sDialog = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED) ) == INVALID_SOCKET ) 
   {
	   sprintf_s( errorMsg,"ERROR��Failed to get a dialog socket %d.POS:ServerThread()\n", WSAGetLastError() );
	   //AfxMessageBox( errorMsg,MB_OK,0 );
	   return 0;
   }

   //������һ���ֶ����ö���
   if ( ( g_EventArray[0] = WSACreateEvent() ) == WSA_INVALID_EVENT )
   {
	   sprintf_s( errorMsg,"ERROR��WSACreateEvent() failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
	   //AfxMessageBox( errorMsg,MB_OK,0 );
	   return 0;
   }

   //���ڿ�ʼ���Ѿ�������һ���¼�
   g_dwTotalEventAmount = 1;

   //����һ���̴߳�������
   AfxBeginThread( ProcessThreadIO,(LPVOID)server );

   //����ѭ������ͣ�ĵȴ��û�������
   while( !server->m_bStopServer )
   {
	   //������վ����,����һ����������
	   ClientAddrLength = sizeof(ClientAddr);
	   if ( (server->sDialog = accept(server->sListen,(sockaddr*)&ClientAddr, &ClientAddrLength)) == INVALID_SOCKET )
	   {
		   return 0;
	   }

	   //��ȡ�ͻ��˵�ַ�˿���Ϣ
	   ClientIP		= inet_ntoa( ClientAddr.sin_addr );
	   ClientPort	= ClientAddr.sin_port;

      //�ش���ӭ��Ϣ
	  if( !server->SendWelcomeMsg( server->sDialog ) )
	  {
		  break;
	  }

	  //��ʼ�����ٽ�������ֹ����
      EnterCriticalSection( &g_CriticalSection );

      //����һ���µ�SOCKET_INF�ṹ������ܵ�����socket
      if( (g_SocketArray[g_dwTotalEventAmount] = (LPSOCKET_INF)GlobalAlloc(GPTR,sizeof(SOCKET_INF))) == NULL )
      {
		  sprintf_s( errorMsg,"ERROR��GlobalAlloc() failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
		  //AfxMessageBox( errorMsg,MB_OK,0 );
		  return 0;
      } 

      //��ʼ���µ�SOCKET_INF�ṹ
	  char buffer[DATA_BUFSIZE];
	  ZeroMemory(buffer, DATA_BUFSIZE);

	  g_SocketArray[g_dwTotalEventAmount]->wsaBuf.len = DATA_BUFSIZE;						//��ʼ��wsaBuff
	  g_SocketArray[g_dwTotalEventAmount]->wsaBuf.buf = buffer;

      g_SocketArray[g_dwTotalEventAmount]->socket = server->sDialog;						//��ʼ��socket

      memset( &(g_SocketArray[g_dwTotalEventAmount]->wsaOverLapped),0,sizeof(OVERLAPPED) ); //��ʼ��wsaOverLapped

	  memset( &(g_SocketArray[g_dwTotalEventAmount]->buffRecv), 0, DATA_BUFSIZE );			//��ʼ��buffRecv
	  memset( &(g_SocketArray[g_dwTotalEventAmount]->buffSend), 0, DATA_BUFSIZE );			//��ʼ��buffSend
      g_SocketArray[g_dwTotalEventAmount]->dwBytesSend = 0;
      g_SocketArray[g_dwTotalEventAmount]->dwBytesRecv = 0;

	  g_SocketArray[g_dwTotalEventAmount]->userLoggedIn = FALSE;							//�û���¼״̬

	  g_SocketArray[g_dwTotalEventAmount]->SocketStatus = WSA_RECV;							//��ʼ��Ԥ��״̬
   
     //�����¼�
      if ( ( g_SocketArray[g_dwTotalEventAmount]->wsaOverLapped.hEvent = 
		     g_EventArray[g_dwTotalEventAmount] = WSACreateEvent() ) == WSA_INVALID_EVENT )
      {
		  sprintf_s( errorMsg,"WSACreateEvent() failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
		  //AfxMessageBox( errorMsg,MB_OK,0 );
		  return 0;
      }

      //������������
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
			 //�ر�socket
			 closesocket( g_SocketArray[g_dwTotalEventAmount]->socket );

			 //�ر��¼�
			 WSACloseEvent( g_EventArray[g_dwTotalEventAmount] );

			 //������ʾ��Ϣ
			 sprintf_s( errorMsg,"ERROR��WSARecv() failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
			 //AfxMessageBox( errorMsg,MB_OK,0 );

			 return 0;
         }
      }

	  //����g_EventArray[]�¼�����Ŀ
      g_dwTotalEventAmount++;

	  //�뿪�ٽ���
      LeaveCriticalSection( &g_CriticalSection );

	  //ʹ��һ���¼����źţ�ʹ�������̴߳����������¼�
      if ( WSASetEvent( g_EventArray[0] ) == FALSE )
      {
		  sprintf_s( errorMsg,"ERROR��WSASetEvent failed with error %d.POS:ServerThread()\n", WSAGetLastError() );
		  //AfxMessageBox( errorMsg,MB_OK,0 );
		  return 0;
      }
   }

   //�رռ����߳�
   server->m_bStopServer = FALSE;

   //�˳������߳�
   return 0;
}

//�������̴߳�����
UINT ProcessThreadIO( LPVOID lpParameter )
{
   DWORD		dwFlags;
   LPSOCKET_INF pSocketInfo;					//����LPSOCKET_INF�ṹ
   DWORD		dwBytesTransferred;				//���ݴ����ֽ���
   DWORD		i;								//index
   CServer *	server = (CServer*)lpParameter;	//������
   char			errorMsg[128];					//������ʾ��Ϣ

   //�����첽��WSASend, WSARecv�������
   while( TRUE )
   {
	   //�����ص�������WSARecv()�� WSARecvFrom()��WSASend()��WSASendTo() �� WSAIoctl()�����ȴ��¼�֪ͨ
      if ( (g_index = WSAWaitForMultipleEvents(g_dwTotalEventAmount, g_EventArray, FALSE, WSA_INFINITE, FALSE)) == WSA_WAIT_FAILED )
      {
		  sprintf_s( errorMsg, "Error WSAWaitForMultipleEvents() failed with error:%d\n",WSAGetLastError() );
		  //AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );
		  return 0;
      }
      
	  //�Ե�һ���¼�����������
      if ( (g_index - WSA_WAIT_EVENT_0) == 0 )
      {
		  //����һ����������Ϊ���ź�״̬
          WSAResetEvent(g_EventArray[0]);
          continue;
      }

	  //ȡ�����ź�״̬���¼���Ӧ��SOCKET_INF�ṹ
      pSocketInfo = g_SocketArray[g_index - WSA_WAIT_EVENT_0];

	  //�����¼���Ϊ���ź�״̬
      WSAResetEvent(g_EventArray[g_index - WSA_WAIT_EVENT_0]);

	  //dwBytesTransferred == 0��ʾͨ�ŶԷ��Ƿ��Ѿ��ر�����
      if ( WSAGetOverlappedResult( pSocketInfo->socket, 
									&(pSocketInfo->wsaOverLapped), 
									&dwBytesTransferred,
									FALSE, 
									&dwFlags ) == FALSE || dwBytesTransferred == 0 )
      {
		  ////�ص�����ʧ��
		  //�ر��׽ӿ�
		 closesocket( pSocketInfo->socket );

		 //�ͷ��׽ӿ���Դ
         GlobalFree( pSocketInfo );

		 //�ر��¼�
         WSACloseEvent( g_EventArray[g_index - WSA_WAIT_EVENT_0] );

         // Cleanup g_SocketArray and g_EventArray by removing the socket event handle
         // and socket information structure if they are not at the end of the
         // arrays.
		 //�����ٽ���
         EnterCriticalSection( &g_CriticalSection );

		 //�����ƶ���ɾ���¼�
         if ( (g_index - WSA_WAIT_EVENT_0) + 1 != g_dwTotalEventAmount )
		 {
            for ( i=(g_index - WSA_WAIT_EVENT_0); i < g_dwTotalEventAmount; i++ ) 
			{
				g_EventArray[i] = g_EventArray[i+1];
			    g_SocketArray[i] = g_SocketArray[i+1];
            }
		}

		 //�¼�������һ
         g_dwTotalEventAmount--;

		 //�뿪�ٽ���
         LeaveCriticalSection(&g_CriticalSection);

		 //�¼������ã��������Ĵ��벻��ִ��
         continue;
      }

	  // ǰ���¼����ã������Ѿ������ݴ���
	  if( pSocketInfo->SocketStatus == WSA_RECV )
	  {
		  //���ݿ���
		  memcpy( &pSocketInfo->buffRecv[pSocketInfo->dwBytesRecv],pSocketInfo->wsaBuf.buf,dwBytesTransferred );
		  
		  //�����ֽ���
		  pSocketInfo->dwBytesRecv = dwBytesTransferred;	//���ģ�ԭ����pSocketInfo->dwBytesRecv += dwBytesTransferred;

		  //�����յ�������
		  if( pSocketInfo->dwBytesRecv > 2 
			  && pSocketInfo->buffRecv[pSocketInfo->dwBytesRecv-2] == '\r'      // Ҫ��֤�����\r\n
			  && pSocketInfo->buffRecv[pSocketInfo->dwBytesRecv-1] == '\n' )  
		  {
			  //����û��Ƿ��Ѿ���¼
			  if( !pSocketInfo->userLoggedIn )//-----------------------δ��¼:�û���¼
			 {
				 if( server->Login(pSocketInfo) == LOGGED_IN )
				 {
					 //�����û���¼״̬
					 pSocketInfo->userLoggedIn = TRUE;
					 
					 //����ftp��Ŀ¼ \ Nonzero if successful; otherwise 0
					 if( SetCurrentDirectory( pSocketInfo->userCurrentDir )==0 )
					 {
						 sprintf_s( errorMsg,"ERROR��POS:ServerThread()\nSetCurrentDirectory() failed with error: %d.", GetLastError() );
						 //AfxMessageBox( errorMsg,MB_OK|MB_ICONERROR );
					 }
				 }
			 }
			 else//--------------------------------------------------�Ѿ���¼���������������
			 {
				 if( server->DealWithCommand( pSocketInfo )==FTP_QUIT )
				 {
					 break;
				 }
			 }

			 //���������
			 memset( pSocketInfo->buffRecv,0,sizeof(pSocketInfo->buffRecv) );
			 pSocketInfo->dwBytesRecv = 0;
		  }
	  }
	  else//���Socket���ڷ���״̬
	  {
		  pSocketInfo->dwBytesSend += dwBytesTransferred;
	  }
	  
 	  // ���������Ժ���������
	  if( server->RecvRequest( pSocketInfo ) == -1 )
	  {
		  return -1;
	  }
   }

   //�����߳�
   return 0;
}


//����������
void CServer::ServerConfigInfo( AccountArray* accountArray,BYTE nFild[],UINT port)
{
	//�˻�����
	int size = (int)(*accountArray).GetCount();
	for( int i = 0 ; i < size ; i ++ )
	{
		m_RegisteredAccount.Add( (*accountArray)[i] );
	}

	//IP����
	for( int i = 0;i < 4;i ++)
	{
		IpFild[i] = nFild[i];
	}

	//�˿�����
	m_ServerPort = port;

	//���÷���������״̬
	m_bStopServer = FALSE;
}

//���ͻ�ӭ��Ϣ
BOOL CServer::SendWelcomeMsg( SOCKET s )
{
	char* welcomeInfo = "Welcome to use GetEasy-Server...\nServer ready..\r\n";

	if( send( s, welcomeInfo, (int)strlen(welcomeInfo), 0 )==SOCKET_ERROR ){
		//AfxMessageBox(_T("Failed in sending welcome msg. POS:CServer::SendWelcomeMsg"));
		return FALSE;
	}
	return TRUE;
}

//�û���¼����
int CServer::Login( LPSOCKET_INF socketInfo )
{
	//��̬����
	static char username[MAX_NAME_LEN];
	static char password[MAX_PWD_LEN];

	//���ر���
	int loginResult = 0;

	//��ȡ�û���
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
	//��ȡ�û���¼����
	if( strstr( strupr(socketInfo->buffRecv),"PASS" )||strstr( strlwr(socketInfo->buffRecv),"pass") )
	{
		sprintf_s( password,"%s",socketInfo->buffRecv+strlen("pass")+1 );
		strtok( password, "\r\n" );
		//�ж��û���������ȷ��
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
				//�ҵ����ϵ��û��ʺ�
				isAccountExisted = TRUE;
				//��õ�ǰ�û�Ԥ���FTP��Ŀ¼
				strcpy( socketInfo->userCurrentDir,m_RegisteredAccount[i].directory );
				strcpy( socketInfo->userRootDir,m_RegisteredAccount[i].directory );
				break;
			}
		}
		//�˻����������Ӧ�ķ�����Ϣ
		if( isAccountExisted )
		{
			sprintf_s( socketInfo->buffSend,"230 User:%s logged in.proceed.\r\n",username );
			loginResult = LOGGED_IN;
		}
		else{
			sprintf_s( socketInfo->buffSend,"530 User:%s cannot logged in, Wrong username or password.Try again.\r\n",username );
			loginResult = LOGIN_FAILED;
		}
		//����ACK
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("Failed in sending ACK. POS:CServer::Login()") );
			return -1;
		}
	}
	//user logged in successfully
	return loginResult;
}


//������Ӧ��Ϣ���ͻ���
int CServer::SendACK( LPSOCKET_INF socketInfo )
{
	//һ��ָ�������������ֽ����ı���
	static DWORD dwSendBytes = 0;
	char errorMsg[128];

	//����socket�ṹ״̬Ϊ����״̬
	socketInfo->SocketStatus = WSA_SEND;

	//��ʼ��wasOverLapped�ṹ
	memset( &(socketInfo->wsaOverLapped), 0, sizeof(WSAOVERLAPPED) );
	socketInfo->wsaOverLapped.hEvent = g_EventArray[g_index - WSA_WAIT_EVENT_0];

	//��ʼ��Ҫ���͵�����
    socketInfo->wsaBuf.buf = socketInfo->buffSend + socketInfo->dwBytesSend;
    socketInfo->wsaBuf.len = (int)strlen( socketInfo->buffSend ) - socketInfo->dwBytesSend;

	//����ACK
	if ( WSASend(socketInfo->socket,&(socketInfo->wsaBuf),1,&dwSendBytes,0,&(socketInfo->wsaOverLapped),NULL) == SOCKET_ERROR ) 
	{
        if ( WSAGetLastError() != ERROR_IO_PENDING )
		{
			sprintf_s( errorMsg,"WSASend() failed with error :%d.POS:CServer::SendACK()\n", WSAGetLastError() );
			//AfxMessageBox( errorMsg );
			return -1;
        }
    }

	//�ɹ�����
	return 0;
}

//���ܿͻ�������
int CServer::RecvRequest( LPSOCKET_INF socketInfo )
{
	//���ر�������
	DWORD dwRecvBytes = 0;		//�������ԣ�ԭ����static DWORD dwRecvBytes = 0;
	DWORD dwFlags = 0;
	char errorMsg[128];

	//����socket�ṹ״̬
	socketInfo->SocketStatus = WSA_RECV;
	
	//��ʼ��wsaOverLapped�ṹ
	memset( &(socketInfo->wsaOverLapped), 0,sizeof(WSAOVERLAPPED) );
	socketInfo->wsaOverLapped.hEvent = g_EventArray[g_index - WSA_WAIT_EVENT_0];

	//���ý����������ݳ���
	socketInfo->wsaBuf.len = DATA_BUFSIZE;

	//��ʼ������������
	if ( WSARecv(socketInfo->socket,&(socketInfo->wsaBuf),1,&dwRecvBytes,&dwFlags,&(socketInfo->wsaOverLapped),NULL) == SOCKET_ERROR )
	{
		if ( WSAGetLastError() != ERROR_IO_PENDING )
		{
			sprintf_s( errorMsg,"WSARecv() failed with error: %d.POS:CServer::RecvRequest()\n", WSAGetLastError() );
			//AfxMessageBox( errorMsg,MB_OK,MB_ICONERROR );
		    return -1;
		}
	}

	//�����������
	return 0;
}

//����������Ϣ
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

	//������ȫ��ת��Ϊ��д��ʽ�����ڴ���
	strupr( command );

	//��ʼ���룬�п���Ϊ��
	int index = 0;
	for( i++ ;  i<len && request[i]!='\r' && request[i]!='\n' ; i++ )
	{
		cmdtail[index] = request[i];
		index++;
	}
	cmdtail[index] = '\0';
}

//�������
int CServer::DealWithCommand( LPSOCKET_INF socketInfo )
{
	//variable difinition
	static SOCKET sDialog		= INVALID_SOCKET;
	static SOCKET sAccept		= INVALID_SOCKET;
	static BOOL   isPasvMode	= FALSE;				//�ж��Ƿ񱻶�����ģʽ
		
	int   operationResult		= 0;					//��¼ĳ�������̽��
	char  command[20];									//��¼����
	char  cmdtail[MAX_REQ_LEN];							//��¼������������
	char  currentDir[MAX_PATH];							//��¼��ǰFTPĿ¼
	char  relativeDir[128];								//��¼��ǰĿ¼�����Ŀ¼

	const char*  szOpeningAMode = "150 Opening ASCII mode data connection for ";
	static DWORD  dwIpAddr		= 0;					//Clientָ��ͨ��IP(�ĸ�'.'�ָ���ipת��Ϊ��Ӧ���޷��ų�����)
	static WORD   wPort			= 0;					//Clientָ��ͨ�Ŷ˿�

	//�������󣬷��������洢��cammand \ ���������������,�洢��cmdtail
	DivideRequest( socketInfo->buffRecv,command,cmdtail );
	
	//����Ƿ�Ϊ��
	if( command[0] ==  '\0' )
	{
		return -1;
	}

	//---------------------------//
	//------��ʼ��������---------//
	//---------------------------//
	//Ϊ��������ָ��һ��IP��ַ�ͱ��ض˿�,PORT��data port���ı�Ĭ�����ݶ˿ڣ�
	if( strstr(command,"PORT") )
	{
		// 0���ɹ�//-1��ʧ��
		//��','�ָ���IP��ַ��ʽ�ֽ�Ϊһ��DWORD��IP(32ΪInternet������ַ)0(dwIpAddr)��һ��WORD��16ΪTCP�˿ڵ�ַ���ζ˿�(wPort)
		if( ConvertCommaAddrToDotAddr( socketInfo->buffRecv+strlen("PORT")+1,&dwIpAddr,&wPort) == -1 )
		{
			//AfxMessageBox( _T("Failed in ConvertCommaAddrToDotAddr().CMD:PORT") );
			return -1;
		}

		//������Ϣ��Ӧ�ͻ���
		sprintf_s( socketInfo->buffSend,"%s","200 PORT Command successful.\r\n" );
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:PORT"),MB_OK|MB_ICONERROR );
			return -1;
		}

		//���´���ģʽ
		isPasvMode = FALSE;

		return CMD_OK;
	}

	//���߷�������һ���Ǳ�׼�˿���������������
	//PASV��passive (��server_DTP����Ϊ��������״̬��ʹ�������ݶ˿ڽ��������������ӣ�����������һ����������)
	if( strstr( command,"PASV") )
	{
		if( DataConnect( sAccept, htonl(INADDR_ANY), PORT_BIND, MODE_PASV ) == -1 )  //0==�ɹ���-1==ʧ��
		{
			//AfxMessageBox( "DataConnect() failed.CMD:PASV",MB_OK|MB_ICONERROR );
			return -1;
		}

		//��ȡ�������Ÿ�����������ɵ�IP����
		char szCommaAddress[40];
		if( ConvertDotAddrToCommaAddr( GetLocalAddress(),PORT_BIND,szCommaAddress )==-1 )
		{
			//AfxMessageBox( _T("Failed in ConvertCommaAddrToDotAddr().CMD:PASV") );
			return -1;
		}

		//����ACK
		sprintf_s( socketInfo->buffSend,"227 Entering Passive Mode (%s).\r\n",szCommaAddress );

		//����ACK
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:PASV"),MB_OK|MB_ICONERROR );
			return -1;
		}

		//���´���ģʽ
    	isPasvMode = TRUE;

		return PASSIVE_MODE;		
	}

	//�÷��������ͻ��˷���һ��Ŀ¼�б�;NLST��name list;LIST��list 
	if( strstr( command, "NLST") || strstr( command,"LIST") )
	{
		if( isPasvMode )
		{
			//�����������󣬲��½��Ựsocket
			sDialog = AcceptConnectRequest( sAccept );
		}

		if( !isPasvMode )	//��������
		{
			sprintf_s( socketInfo->buffSend,"%s/bin/ls.\r\n",szOpeningAMode );
		}
		else				//��������
		{
			strcpy( socketInfo->buffSend,"125 Data connection already open; Transfer starting.\r\n" );
		}
		
		//send ACK
		if( SendACK( socketInfo ) == -1 )
		{
			//AfxMessageBox( _T("SendACK() failed.CMD:NLST/LIST"),MB_OK|MB_ICONERROR );
			return -1;
		}

		// ȡ���ļ��б���Ϣ����ת�����ַ���
		BOOL isListCommand = strstr( command,"LIST" ) ? TRUE : FALSE;
		char buffer[DATA_BUFSIZE];

		UINT nStrLen = FileListToString( buffer,sizeof(buffer),isListCommand);

		//������������ģʽ֮��Ĳ��
		if( !isPasvMode )				//��������ģʽ
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

			//�ر�socket
			closesocket(sAccept);
		}
		else							//��������ģʽ
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

	//�÷��������ͻ��˴���һ����·������ָ�����ļ��ĸ���(��ʵ����FTP���ع���ʵ��)
	//RETR��retrieve��������ʹ������_DTP����·����ָ�����ļ�����������������һ�˵ķ��������û�DTP
	if( strstr( command, "RETR") )
	{
		if( isPasvMode )
		{
			//�����������󣬲��½��Ựsocket
			sDialog = AcceptConnectRequest( sAccept );
		}

		char fileNameSize[MAX_PATH];
		int nFileSize = CombindFileNameSize( cmdtail,fileNameSize ); //���ػ�ȡ�ļ���С�����ļ�������С��Ϣ������fileNameSize��

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
			//AfxMessageBox( _T("���仺��ʧ��!\n") );
			return -1;
		}

		////��ָ���ļ�д�뻺�� / �����ܹ���ȡ�ֽ���
		if( ReadFileToBuffer( cmdtail,buffer, nFileSize ) == nFileSize )
		{
			// ����Data FTP����
			Sleep( 10 );

			if( isPasvMode )
			{
				DataSend( sDialog,buffer,nFileSize );
				closesocket( sDialog );
			}
			else
			{
				//��������
				if( DataConnect( sAccept,dwIpAddr,wPort,MODE_PORT ) == -1 )
				{
					return -1;
				}

				//��������
				DataSend( sAccept, buffer, nFileSize );
				closesocket( sAccept );
			}
		}

		//�������
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

	//STOR---store��������ʹ������DTP���վ��������Ӵ�����������ݣ���������Ϊ�ļ����ڷ�������(ʵ�ʾ���FTP�ϴ�����)
	if( strstr( command, "STOR") )
	{
		//����Ǳ���ģʽ������Ҫ�������ӣ������Ựsocket
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

		// ����Data FTP����
		if( isPasvMode )
		{
			//��������
			DataRecv( sDialog,cmdtail );
		}
		else
		{
			if( DataConnect( sAccept,dwIpAddr,wPort,MODE_PORT ) == -1 )
			{
				return -1;
			}

			//��������
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

	//��ֹUSER�����û�����ڽ����ļ����䣬���������رտ������ӣ�����ļ�����
	//���ڽ��У���������Ȼ��ֱ��������Ӧ��Ȼ�����������ر�;QUIT---logout
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

	//PWD:��Ӧ���з��ص�ǰ����Ŀ¼������;PWD---print working directory
	if( strstr( command,"XPWD" ) || strstr( command,"PWD") ) 
	{
		//��ȡ��ǰĿ¼
		GetCurrentDirectory( MAX_PATH,currentDir );

		//��ȡ��ǰĿ¼�����Ŀ¼
		char tempStr[128];
		GetRalativeDirectory(currentDir,socketInfo->userRootDir,tempStr);
		HostToNet( tempStr );

		//����ACK
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
		//����Ƿ��Ŀ¼
		if( cmdtail[0]=='\0' )
		{
			strcpy(cmdtail,"\\");
		}

		//���Ӧ��ת�䵽��Ŀ¼
		char szSetDir[MAX_PATH];
		strcpy( szSetDir,".." );

		//���õ�ǰĿ¼ \ nonzero is reasonable
		if( SetCurrentDirectory( szSetDir )==0 )
		{
			//set ACK
			sprintf_s( socketInfo->buffSend,"550 '%s' No such file or Directory.\r\n",szSetDir );

			//��¼������
			operationResult = CANNOT_FIND;
		} 
		else
		{
			//��õ�ǰĿ¼
			GetCurrentDirectory( MAX_PATH,currentDir );

			//������·��
			relativeDir[0] = '\0';
			GetRalativeDirectory(currentDir,socketInfo->userRootDir,relativeDir);

			//��\..\..��ʽת��Ϊ/../..��ʽ,�Ա���Ӧ�ͻ���
			HostToNet( relativeDir );

			//set ACK
			sprintf_s( socketInfo->buffSend,"250 'CDUP' command successful,'/%s' is current directory.\r\n",relativeDir );

			//��¼������
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

	//CWD:�ѵ�ǰĿ¼��ΪԶ���ļ�ϵͳ��ָ��·����������ı��¼���ʺ���Ϣ�������
	//CWD---change working directory
	if( strstr( command,"CWD" ) )
	{
		//����Ƿ��Ŀ¼
		if( cmdtail=='\0' )
		{
			strcpy( cmdtail,"\\" );
		}

		//���Ӧ��ת�䵽��Ŀ¼
		GetAbsoluteDirectory( cmdtail,socketInfo->userCurrentDir,currentDir );

		//��⵱ǰĿ¼�Ƿ��ڸ�Ŀ¼֮��
		if( strlen(currentDir) < strlen(socketInfo->userRootDir) )
		{
			//������Ӧ��Ϣ
			sprintf_s( socketInfo->buffSend,"550 'CWD' command failed,'%s' User no power.\r\n",cmdtail );
		}		
		else if( SetCurrentDirectory( currentDir )==0 )  //���õ�ǰĿ¼ \ nonzero is reasonable
		{
			//������Ӧ��Ϣ
			sprintf_s( socketInfo->buffSend,"550 'CWD' command failed,'%s': No such file or Directory.\r\n",cmdtail );

			//��¼������
			operationResult = CANNOT_FIND;
		}
		else
		{
			//�����û���ǰĿ¼��Ϣ
			strcpy( socketInfo->userCurrentDir,currentDir );

			//������·��
			relativeDir[0] = '\0';
			GetRalativeDirectory(currentDir,socketInfo->userRootDir,relativeDir);

			//��\..\..��ʽת��Ϊ/../..��ʽ,�Ա���Ӧ�ͻ���
			HostToNet( relativeDir );

			//������Ӧ��Ϣ
			sprintf_s( socketInfo->buffSend,"250 'CWD' command successful,'/%s' is current directory.\r\n",relativeDir );

			//��¼������
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

	//ɾ���ļ� ;DELE---delete, XRMD childɾ����Ϊ��child����Ŀ¼;RMD--Remove Directory
	if( strstr( command,"DELE") )
	{
		//���Ӧ��ת�䵽��Ŀ¼
		GetAbsoluteDirectory( cmdtail,socketInfo->userCurrentDir,currentDir );

		//������·��
		GetRalativeDirectory( currentDir,socketInfo->userRootDir,relativeDir );

		//��\..\..��ʽת��Ϊ/../..��ʽ,�Ա���Ӧ�ͻ���
		HostToNet( relativeDir );

		//����ɾ���ļ� �� �ļ���
		BOOL isDELE = TRUE;
			operationResult = TryDeleteFile( currentDir );

		//����ɾ�������������Ӧ��ϢACK
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

	//ȷ�����ݵĴ��䷽ʽ
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

	//Restart�����־�ļ��ڵ����ݵ㣬��������㿪ʼ���������ļ�
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

	//���඼����Ч������
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
	int commaAmout	= 0;			//�������Ÿ���
	char dotAddr[MAX_ADDR_LEN];		//������ȡ�ĸ����������IP��ַ��a.b.c.d��
	char szPort[MAX_ADDR_LEN];		//������ȡip��ַ����Ķ˿ں�

	//��ʼ��Ϊ0
	memset( dotAddr,0,sizeof(dotAddr) );
	memset( szPort, 0, sizeof(szPort)   );
	*pdwIpAddr	= 0;
	*pwPort		= 0;

	//�ҳ����е�','����Ϊ'.'
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

	//���Ϸ���
	if( commaAmout != 5 )
	{
		return -1;
	}

	//���õ����ĸ�'.'�ָ���ipת��Ϊ��Ӧ���޷��ų����ͱ�ʾ
	(*pdwIpAddr) = inet_addr( dotAddr );
	if( (*pdwIpAddr)  == INADDR_NONE )
	{
		return -1;
	}

	char *temp = strtok( szPort+1,"." );	//ǰ����
	if( temp == NULL )
	{
		return -1;
	}
	(*pwPort) = (WORD)( atoi(temp) * 256 );
	
	temp = strtok(NULL,".");				//�󲿷�
	if( temp == NULL )
	{
		return -1;
	}
	(*pwPort) += (WORD)atoi( temp );
		
	return 0;
}

//��һ��32λInternet������ַ��һ��16λTCP�˿ڵ�ַת��Ϊһ�������������Ÿ�����������ɵ�IP��ַ����
int CServer::ConvertDotAddrToCommaAddr(char *dotAddr, WORD wPort, char* commaAddr )
{
	//����˿�
	char szPort[10];
	sprintf_s( szPort,"%d,%d",wPort/256,wPort%256 );

	//�����׼Internet IP
	sprintf( commaAddr,"%s,",dotAddr );

	//��commaAddr�е�'.'ת��Ϊ','
	int index = 0;
	while( commaAddr[index] ) 
	{
		if( commaAddr[index] == '.' )
		{
			commaAddr[index] = ',';
		}

		index++;
	}

	//��IP���˿�ƴ������
	sprintf( commaAddr,"%s%s",commaAddr,szPort );

	return 0;
}

//�������Ӻ���--��������״̬�����������ģʽ��������connect�������ͻ��ˣ�����Ǳ���ģʽ����������ʼ����
int CServer::DataConnect(SOCKET &s, DWORD dwIp, WORD wPort, int nMode)
{
	char errorMsg[128];
	//����һ��socket
	s = socket( AF_INET,SOCK_STREAM,0 );
	if( s == INVALID_SOCKET )
	{
		sprintf_s( errorMsg,"socket() failed to get a socket with error: %d.POS:CServer::DataConnect()\n", WSAGetLastError() );
		 //AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );
		 return -1;
	}

	//���õ�ַ��Ϣ
	struct sockaddr_in addrInfo;

	//���õ�ַ��
	addrInfo.sin_family = AF_INET;

	//���ö˿ں�IP
	if( nMode == MODE_PASV )
	{
		addrInfo.sin_port = htons( wPort );
		addrInfo.sin_addr.s_addr = dwIp;
	}
	else
	{
		addrInfo.sin_port = htons( DATA_FTP_PORT );					//DATA_FTP_PORT==20
		addrInfo.sin_addr.s_addr = inet_addr( GetLocalAddress() );  //���ú�����ȡ������ַ
	}

	//����closesocket��һ�㲻�������رն�����TIME_WAIT�Ĺ��̣�����������ø�socket s
	BOOL bReuseAddr = TRUE;
	if( setsockopt( s,SOL_SOCKET,SO_REUSEADDR,(char*)&bReuseAddr,sizeof(BOOL) ) == SOCKET_ERROR )
	{
		//�ر�socket
		closesocket(s);

		sprintf_s( errorMsg,"setsockopt() failed to setsockopt with error: %d.POS:CServer::DataConnect()\n",WSAGetLastError() );
		//AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );

		return -1;
	}

	//��socket,ָ��ͨ�Ŷ���
	if( bind( s,(struct sockaddr*)&addrInfo,sizeof(addrInfo)) == SOCKET_ERROR )
	{
		//�ر�socket
		closesocket(s);

		sprintf_s( errorMsg,"bind() failed with error: %d.\n",WSAGetLastError() );
		//AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );

		return -1;
	}

	//�����뱻������Ĳ�ͬ����
	if( nMode == MODE_PASV )
	{
		//��ʼ����
		if( listen( s,SOMAXCONN ) == SOCKET_ERROR )
		{
			//�ر�socket
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

		//���ӵ��ͻ���
		if( connect( s, (const sockaddr*)&addr,sizeof(addr) ) == SOCKET_ERROR )
		{
			//�ر�socket
			closesocket(s);

			sprintf_s(errorMsg,"connect() failed with error: %d\n",WSAGetLastError() );
			//AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );

			return -1;
		}
	}

	//��������
	return 0;
}

//��ȡ����IP��ַ
char* CServer::GetLocalAddress()
{
	char hostname[128];
	char errorMsg[50];

	//��ʼ����0
	memset( hostname,0,sizeof(hostname) );

	//��ȡ��������
	if( gethostname( hostname,sizeof(hostname) ) == SOCKET_ERROR )
	{
		sprintf_s( errorMsg,"gethostname() failed with error: %d.POS:CServer::GetLocalAddress()",WSAGetLastError() );
		//AfxMessageBox( errorMsg, MB_OK|MB_ICONSTOP );

		return NULL;
	}

	//gethostbyname()���ض�Ӧ�ڸ����������İ����������ֺ͵�ַ��Ϣ��hostent�ṹָ��/ʧ�ܷ���һ����ָ��
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

	//�������ַת���ɡ�.��������ַ�����ʽ
	int length = (int)strlen( inet_ntoa(*pinAddr) );
	if ( (DWORD)length > sizeof(hostname) )
	{
		WSASetLastError(WSAEINVAL);
		return NULL;
	}

	return inet_ntoa(*pinAddr);
}

//������������
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

//���ļ��б���Ϣת�����ַ��������浽buffer��,������buffer������Ϣ
int CServer::FileListToString(char *buffer, UINT nBufferSize, BOOL isListCommand)
{
	//����һ���ļ���Ϣ���������ݴ浱ǰĿ¼���ҵ����ļ���Ϣ
	FILE_INF fileInfo[MAX_FILE_NUM];

	//���ú�����ȡ�ļ��б���Ϣ���浽���� fileInfo �� / �����ļ���Ŀ��������fileAmmount
	int fileAmmount = GetFileList( fileInfo, MAX_FILE_NUM, "*.*" );

	//��ʼ��
	sprintf( buffer,"%s","" );

	if( isListCommand )//------------------- LIST command,��������Ҫ�ļ�������Ҫ�ļ���С������ʱ�����Ϣ
	{
		SYSTEMTIME systemTime;			//�����ݴ��ļ�ʱ��ת��������ϵͳʱ��
		char tempTimeFormat[128]="";	//����fileString���ļ�ʱ���ʽ
		char timeBlock[20];				//����fileString���ļ�ʱ���ʽ�е�AM��PM

		//��ʼ�������ļ����ļ���������ʱ����Ϣ���ϵ�buffer��
		for( int i=0; i<fileAmmount; i++ )
		{
			//���buffer�Ƿ����㹻�ռ�
			if( strlen(buffer)>nBufferSize-128 )
			{
				break;
			}

			//����Ƿ����ļ�
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

			//����fileString���ļ�ʱ���ʽ�е�AM��PM
			if( systemTime.wHour <= 12 ){
				strcpy( timeBlock,"AM" );
			}
			else
			{ 
				systemTime.wHour -= 12;
				strcpy( timeBlock,"PM" ); 
			}

			//����fileString���ļ�ʱ���ʽ��ֻ����λ������ʾ
			if( systemTime.wYear >= 2000 )
			{
				systemTime.wYear -= 2000;
			}
			else
			{
				systemTime.wYear -= 1900;
			}

			//����ʱ���ʽ
			ZeroMemory( tempTimeFormat,sizeof(tempTimeFormat) );
			sprintf_s( tempTimeFormat,"%02u-%02u-%02u  %02u:%02u%s   ",
						systemTime.wMonth,systemTime.wDay,systemTime.wYear,systemTime.wHour,systemTime.wMonth,timeBlock );

			//�ѵ�ǰ�ļ�ʱ�䱣����buffer������
			strcat( buffer,tempTimeFormat );

			//����ҵ��Ľ���Ƿ�Ŀ¼
			if( fileInfo[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )	//��Ŀ¼�������ļ�
			{
				strcat( buffer,"<DIR>" );
				strcat( buffer,"                 " );
			}
			else															//����Ŀ¼�����ļ�
			{
				strcat( buffer,"     " );

				// �ļ���С
				sprintf_s( tempTimeFormat,"%9d Bytes  ",fileInfo[i].nFileSizeLow );
			}

			// �ļ���
			strcat( buffer,fileInfo[i].szFileName );
			strcat( buffer,"\r\n");
		}
	} 
	else//--------------------------------------------------NLIST command ֻ��Ҫ�ļ���
	{
		//�ѵ�ǰĿ¼�µ��ļ������Ƽ��ϵ�buffer��
		for( int i=0; i<fileAmmount; i++ )
		{
			//����Ƿ��㹻�ռ�
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

//��ȡ�ļ��б� / return �б�����
int CServer::GetFileList(LPFILE_INF fileInfo, int arraySize, const char *searchPath)
{
	int index = 0;

	WIN32_FIND_DATA  wfd;
	ZeroMemory( &wfd,sizeof(WIN32_FIND_DATA) );

	char fileName[MAX_PATH];
	ZeroMemory( fileName,MAX_PATH );

	//��ȡ��ǰĿ¼
	GetCurrentDirectory( MAX_PATH,fileName );

	//��ȡ��ǰĿ¼����һ��Ŀ¼
	if( fileName[ strlen(fileName)-1 ] != '\\' )
	{
		strcat( fileName,"\\" );
	}
	strcat( fileName, searchPath );

	//���ҵ�һ���ļ����У�
	HANDLE fileHandle = FindFirstFile( (LPCTSTR)fileName, &wfd );

	//����ļ�����
	if ( fileHandle != INVALID_HANDLE_VALUE ) 
	{
		lstrcpy( fileInfo[index].szFileName, wfd.cFileName );
		fileInfo[index].dwFileAttributes = wfd.dwFileAttributes;
		fileInfo[index].ftCreationTime	 = wfd.ftCreationTime;
		fileInfo[index].ftLastAccessTime = wfd.ftLastAccessTime;
		fileInfo[index].ftLastWriteTime  = wfd.ftLastWriteTime;
		fileInfo[index].nFileSizeHigh    = wfd.nFileSizeHigh;
		fileInfo[index].nFileSizeLow	 = wfd.nFileSizeLow;

		//index��һ
		index++;

		//�������������ļ�Long�������ʾ�ɹ������ʾʧ��
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

		//������ϣ��ر��ļ����
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

//���ļ������ļ���С��Ϣ���ϵ�һ��,�����浽filenamesize�� / return: �ļ��Ĵ�С

int CServer::CombindFileNameSize(const char *filename, char *filnamesize)
{
	//�ٶ��ļ��Ĵ�С������4GB��ֻ�����λ
	int fileSize = -1;
	FILE_INF fileInfo[1];

	//��ȡ�ļ��б���Ϣ����������fileInfo�У�fileAmount��¼�ļ���Ŀ
	int fileAmount = GetFileList( fileInfo,1,filename );
	if( fileAmount != 1 )
	{
		return -1;
	}

	//���ļ����ʹ�С��Ϣ���ϵ�һ��
	sprintf( filnamesize, "%s<%d bytes>",filename,fileInfo[0].nFileSizeLow );

	//����ļ���С
	fileSize = fileInfo[0].nFileSizeLow;

	//�����ļ���С
	return fileSize;
}


////��ָ���ļ�д�뻺�� / �����ܹ���ȡ�ֽ���
int CServer::ReadFileToBuffer(const char *szFileName, char *buff, int fileSize)
{
	//local variable definition
	int  index		= 0;
	int  bytesLeft	= fileSize;
	int  bytesRead	= 0;

	//��ȡ�ļ�������·��
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
				//�رվ��
				CloseHandle( hFile );
				//AfxMessageBox( _T("Filed in ReadFile(),POS:CServer::ReadFileToBuffer()") );

				return 0;
			}

			index += bytesRead;
			bytesLeft -= bytesRead;
		}

		//�رվ��
		CloseHandle( hFile );
	}

	//�����ܹ���ȡ�ֽ���
	return index;
}

//���ݽ��գ�д���ļ� / ֻ����4M���µ��ļ�
int CServer::DataRecv(SOCKET s, const char *fileName)
{
	DWORD  index		= 0;
	DWORD  bytesWritten = 0;
	DWORD  buffBytes	= 0;
	char   buff[DATA_BUFSIZE];
	int    totalRecvBytes = 0;

	//��ȡ�ļ�����·��
	char   fileAbsolutePath[MAX_PATH];
	GetCurrentDirectory( MAX_PATH,fileAbsolutePath );
	strcat( fileAbsolutePath,"\\" );
	strcat(fileAbsolutePath,fileName );

	//�����ļ����
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
		//��ʼ�����ã�׼����������
		int nBytesRecv	= 0;
		index			= 0; 
		buffBytes		= DATA_BUFSIZE;

		//��ʼ��������
		while( buffBytes > 0 )
		{
			nBytesRecv = recv( s,&buff[index],buffBytes,0 );
			if( nBytesRecv == SOCKET_ERROR ) 
			{
				//AfxMessageBox( _T("Failed in recv().POS:CServer::DataRecv()") );
				return -1;
			}

			//����Ϊ�ջ��Ѿ�������
			if( nBytesRecv == 0 )
			{
				break;
			}
		
			index += nBytesRecv;
			buffBytes -= nBytesRecv;
		}

		//�ܹ������ֽ�������
		totalRecvBytes += index;

		//Ҫд���ļ����ֽ����Խ��յ����ֽ���Ϊ׼
		buffBytes = index;

		//������0ָ��ʼλ��
		index = 0;

		//��ʼд���ļ�
		while( buffBytes > 0 ) 
		{
			//�ƶ��ļ�ָ�뵽�ļ�ĩβ
			if( !SetEndOfFile(hFile) )
			{
				return 0;
			}
			//
			if( !WriteFile( hFile,&buff[index],buffBytes,&bytesWritten,NULL ) ) 
			{
				CloseHandle( hFile );
				//AfxMessageBox( _T("д���ļ�����.") );
				return 0;
			}

			index += bytesWritten;
			buffBytes -= bytesWritten;
		}

		//�ж��Ƿ�����꣬�п����ļ����ڶ����buff������С
		if( nBytesRecv == 0 )
		{
			break;
		}
	}

	//�ر��ļ����
	CloseHandle( hFile );

	//�����ܹ������ֽ���
	return totalRecvBytes;
}

//������·��
void CServer::GetRalativeDirectory(char *currDir,char*rootDir,char* ralaDir)
{
	int nStrLen = (int)strlen( rootDir );

	//�Ƚ��ַ���currDir��rootDir��ǰnStrLen���ַ��������ִ�Сд
	if( strnicmp( currDir,rootDir, nStrLen )==0 )
	{
		strcpy( ralaDir,currDir+nStrLen );
	}

	//�ж��Ƿ��Ǹ�Ŀ¼
	if( ralaDir!=NULL && ralaDir[0] == '\0' )
	{
		strcpy( ralaDir,"/" );
	}
}

////��\..\..��ʽת��Ϊ/../..��ʽ
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

////��þ���·��
void CServer::GetAbsoluteDirectory(char* dir,char* currentDir,char* userCurrDir )
{
	//local var
	char szTemp[MAX_PATH];
	int len = 0;

	//����û���ǰĿ¼
	strcpy( userCurrDir,currentDir );

	//�ص���һĿ¼
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

		//�����һ��'\'���ض�
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

	//�ߵ���ǰĿ¼����һ��Ŀ¼
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

	//Ĭ�ϴ��ļ����ڵ�ǰĿ¼��
	strcat( userCurrDir,szTemp );

	////��/../..��ʽת��Ϊ\..\..��ʽ
	NetToHost( userCurrDir );
}

////��/../..��ʽת��Ϊ\..\..��ʽ
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

//ɾ���ļ�
int CServer::TryDeleteFile(char *deletedPath)
{
	//����һ��CFileFind��������ڲ���
	CFileFind fileFinder;

	if( !fileFinder.FindFile( deletedPath ) )//----·�����Ϸ�
	{
		return CANNOT_FIND;
	}
	else if( DeleteFile( deletedPath ) )//---------�ļ���ɾ��
	{
		return DIR_CHANGED;
	}
	else//-----------------------------------------�ļ���ʱ����ɾ��
	{
		return ACCESS_DENY;
	}
}

////���·���Ƿ�Ϸ�
BOOL CServer::IsPathExist(char *path)
{
	if( GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES || GetLastError() != ERROR_FILE_NOT_FOUND )
	{
		return TRUE;
	}
		
	return FALSE;
}

//ADD