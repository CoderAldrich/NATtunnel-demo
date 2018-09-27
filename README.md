# NATtunnel-demo
内网穿透demo，放这里备份

P2P之UDP穿透NAT的原理与实现 - 增强篇（附修改过的源代码）

关键词: P2P UDP NAT 原理 穿透 Traveral Symmetric Cone
原始作者: Hwycheng Leo(FlashBT@Hotmail.com)
源码下载: http://bbs.hwysoft.com/download/UDP-NAT-LEO.rar
参考：http://midcom-p2p.sourceforge.net/draft-ford-midcom-p2p-01.txt
      P2P之UDP穿透NAT的原理与实现(shootingstars)

文章说明:

关于UDP穿透NAT的中文资料在网络上是很少的，仅有<<P2P之UDP穿透NAT的原理与实现(shootingstars)>>这篇文章有实际的参考价值。
本人近两年来也一直从事P2P方面的开发工作，比较有代表性的是个人开发的BitTorrent下载软件 - FlashBT(变态快车). 对P2P下载
或者P2P的开发感兴趣的朋友可以访问软件的官方主页: http://www.hwysoft.com/chs/ 下载看看，说不定有收获。写这篇文章的主要
目的是懒的再每次单独回答一些网友的提问, 一次性写下来, 即节省了自己的时间，也方便了对于P2P的UDP穿透感兴趣的网友阅读和
理解。对此有兴趣和经验的朋友可以给我发邮件或者访问我的个人Blog留言: http://hwycheng.blogchina.com. 
您可以自由转载此篇文章，但是请保留此说明。

再次感谢shootingstars网友的早期贡献. 表示谢意。

----------------------------------------------------------------------------------------------------------------------------

NAT(The IP Network Address Translator) 的概念和意义是什么?

NAT, 中文翻译为网络地址转换。具体的详细信息可以访问RFC 1631 - http://www.faqs.org/rfcs/rfc1631.html, 这是对于NAT的定义和解释的最权威的描述。网络术语都是很抽象和艰涩的，除非是专业人士，否则很难从字面中来准确理解NAT的含义。

要想完全明白NAT 的作用，我们必须理解IP地址的两大分类，一类是私有IP地址，在这里我们称作内网IP地址。一类是非私有的IP地址，在这里我们称作公网IP地址。关于IP地址的概念和作用的介绍参见我的另一篇文章: http://hwycheng.blogchina.com/2402121.html

内网IP地址: 是指使用A/B/C类中的私有地址, 分配的IP地址在全球不惧有唯一性，也因此无法被其它外网主机直接访问。
公网IP地址: 是指具有全球唯一的IP地址，能够直接被其它主机访问的。

NAT 最初的目的是为使用内网IP地址的计算机提供通过少数几台具有公网的IP地址的计算机访问外部网络的功能。NAT 负责将某些内网IP地址的计算机向外部网络发出的IP数据包的源IP地址转换为NAT自己的公网的IP地址，目的IP地址不变, 并将IP数据包转发给路由器，最终到达外部的计算机。同时负责将外部的计算机返回的IP数据包的目的IP地址转换为内网的IP地址，源IP地址不变，并最终送达到内网中的计算机。
                                                 
	----------------------                           ----------------------               
	| 192.168.0.5        |  Internat host            | 192.168.0.6        |  Internat host
	----------------------                           ----------------------               
	        ^ port:2809                                      ^port: 1827                            
	        |                                                |                            
	        V                                                V                            
	----------------------                           ----------------------               
	| 192.168.0.1        | NAT device                | 192.168.0.2        | NAT device    
	| 61.51.99.86        |                           | 61.51.77.66        |               
	----------------------                           ----------------------               
	        ^                                                ^                            
	        |                                                |                            
	        V port:80                                        V port: 80                           
	----------------------                           ----------------------               
	| 61.51.202.88       | Internet host             | 61.51.76.102       | Internet host 
	----------------------                           ----------------------               
                                                            
                              图一: NAT 实现了私有IP的计算机分享几个公网IP地址访问Internet的功能。
                              
随着网络的普及，IPv4的局限性暴露出来。公网IP地址成为一种稀缺的资源，此时NAT 的功能局限也暴露出来，同一个公网的IP地址，某个时间只能由一台私有IP地址的计算机使用。于是NAPT(The IP Network Address/Port Translator)应运而生，NAPT实现了多台私有IP地址的计算机可以同时通过一个公网IP地址来访问Internet的功能。这在很大程度上暂时缓解了IPv4地址资源的紧张。

NAPT 负责将某些内网IP地址的计算机向外部网络发出的TCP/UDP数据包的源IP地址转换为NAPT自己的公网的IP地址，源端口转为NAPT自己的一个端口。目的IP地址和端口不变, 并将IP数据包发给路由器，最终到达外部的计算机。同时负责将外部的计算机返回的IP数据包的目的IP地址转换内网的IP地址，目的端口转为内网计算机的端口，源IP地址和源端口不变，并最终送达到内网中的计算机。

                                                 
		----------------------                           ----------------------               
		| 192.168.0.5        |  Internat host            | 192.168.0.6        |  Internat host
		----------------------                           ----------------------               
			port: 2809	^                   ^ port: 1827
					 \                 /
					  v               v				
					----------------------            
					| 192.168.0.1        | NAT device 
					| 61.51.99.86        |            
					---------------------- 					
	map port:9882 to 192.168.0.5:2809 ^              ^ map port: 9881 to 192.168.0.6:1827
					 /                \
			     port:80	v                  v	port:80				
		----------------------                           ----------------------               
		| 61.51.202.88       | Internet host             | 61.51.76.102       | Internet host 
		----------------------                           ----------------------   				
				
                              图二: NAPT 实现了私有IP的计算机分享一个公网IP地址访问Internet的功能。						
 
在我们的工作和生活中, NAPT的作用随处可见，绝大部分公司的网络架构，都是通过1至N台支持NAPT的路由器来实现公司的所有计算机连接外部的Internet网络的。包括本人在写这篇文章的时候，也是在家中使用一台IBM笔记本通过一台宽带连接的台式机来访问Internet的。我们本篇文章主要讨论的NAPT的问题。

NAPT(The IP Network Address/Port Translator) 为何阻碍了P2P软件的应用?

通过NAPT 上网的特点决定了只能由NAPT内的计算机主动向NAPT外部的主机发起连接，外部的主机想直接和NAPT内的计算机直接建立连接是不被允许的。IM(即时通讯)而言，这意味着由于NAPT内的计算机和NAPT外的计算机只能通过服务器中转数据来进行通讯。对于P2P方式的下载程序而言，意味着NAPT内的计算机不能接收到NAPT外部的连接，导致连接数用过少，下载速度很难上去。因此P2P软件必须要解决的一个问题就是要能够在一定的程度上解决NAPT内的计算机不能被外部连接的问题。

NAT(The IP Network Address Translator) 进行UDP穿透的原理是什么?

TCP/IP传输时主要用到TCP和UDP协议。TCP协议是可靠的，面向连接的传输协议。UDP是不可靠的，无连接的协议。根据TCP和UDP协议的实现原理，对于NAPT来进行穿透，主要是指的UDP协议。TCP协议也有可能，但是可行性非常小，要求更高，我们此处不作讨论，如果感兴趣可以到Google上搜索，有些文章对这个问题做了探讨性的描述。下面我们来看看利用UDP协议来穿透NAPT的原理是什么:

			----------------------                           ----------------------               
			| 192.168.0.5        |  Internat host            | 192.168.0.6        |  Internat host
			----------------------                           ----------------------               
			  UDP port: 2809	^                   ^ UDP port: 1827
						 \                 /
						  v               v				
						----------------------            
						| 192.168.0.1        | NAT device 
						| 61.51.99.86        |            
						---------------------- 					
  Session(192.168.0.6:1827 <-> 61.51.76.102:8098) ^              ^ Session(192.168.0.6:1827 <-> 61.51.76.102:8098)
               map port:9882 to 192.168.0.5:2809 /                \map port: 9881 to 192.168.0.6:1827
				  UDP port:8098	v                  v	UDP port:8098				
			----------------------                           ----------------------               
			| 61.51.202.88       | Internet host             | 61.51.76.102       | Internet host 
			----------------------                           ---------------------- 		
							
					
		                      图三: NAPT 是如何将私有IP地址的UDP数据包与公网主机进行透明传输的。

UDP协议包经NAPT透明传输的说明:

NAPT为每一个Session分配一个NAPT自己的端口号，依据此端口号来判断将收到的公网IP主机返回的TCP/IP数据包转发给那台内网IP地址的计算机。在这里Session是虚拟的，UDP通讯并不需要建立连接，但是对于NAPT而言，的确要有一个Session的概念存在。NAPT对于UDP协议包的透明传输面临的一个重要的问题就是如何处理这个虚拟的Session。我们都知道TCP连接的Session以SYN包开始，以FIN包结束，NAPT可以很容易的获取到TCP Session的生命周期，并进行处理。但是对于UDP而言，就麻烦了，NAPT并不知道转发出去的UDP协议包是否到达了目的主机，也没有办法知道。而且鉴于UDP协议的特点，可靠很差，因此NAPT必须强制维持Session的存在，以便等待将外部送回来的数据并转发给曾经发起请求的内网IP地址的计算机。NAPT具体如何处理UDP Session的超时呢？不同的厂商提供的设备对于NAPT的实现不近相同，也许几分钟，也许几个小时，些NAPT的实现还会根据设备的忙碌状态进行智能计算超时时间的长短。

		  [192.168.0.6:1827]
                            | UDP Packet[src ip:192.168.0.6 src port:1827 dst ip:61.51.76.102 dst port 8098]
                            v
	[pub ip: 61.51.99.86]NAT[priv ip: 192.168.0.1]
                            | UDP Packet[src ip:61.51.99.86 src port:9881 dst ip:61.51.76.102 dst port 8098]
                            v			
		  [61.51.76.102:8098]
		  
		  		    图四: NAPT 将内部发出的UDP协议包的源地址和源端口改变传输给公网IP主机。
		  		    
		  		    
		  [192.168.0.6:1827]
		            ^
                            | UDP Packet[src ip:61.51.76.102 src port:8098 dst ip:192.168.0.6 dst port 1827]
	[pub ip: 61.51.99.86]NAT[priv ip: 192.168.0.1]
		            ^	
                            | UDP Packet[src ip:61.51.76.102 src port:8098 dst ip:61.51.99.86 dst port 9881]	
		  [61.51.76.102:8098]
		  
		  		    图五: NAPT 将收到的公网IP主机返回的UDP协议包的目的地址和目的端口改变传输给内网IP计算机。		  		    
现在我们大概明白了NAPT如何实现内网计算机和外网主机间的透明通讯。现在来看一下我们最关心的问题，就是NAPT是依据什么策略来判断是否要为一个请求发出的UDP数据包建立Session的呢？主要有一下几个策略: 

A. 源地址(内网IP地址)不同，忽略其它因素, 在NAPT上肯定对应不同的Session
B. 源地址(内网IP地址)相同，源端口不同，忽略其它的因素，则在NAPT上也肯定对应不同的Session
C. 源地址(内网IP地址)相同，源端口相同，目的地址(公网IP地址)相同，目的端口不同，则在NAPT上肯定对应同一个Session
D. 源地址(内网IP地址)相同，源端口相同，目的地址(公网IP地址)不同，忽略目的端口，则在NAPT上是如何处理Session的呢？

D的情况正式我们关心和要讨论的问题。依据目的地址(公网IP地址)对于Session的建立的决定方式我们将NAPT设备划分为两大类:

Symmetric NAPT:
对于到同一个IP地址，任意端口的连接分配使用同一个Session; 对于到不同的IP地址, 任意端口的连接使用不同的Session. 
我们称此种NAPT为 Symmetric NAPT. 也就是只要本地绑定的UDP端口相同， 发出的目的IP地址不同，则会建立不同的Session.

	[202.223.98.78:9696] [202.223.98.78:9696] [202.223.98.78:9696]
		^		^			^
		|		|			|
		v		v			v
	       9883	       9882		       9881
		                 |
			     \ [NAT] /
			         ^
			         |
			         v			  
			  [192.168.0.6:1827]
			  
			  图六: Symmetric 的英文意思是对称。多个端口对应多个主机，平行的，对称的!
		  
Cone NAPT:
对于到同一个IP地址，任意端口的连接分配使用同一个Session; 对于到不同的IP地址，任意端口的连接也使用同一个Session.
我们称此种NAPT为 Cone NAPT. 也就是只要本地绑定的UDP端口相同， 发出的目的地址不管是否相同， 都使用同一个Session.

	[202.223.98.78:9696] [202.223.98.78:9696] [202.223.98.78:9696]

			^	   ^	     ^
			 \	   |	    /
			  v	   v	   v
			         9881
                                 [NAT]
				   ^
				   |
				   v			  
			  [192.168.0.6:1827]
			  
			  图七: Cone 的英文意思是锥。一个端口对应多个主机，是不是像个锥子?

现在绝大多数的NAPT属于后者，即Cone NAT。本人在测试的过程中，只好使用了一台日本的Symmetric NAT。还好不是自己的买的，我从不买日货, 希望看这篇文章的朋友也自觉的不要购买日本的东西。Win9x/2K/XP/2003系统自带的NAPT也是属于 Cone NAT的。这是值的庆幸的，因为我们要做的UDP穿透只能在Cone NAT间进行，只要有一台不是Cone NAT，对不起，UDP穿透没有希望了，服务器转发吧。后面会做详细分析!

下面我们再来分析一下NAPT 工作时的一些数据结构，在这里我们将真正说明UDP可以穿透Cone NAT的依据。这里描述的数据结构只是为了说明原理，不具有实际参考价值，真正感兴趣可以阅读Linux的中关于NAT实现部分的源码。真正的NAT实现也没有利用数据库的，呵呵，为了速度！

Symmetric NAPT 工作时的端口映射数据结构如下:

内网信息表:

[NAPT 分配端口] [ 内网IP地址 ] [ 内网端口 ] [ 外网IP地址 ] [ SessionTime 开始时间 ]

PRIMARY KEY( [NAPT 分配端口] ) -> 表示依据[NAPT 分配端口]建立主键，必须唯一且建立索引，加快查找.
UNIQUE( [ 内网IP地址 ], [ 内网端口 ] ) -> 表示这两个字段联合起来不能重复.
UNIQUE( [ 内网IP地址 ], [ 内网端口 ], [ 外网IP地址 ] ) -> 表示这三个字段联合起来不能重复.

映射表:

[NAPT 分配端口] [ 外网端口 ]

UNIQUE( [NAPT 分配端口], [ 外网端口 ] ) -> 表示这两个字段联合起来不能重复.

Cone NAPT 工作时的端口映射数据结构如下:

内网信息表:

[NAPT 分配端口] [ 内网IP地址 ] [ 内网端口 ] [ SessionTime 开始时间 ]

PRIMARY KEY( [NAPT 分配端口] ) -> 表示依据[NAPT 分配端口]建立主键，必须唯一且建立索引，加快查找.
UNIQUE( [ 内网IP地址 ], [ 内网端口 ] ) -> 表示这两个字段联合起来不能重复.

外网信息表:

[ wid 主键标识 ] [ 外网IP地址 ] [ 外网端口 ]

PRIMARY KEY( [ wid 主键标识 ] ) -> 表示依据[ wid 主键标识 ]建立主键，必须唯一且建立索引，加快查找.
UNIQUE( [ 外网IP地址 ], [ 外网端口 ] ) -> 表示这两个字段联合起来不能重复.

映射表: 实现一对多，的

[NAPT 分配端口] [ wid 主键标识 ]

UNIQUE( [NAPT 分配端口], [ wid 主键标识 ] ) -> 表示这两个字段联合起来不能重复.
UNIQUE( [ wid 主键标识 ] ) -> 标识此字段不能重复.

看完了上面的数据结构是更明白了还是更晕了？ 呵呵! 多想一会儿就会明白了。通过NAT,内网计算机计算机向外连结是很容易的，NAPT会自动处理，我们的应用程序根本不必关心它是如何处理的。那么外部的计算机想访问内网中的计算机如何实现呢？我们来看一下下面的流程：

c 是一台在NAPT后面的内网计算机，s是一台有外网IP地址的计算机。c 主动向 s 发起连接请求，NAPT依据上面描述的规则在自己的数据结构中记录下来，建立一个Session. 然后 c 和 s 之间就可以实现双向的透明的数据传输了。如下面所示:

   c[192.168.0.6:1827] <-> [priv ip: 192.168.0.1]NAPT[pub ip: 61.51.99.86:9881] <-> s[61.51.76.102:8098]

由此可见，一台外网IP地址的计算机想和NAPT后面的内网计算机通讯的条件就是要求NAPT后面的内网计算机主动向外网IP地址的计算机发起一个UDP数据包。外网IP地址的计算机利用收到的UDP数据包获取到NAPT的外网IP地址和映射的端口，以后就可以和内网IP的计算机透明的进行通讯了。
    
现在我们再来分析一下我们最关心的两个NAPT后面的内网计算机如何实现直接通讯呢? 两者都无法主动发出连接请求，谁也不知道对方的NAPT的公网IP地址和NAPT上面映射的端口号。所以我们要靠一个公网IP地址的服务器帮助两者来建立连接。当两个NAPT后面的内网计算机分别连接了公网IP地址的服务器后，服务器可以从收到的UDP数据包中获取到这两个NAPT设备的公网IP地址和这两个连接建立的Session的映射端口。
两个内网计算机可以从服务器上获取到对方的NAPT设备公网IP地址和映射的端口了。

我们假设两个内网计算机分别为A和B，对应的NAPT分别为AN和BN， 如果A在获取到B对应的BN的IP地址和映射的端口后，迫不急待的向这个IP
地址和映射的端口发送了个UDP数据包，会有什么情况发生呢？依据上面的原理和数据结构我们会知道，AN会在自己的数据结构中生成一条记录，标识一个新Session的存在。BN在收到数据包后，从自己的数据结构中查询，没有找到相关记录，因此将包丢弃。B是个慢性子，此时才慢吞吞的向着AN的IP地址和映射的端口发送了一个UDP数据包，结果如何呢？当然是我们期望的结构了，AN在收到数据包后，从自己的数据结构中查找到了记录，所以将数据包进行处理发送给了A。A 再次向B发送数据包时，一切都时畅通无阻了。OK, 大工告成！且慢，这时对于Cone NAPT而言，对于Symmetric NAPT呢？呵呵，自己分析一下吧...

NAPT(The IP Network Address/Port Translator) 进行UDP穿透的具体情况分析!

首先明确的将NAPT设备按照上面的说明分为: Symmetric NAPT 和 Cone NAPT, Cone NAPT 是我们需要的。Win9x/2K/XP/2003 自带的NAPT也为Cone NAPT。

第一种情况, 双方都是Symmetric NAPT:

此情况应给不存在什么问题，肯定是不支持UDP穿透。

第二种情况, 双方都是Cone NAPT:

此情况是我们需要的，可以进行UDP穿透。

第三种情况, 一个是Symmetric NAPT, 一个是Cone NAPT:

此情况比较复杂，但我们按照上面的描述和数据机构进行一下分析也很容易就会明白了, 分析如下,

假设: A -> Symmetric NAT, B -> Cone NAT

1. A 想连接 B, A 从服务器那儿获取到 B 的NAT地址和映射端口, A 通知服务器，服务器告知 B A的NAT地址和映射端口, B 向 A 发起连接，A 肯定无法接收到。此时 A 向 B 发起连接， A 对应的NAT建立了一个新的Session，分配了一个新的映射端口， B 的 NAT 接收到UDP包后，在自己的映射表中查询，无法找到映射项，因此将包丢弃了。

2. B 想连接 A, B 从服务器那儿获取到 A 的NAT地址和映射端口, B 通知服务器, 服务器告知 A B的NAT地址和映射端口,A 向 B 发起连接, A 对应的NAT建立了一个新的Session，分配了一个新的映射端口B肯定无法接收到。此时 B 向 A 发起连接, 由于 B 无法获取 A 建立的新的Session的映射端口，仍是使用服务器上获取的映射端口进行连接， 因此 A 的NAT在接收到UDP包后，在自己的映射表中查询，无法找到映射项, 因此将包丢弃了。

根据以上分析，只有当连接的两端的NAT都为Cone NAT的情况下，才能进行UDP的内网穿透互联。


NAPT(The IP Network Address/Port Translator) 进行UDP穿透如何进行现实的验证和分析!

需要的网络结构如下:

三个NAT后面的内网机器，两个外网服务器。其中两台Cone NAPT，一台 Symmetric NAPT。

验证方法:

可以使用本程序提供的源码，编译，然后分别运行服务器程序和客户端。修改过后的源码增加了客户端之间直接通过IP地址和端口发送消息的命令，利用此命令，你可以手动的验证NAPT的穿透情况。为了方便操作，推荐你使用一个远程登陆软件，可以直接在一台机器上操作所有的相关的计算机，这样很方便，一个人就可以完成所有的工作了。呵呵，本人就是这么完成的。欢迎有兴趣和经验的朋友来信批评指正，共同进步。
