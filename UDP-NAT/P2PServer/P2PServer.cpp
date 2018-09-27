/* P2P 程序服务端
 * 
 * 文件名：P2PServer.c
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

#pragma comment(lib, "ws2_32.lib")

#include "windows.h"
#include "..\proto.h"
#include "..\Exception.h"

USHORT g_nServerPort = SERVER_PORT;

UserList ClientList;

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

int testNATProp()
{
	try{
		InitWinSock();
		
		SOCKET PrimaryUDP;
		PrimaryUDP = mksock(SOCK_DGRAM);

		sockaddr_in local;
		local.sin_family=AF_INET;
		local.sin_port= htons(g_nServerPort); 
		local.sin_addr.s_addr = htonl(INADDR_ANY);
		int nResult=bind(PrimaryUDP,(sockaddr*)&local,sizeof(sockaddr));
		if(nResult==SOCKET_ERROR)
			throw Exception("bind error");

		sockaddr_in sender;
		char recvbuf;
		memset(&recvbuf,0,sizeof(char));

		// 开始主循环.
		// 主循环负责下面几件事情:
		// 一:读取客户端登陆和登出消息,记录客户列表
		// 二:转发客户p2p请求
		for(;;)
		{
			int dwSender = sizeof(sender);
			int ret = recvfrom(PrimaryUDP, (char *)&recvbuf, sizeof(char), 0, (sockaddr *)&sender, &dwSender);
			if(ret <= 0)
			{
				printf("recv error");
				continue;
			}


			//  将这个用户的信息记录到用户列表中
			printf("Receive message from: %s:%ld\n", inet_ntoa( sender.sin_addr ), ntohs(sender.sin_port) );
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
		g_nServerPort = atoi( argv[ 1 ] );
	}

	try{
		InitWinSock();
		
		SOCKET PrimaryUDP;
		PrimaryUDP = mksock(SOCK_DGRAM);

		sockaddr_in local;
		local.sin_family=AF_INET;
		local.sin_port= htons(g_nServerPort); 
		local.sin_addr.s_addr = htonl(INADDR_ANY);
		int nResult=bind(PrimaryUDP,(sockaddr*)&local,sizeof(sockaddr));
		if(nResult==SOCKET_ERROR)
			throw Exception("bind error");

		sockaddr_in sender;
		stMessage recvbuf;
		memset(&recvbuf,0,sizeof(stMessage));

		// 开始主循环.
		// 主循环负责下面几件事情:
		// 一:读取客户端登陆和登出消息,记录客户列表
		// 二:转发客户p2p请求
		for(;;)
		{
			int dwSender = sizeof(sender);
			int ret = recvfrom(PrimaryUDP, (char *)&recvbuf, sizeof(stMessage), 0, (sockaddr *)&sender, &dwSender);
			if(ret <= 0)
			{
				printf("recv error");
				continue;
			}
			else
			{
				int messageType = recvbuf.iMessageType;
				switch(messageType){
				case LOGIN:
					{
						//  将这个用户的信息记录到用户列表中
						stUserListNode *currentuser = new stUserListNode();
						strcpy(currentuser->userName, recvbuf.message.loginmember.userName);
						currentuser->ip = ntohl(sender.sin_addr.S_un.S_addr);
						currentuser->port = ntohs(sender.sin_port);

						BOOL bFound = FALSE;
						for(UserList::iterator UserIterator=ClientList.begin();
							UserIterator!=ClientList.end();
							++UserIterator)
						{
							if( strcmp( ((*UserIterator)->userName), recvbuf.message.loginmember.userName) == 0 )
							{
								bFound = TRUE;
								break;
							}
						}
						
						if ( !bFound )
						{
							printf("has a user login : %s <-> %s:%ld\n", recvbuf.message.loginmember.userName, inet_ntoa( sender.sin_addr ), ntohs(sender.sin_port) );
							ClientList.push_back(currentuser);
						}
						
						// 发送已经登陆的客户信息
						int nodecount = (int)ClientList.size();
						sendto(PrimaryUDP, (const char*)&nodecount, sizeof(int), 0, (const sockaddr*)&sender, sizeof(sender));
						for(UserList::iterator UserIterator=ClientList.begin();
								UserIterator!=ClientList.end();
								++UserIterator)
						{
							sendto(PrimaryUDP, (const char*)(*UserIterator), sizeof(stUserListNode), 0, (const sockaddr*)&sender, sizeof(sender)); 
						}
								
						printf("send user list information to: %s <-> %s:%ld\n", recvbuf.message.loginmember.userName, inet_ntoa( sender.sin_addr ), ntohs(sender.sin_port) );

						break;
					}
				case LOGOUT:
					{
						// 将此客户信息删除
						printf("has a user logout : %s <-> %s:%ld\n", recvbuf.message.logoutmember.userName, inet_ntoa( sender.sin_addr ), ntohs(sender.sin_port) );
						UserList::iterator removeiterator;
						for(UserList::iterator UserIterator=ClientList.begin();
							UserIterator!=ClientList.end();
							++UserIterator)
						{
							if( strcmp( ((*UserIterator)->userName), recvbuf.message.logoutmember.userName) == 0 )
							{
								removeiterator = UserIterator;
								break;
							}
						}
						ClientList.remove(*removeiterator);
						break;
					}
				case P2PTRANS:
					{
						// 某个客户希望服务端向另外一个客户发送一个打洞消息
						printf("%s:%ld wants to p2p %s\n",inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), recvbuf.message.translatemessage.userName );
						stUserListNode node = GetUser(recvbuf.message.translatemessage.userName);
						sockaddr_in remote;
						remote.sin_family=AF_INET;
						remote.sin_port= htons(node.port); 
						remote.sin_addr.s_addr = htonl(node.ip);

						in_addr tmp;
						tmp.S_un.S_addr = htonl(node.ip);

						stP2PMessage transMessage;
						transMessage.iMessageType = P2PSOMEONEWANTTOCALLYOU;
						transMessage.iStringLen = ntohl(sender.sin_addr.S_un.S_addr);
						transMessage.Port = ntohs(sender.sin_port);
                        
						sendto(PrimaryUDP,(const char*)&transMessage, sizeof(transMessage), 0, (const sockaddr *)&remote, sizeof(remote));
						printf( "tell %s <-> %s:%d to send p2ptrans message to: %s:%ld\n", 
							recvbuf.message.translatemessage.userName, inet_ntoa(remote.sin_addr), node.port, inet_ntoa(sender.sin_addr), ntohs(sender.sin_port) );

						break;
					}
				
				case GETALLUSER:
					{
						int command = GETALLUSER;
						sendto(PrimaryUDP, (const char*)&command, sizeof(int), 0, (const sockaddr*)&sender, sizeof(sender));

						int nodecount = (int)ClientList.size();
						sendto(PrimaryUDP, (const char*)&nodecount, sizeof(int), 0, (const sockaddr*)&sender, sizeof(sender));

						for(UserList::iterator UserIterator=ClientList.begin();
								UserIterator!=ClientList.end();
								++UserIterator)
						{
							sendto(PrimaryUDP, (const char*)(*UserIterator), sizeof(stUserListNode), 0, (const sockaddr*)&sender, sizeof(sender)); 
						}

						printf("send user list information to: %s <-> %s:%ld\n", recvbuf.message.loginmember.userName, inet_ntoa( sender.sin_addr ), ntohs(sender.sin_port) );

						break;
					}
				}
			}
		}

	}
	catch(Exception &e)
	{
		printf(e.GetMessage());
		return 1;
	}

	return 0;
}

