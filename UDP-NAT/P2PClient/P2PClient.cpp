 /* P2P 程序客户端
 * 
 * 文件名：P2PClient.c
 *
 * 日期：2004-5-21
 *
 * 作者：shootingstars(zhouhuis22@sina.com)
 *
 */

/*  源码修改声明:
 *
 *	这个修改的代码是早期为了节省时间在原作者(shootingstars)的代码的基础上修
 *  改测试用的。原代码的版权归原作者所有。对于做的修改的部分本人不作任何担保，
 *  只是为了方便大家使用修改后的代码对于UDP穿透进行验证而已。
 *
 *  原作者的关于UDP穿透的简介是网上较早的一份对于UDP穿透内网的说明，但是并
 *  不是特别的清楚和详细。本人在从事P2P程序开发的过程中, 对于内网穿透做了很
 *  好的实现。由于经常有人问起这方面的问题，因此决定将自己对于使用UDP内网
 *  穿透的经验也写出来与大家分享， 希望对于从事这方面工作的朋友有所帮助.
 *  
 *  源码修改者简介:
 *
 *  Hwycheng Leo, like C/C++/Python/Perl, hate Java. 
 *  一直使用C/C++语言从事网络服务器和客户端方面的开发工作。近两年来主攻P2P技术方向。
 *  开发了完全免费强大的BitTorrent下载软件 - FlashBT(变态快车). 近期在公司负责开发
 *  P2P + IM 的商业化平台。
 *  
 *  邮件/MSN: FlashBT@Hotmail.com           [欢迎热爱网络编程和P2P开发的来信交流]
 *  软件主页: http://www.hwysoft.com/chs/   [FlashBT(变态快车)的官方主页]
 *  个人Blog: http://hwycheng.blogchina.com [主要是自己写的一些技术文章和生活感受]
 */

#pragma comment(lib,"ws2_32.lib")

#include "windows.h"
#include "..\proto.h"
#include "..\Exception.h"
#include <iostream>

using namespace std;

USHORT g_nClientPort = 9896;
USHORT g_nServerPort = SERVER_PORT;

UserList ClientList;

#define COMMANDMAXC 256
#define MAXRETRY    5

SOCKET PrimaryUDP;
char UserName[10];
char ServerIP[20];

bool RecvedACK;

void InitWinSock()
{
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("Windows sockets 2.2 startup");
		throw Exception("");
	}
	else{
		printf("Using %s (Status: %s)\n",
			wsaData.szDescription, wsaData.szSystemStatus);
		printf("with API versions %d.%d to %d.%d\n\n",
			LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion),
			LOBYTE(wsaData.wHighVersion), HIBYTE(wsaData.wHighVersion));
	}
}

SOCKET mksock(int type)
{
	SOCKET sock = socket(AF_INET, type, 0);
	if (sock < 0)
	{
        printf("create socket error");
		throw Exception("");
	}
	return sock;
}

stUserListNode GetUser(char *username)
{
	for(UserList::iterator UserIterator=ClientList.begin();
						UserIterator!=ClientList.end();
							++UserIterator)
	{
		if( strcmp( ((*UserIterator)->userName), username) == 0 )
			return *(*UserIterator);
	}
	throw Exception("not find this user");
}

void BindSock(SOCKET sock)
{
	sockaddr_in sin;
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(g_nClientPort);
	
	if (bind(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0)
		throw Exception("bind error");
}

void ConnectToServer(SOCKET sock,char *username, char *serverip)
{
	sockaddr_in remote;
	remote.sin_addr.S_un.S_addr = inet_addr(serverip);
	remote.sin_family = AF_INET;
	remote.sin_port = htons(g_nServerPort);
	
	stMessage sendbuf;
	sendbuf.iMessageType = LOGIN;
	strncpy(sendbuf.message.loginmember.userName, username, 10);

	sendto(sock, (const char*)&sendbuf, sizeof(sendbuf), 0, (const sockaddr*)&remote,sizeof(remote));
	int usercount;
	int fromlen = sizeof(remote);

    for ( ;; )
    {
        fd_set readfds;
        fd_set writefds;

        FD_ZERO( &readfds );
        FD_ZERO( &writefds );

        FD_SET( sock, &readfds );
        int maxfd = sock;

        timeval to;
        to.tv_sec = 2;
        to.tv_usec = 0;

        int n = select( maxfd + 1, &readfds, &writefds, NULL, &to );

        if ( n > 0 )
        {
            if ( FD_ISSET( sock, &readfds ) )
			{
				int iread = recvfrom(sock, (char *)&usercount, sizeof(int), 0, (sockaddr *)&remote, &fromlen);
				if(iread<=0)
				{
					throw Exception("Login error\n");
				}

				break;
			}
        }
        else if ( n < 0 )
        {
			throw Exception("Login error\n");
        }

		sendto(sock, (const char*)&sendbuf, sizeof(sendbuf), 0, (const sockaddr*)&remote,sizeof(remote));
    }

	// 登录到服务端后，接收服务端发来的已经登录的用户的信息
	cout<<"Have "<<usercount<<" users logined server:"<<endl;
	for(int i = 0;i<usercount;i++)
	{
		stUserListNode *node = new stUserListNode;
		recvfrom(sock, (char*)node, sizeof(stUserListNode), 0, (sockaddr *)&remote, &fromlen);
		ClientList.push_back(node);
		cout<<"Username:"<<node->userName<<endl;
		in_addr tmp;
		tmp.S_un.S_addr = htonl(node->ip);
		cout<<"UserIP:"<<inet_ntoa(tmp)<<endl;
		cout<<"UserPort:"<<node->port<<endl;
		cout<<""<<endl;
	}
}

void OutputUsage()
{
	cout<<"You can input you command:\n"
		<<"Command Type:\"send\",\"tell\", \"exit\",\"getu\"\n"
		<<"Example : send Username Message\n"
		<<"Example : tell Username ip:port Message\n"
		<<"          exit\n"
		<<"          getu\n"
		<<endl;
}

/* 这是主要的函数：发送一个消息给某个用户(C)
 *流程：直接向某个用户的外网IP发送消息，如果此前没有联系过
 *      那么此消息将无法发送，发送端等待超时。
 *      超时后，发送端将发送一个请求信息到服务端，
 *      要求服务端发送给客户C一个请求，请求C给本机发送打洞消息
 *      以上流程将重复MAXRETRY次
 */
bool SendMessageTo(char *UserName, char *Message)
{
	char realmessage[256];
	unsigned int UserIP;
	unsigned short UserPort;
	bool FindUser = false;
	for(UserList::iterator UserIterator=ClientList.begin();
						UserIterator!=ClientList.end();
						++UserIterator)
	{
		if( strcmp( ((*UserIterator)->userName), UserName) == 0 )
		{
			UserIP = (*UserIterator)->ip;
			UserPort = (*UserIterator)->port;
			FindUser = true;
		}
	}

	if(!FindUser)
		return false;

	strcpy(realmessage, Message);
	for(int i=0;i<MAXRETRY;i++)
	{
		RecvedACK = false;

		sockaddr_in remote;
		remote.sin_addr.S_un.S_addr = htonl(UserIP);
		remote.sin_family = AF_INET;
		remote.sin_port = htons(UserPort);
		stP2PMessage MessageHead;
		MessageHead.iMessageType = P2PMESSAGE;
		MessageHead.iStringLen = (int)strlen(realmessage)+1;
		int isend = sendto(PrimaryUDP, (const char *)&MessageHead, sizeof(MessageHead), 0, (const sockaddr*)&remote, sizeof(remote));
		isend = sendto(PrimaryUDP, (const char *)&realmessage, MessageHead.iStringLen, 0, (const sockaddr*)&remote, sizeof(remote));
		
		// 等待接收线程将此标记修改
		for(int j=0;j<10;j++)
		{
			if(RecvedACK)
				return true;
			else
				Sleep(300);
		}

		// 没有接收到目标主机的回应，认为目标主机的端口映射没有
		// 打开，那么发送请求信息给服务器，要服务器告诉目标主机
		// 打开映射端口（UDP打洞）
		sockaddr_in server;
		server.sin_addr.S_un.S_addr = inet_addr(ServerIP);
		server.sin_family = AF_INET;
		server.sin_port = htons(g_nServerPort);
	
		stMessage transMessage;
		transMessage.iMessageType = P2PTRANS;
		strcpy(transMessage.message.translatemessage.userName, UserName);

		sendto(PrimaryUDP, (const char*)&transMessage, sizeof(transMessage), 0, (const sockaddr*)&server, sizeof(server));
		Sleep(100);// 等待对方先发送信息。
	}
	return false;
}

bool SendMessageTo2(char *UserName, char *Message, const char *pIP, USHORT nPort )
{
	char realmessage[256];
	unsigned int UserIP = 0L;
	unsigned short UserPort = 0;

	if ( pIP != NULL )
	{
		UserIP = ntohl( inet_addr( pIP ) );
		UserPort = nPort;
	}
	else
	{
		bool FindUser = false;
		for(UserList::iterator UserIterator=ClientList.begin();
							UserIterator!=ClientList.end();
							++UserIterator)
		{
			if( strcmp( ((*UserIterator)->userName), UserName) == 0 )
			{
				UserIP = (*UserIterator)->ip;
				UserPort = (*UserIterator)->port;
				FindUser = true;
			}
		}

		if(!FindUser)
			return false;
	}

	strcpy(realmessage, Message);

	sockaddr_in remote;
	remote.sin_addr.S_un.S_addr = htonl(UserIP);
	remote.sin_family = AF_INET;
	remote.sin_port = htons(UserPort);
	stP2PMessage MessageHead;
	MessageHead.iMessageType = P2PMESSAGE;
	MessageHead.iStringLen = (int)strlen(realmessage)+1;

	printf( "Send message, %s:%ld -> %s\n", inet_ntoa( remote.sin_addr ), ntohs( remote.sin_port ), realmessage );
	
	for(int i=0;i<MAXRETRY;i++)
	{
		RecvedACK = false;
		int isend = sendto(PrimaryUDP, (const char *)&MessageHead, sizeof(MessageHead), 0, (const sockaddr*)&remote, sizeof(remote));
		isend = sendto(PrimaryUDP, (const char *)&realmessage, MessageHead.iStringLen, 0, (const sockaddr*)&remote, sizeof(remote));
		
		// 等待接收线程将此标记修改
		for(int j=0;j<10;j++)
		{
			if(RecvedACK)
				return true;
			else
				Sleep(300);
		}
	}
	return false;
}

// 解析命令，暂时只有exit和send命令
// 新增getu命令，获取当前服务器的所有用户
void ParseCommand(char * CommandLine)
{
	if(strlen(CommandLine)<4)
		return;
	char Command[10];
	strncpy(Command, CommandLine, 4);
	Command[4]='\0';

	if(strcmp(Command,"exit")==0)
	{
		stMessage sendbuf;
		sendbuf.iMessageType = LOGOUT;
		strncpy(sendbuf.message.logoutmember.userName, UserName, 10);
		sockaddr_in server;
		server.sin_addr.S_un.S_addr = inet_addr(ServerIP);
		server.sin_family = AF_INET;
		server.sin_port = htons(g_nServerPort);

		sendto(PrimaryUDP,(const char*)&sendbuf, sizeof(sendbuf), 0, (const sockaddr *)&server, sizeof(server));
		shutdown(PrimaryUDP, 2);
		closesocket(PrimaryUDP);
		exit(0);
	}
	else if(strcmp(Command,"send")==0)
	{
		char sendname[20];
		char message[COMMANDMAXC];
		int i;
		for(i=5;;i++)
		{
			if(CommandLine[i]!=' ')
				sendname[i-5]=CommandLine[i];
			else
			{
				sendname[i-5]='\0';
				break;
			}
		}
		strcpy(message, &(CommandLine[i+1]));
		if(SendMessageTo(sendname, message))
			printf("Send OK!\n");
		else 
			printf("Send Failure!\n");
	}
	else if(strcmp(Command,"tell")==0)
	{
		char sendname[20];
		char sendto[ 64 ] = {0};
		char message[COMMANDMAXC];
		int i;
		for(i=5;;i++)
		{
			if(CommandLine[i]!=' ')
				sendname[i-5]=CommandLine[i];
			else
			{
				sendname[i-5]='\0';
				break;
			}
		}

		i++;
		int nStart = i;
		for(;;i++)
		{
			if(CommandLine[i]!=' ')
				sendto[i-nStart]=CommandLine[i];
			else
			{
				sendto[i-nStart]='\0';
				break;
			}
		}

		strcpy(message, &(CommandLine[i+1]));

		char szIP[32] = {0};
		char *p1 = sendto;
		char *p2 = szIP;
		while ( *p1 != ':' )
		{
			*p2++ = *p1++;	
		}

		p1++;
		USHORT nPort = atoi( p1 );

		if(SendMessageTo2(sendname, message, strcmp( szIP, "255.255.255.255" ) ? szIP : NULL, nPort ))
			printf("Send OK!\n");
		else 
			printf("Send Failure!\n");
	}
	else if(strcmp(Command,"getu")==0)
	{
		int command = GETALLUSER;
		sockaddr_in server;
		server.sin_addr.S_un.S_addr = inet_addr(ServerIP);
		server.sin_family = AF_INET;
		server.sin_port = htons(g_nServerPort);

		sendto(PrimaryUDP,(const char*)&command, sizeof(command), 0, (const sockaddr *)&server, sizeof(server));
	}
}

// 接受消息线程
DWORD WINAPI RecvThreadProc(LPVOID lpParameter)
{
	sockaddr_in remote;
	int sinlen = sizeof(remote);
	stP2PMessage recvbuf;
	for(;;)
	{
		int iread = recvfrom(PrimaryUDP, (char *)&recvbuf, sizeof(recvbuf), 0, (sockaddr *)&remote, &sinlen);
		if(iread<=0)
		{
			printf("recv error\n");
			continue;
		}
		switch(recvbuf.iMessageType)
		{
		case P2PMESSAGE:
			{
				// 接收到P2P的消息
				char *comemessage= new char[recvbuf.iStringLen];
				int iread1 = recvfrom(PrimaryUDP, comemessage, 256, 0, (sockaddr *)&remote, &sinlen);
				comemessage[iread1-1] = '\0';
				if(iread1<=0)
					throw Exception("Recv Message Error\n");
				else
				{
					printf("Recv a Message, %s:%ld -> %s\n", inet_ntoa( remote.sin_addr), htons(remote.sin_port), comemessage);
					
					stP2PMessage sendbuf;
					sendbuf.iMessageType = P2PMESSAGEACK;
					sendto(PrimaryUDP, (const char*)&sendbuf, sizeof(sendbuf), 0, (const sockaddr*)&remote, sizeof(remote));
					printf("Send a Message Ack to %s:%ld\n", inet_ntoa( remote.sin_addr), htons(remote.sin_port) );
				}

				delete []comemessage;
				break;

			}
		case P2PSOMEONEWANTTOCALLYOU:
			{
				// 接收到打洞命令，向指定的IP地址打洞
				printf("Recv p2someonewanttocallyou from %s:%ld\n", inet_ntoa( remote.sin_addr), htons(remote.sin_port) );

				sockaddr_in remote;
				remote.sin_addr.S_un.S_addr = htonl(recvbuf.iStringLen);
				remote.sin_family = AF_INET;
				remote.sin_port = htons(recvbuf.Port);

				// UDP hole punching
				stP2PMessage message;
				message.iMessageType = P2PTRASH;
				sendto(PrimaryUDP, (const char *)&message, sizeof(message), 0, (const sockaddr*)&remote, sizeof(remote));
	
				printf("Send p2ptrash to %s:%ld\n", inet_ntoa( remote.sin_addr), htons(remote.sin_port) );
                
				break;
			}
		case P2PMESSAGEACK:
			{
				// 发送消息的应答
				RecvedACK = true;
				printf("Recv message ack from %s:%ld\n", inet_ntoa( remote.sin_addr), htons(remote.sin_port) );

				break;
			}
		case P2PTRASH:
			{
				// 对方发送的打洞消息，忽略掉。
				//do nothing ...
				printf("Recv p2ptrash data from %s:%ld\n", inet_ntoa( remote.sin_addr), htons(remote.sin_port) );

				break;
			}
		case GETALLUSER:
			{
				int usercount;
				int fromlen = sizeof(remote);
				int iread = recvfrom(PrimaryUDP, (char *)&usercount, sizeof(int), 0, (sockaddr *)&remote, &fromlen);
				if(iread<=0)
				{
					throw Exception("Login error\n");
				}
				
				ClientList.clear();

				cout<<"Have "<<usercount<<" users logined server:"<<endl;
				for(int i = 0;i<usercount;i++)
				{
					stUserListNode *node = new stUserListNode;
					recvfrom(PrimaryUDP, (char*)node, sizeof(stUserListNode), 0, (sockaddr *)&remote, &fromlen);
					ClientList.push_back(node);
					cout<<"Username:"<<node->userName<<endl;
					in_addr tmp;
					tmp.S_un.S_addr = htonl(node->ip);
					cout<<"UserIP:"<<inet_ntoa(tmp)<<endl;
					cout<<"UserPort:"<<node->port<<endl;
					cout<<""<<endl;
				}
				break;
			}
		}
	}
}

int testNATProp()
{
	try
	{
		InitWinSock();
		
		PrimaryUDP = mksock(SOCK_DGRAM);
		BindSock(PrimaryUDP);

		char szServerIP1[ 32 ] = {0};
		char szServerIP2[ 32 ] = {0};

		cout<<"Please input server1 ip:";
		cin>>szServerIP1;

		cout<<"Please input server2 ip:";
		cin>>szServerIP2;

		sockaddr_in remote1;
		remote1.sin_addr.S_un.S_addr = inet_addr(szServerIP1);
		remote1.sin_family = AF_INET;
		remote1.sin_port = htons(g_nServerPort);
		
		sockaddr_in remote2;
		remote2.sin_addr.S_un.S_addr = inet_addr(szServerIP2);
		remote2.sin_family = AF_INET;
		remote2.sin_port = htons(g_nServerPort);

		char chData = 'A';
		int nCount = 0;

		for(;;)
		{
			nCount++;
			printf( "send message to: %s:%ld, %ld\n", szServerIP1, g_nServerPort, nCount );

			sendto(PrimaryUDP, (const char*)&chData, sizeof(char), 0, (const sockaddr*)&remote1,sizeof(remote1));

			if ( szServerIP2[ 0 ] != 'x' )
			{
				printf( "send message to: %s:%ld, %ld\n", szServerIP2, g_nServerPort, nCount );
				sendto(PrimaryUDP, (const char*)&chData, sizeof(char), 0, (const sockaddr*)&remote2,sizeof(remote2));
			}

			Sleep( 5000 );
		}
	}
	catch(Exception &e)
	{
		printf(e.GetMessage());
		return 1;
	}
	return 0;
}


int main(int argc, char* argv[])
{
//	testNATProp();
// 	return 0;

	if ( argc > 1 )
	{
		g_nClientPort = atoi( argv[ 1 ] );
	}

	if ( argc > 2 )
	{
		g_nServerPort = atoi( argv[ 2 ] );
	}

	try
	{
		InitWinSock();
		
		PrimaryUDP = mksock(SOCK_DGRAM);
		BindSock(PrimaryUDP);

		cout<<"Please input server ip:";
		cin>>ServerIP;

		cout<<"Please input your name:";
		cin>>UserName;

		ConnectToServer(PrimaryUDP, UserName, ServerIP);

		HANDLE threadhandle = CreateThread(NULL, 0, RecvThreadProc, NULL, NULL, NULL);
		CloseHandle(threadhandle);
		OutputUsage();

		for(;;)
		{
			char Command[COMMANDMAXC];
			cin >> Command;
			ParseCommand(Command);
		}
	}
	catch(Exception &e)
	{
		printf(e.GetMessage());
		return 1;
	}
	return 0;
}

