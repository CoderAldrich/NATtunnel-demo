/* 异常类
 *
 * 文件名：Exception.h
 *
 * 日期：2004.5.5
 *
 * 作者：shootingstars(zhouhuis22@sina.com)
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

#ifndef __HZH_Exception__
#define __HZH_Exception__

#define EXCEPTION_MESSAGE_MAXLEN 256
#include "string.h"

class Exception
{
private:
	char m_ExceptionMessage[EXCEPTION_MESSAGE_MAXLEN];
public:
	Exception(char *msg)
	{
		strncpy(m_ExceptionMessage, msg, EXCEPTION_MESSAGE_MAXLEN);
	}

	char *GetMessage()
	{
		return m_ExceptionMessage;
	}
};



#endif