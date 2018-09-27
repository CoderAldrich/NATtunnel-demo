/* P2P 程序传输协议
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

#pragma once
#include <list>

// 定义iMessageType的值
#define LOGIN 1
#define LOGOUT 2
#define P2PTRANS 3
#define GETALLUSER  4

// 服务器端口
#define SERVER_PORT 6060

// Client登录时向服务器发送的消息
struct stLoginMessage
{
	char userName[10];
	char password[10];
};

// Client注销时发送的消息
struct stLogoutMessage
{
	char userName[10];
};

// Client向服务器请求另外一个Client(userName)向自己方向发送UDP打洞消息
struct stP2PTranslate
{
	char userName[10];
};

// Client向服务器发送的消息格式
struct stMessage
{
	int iMessageType;
	union _message
	{
		stLoginMessage loginmember;
		stLogoutMessage logoutmember;
		stP2PTranslate translatemessage;
	}message;
};

// 客户节点信息
struct stUserListNode
{
	char userName[10];
	unsigned int ip;
	unsigned short port;
};

// Server向Client发送的消息
struct stServerToClient
{
	int iMessageType;
	union _message
	{
		stUserListNode user;
	}message;

};

//======================================
// 下面的协议用于客户端之间的通信
//======================================
#define P2PMESSAGE 100               // 发送消息
#define P2PMESSAGEACK 101            // 收到消息的应答
#define P2PSOMEONEWANTTOCALLYOU 102  // 服务器向客户端发送的消息
                                     // 希望此客户端发送一个UDP打洞包
#define P2PTRASH        103          // 客户端发送的打洞包，接收端应该忽略此消息

// 客户端之间发送消息格式
struct stP2PMessage
{
	int iMessageType;
	int iStringLen;         // or IP address
	unsigned short Port; 
};

using namespace std;
typedef list<stUserListNode *> UserList;

