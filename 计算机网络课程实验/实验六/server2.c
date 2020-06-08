//#pragma pack(1)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/time.h>
#include <memory.h>
#include <assert.h>
#include <time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#define MAX_DEVICE 50
#define PACKET_SIZE 4096 //数据包的长度
#define DATA_LEN 56		 //ICMP包数据部分长度
#define IPID 10
#define HWADDR(addr) ((unsigned char *)&addr)[0], ((unsigned char *)&addr)[1], ((unsigned char *)&addr)[2], ((unsigned char *)&addr)[3], ((unsigned char *)&addr)[4], ((unsigned char *)&addr)[5]


struct device_item //设备信息表包含的信息：1. 接口信息（ifindex)  2. 接口的MAC地址  3. 接口名称
{
	uint32_t ifIndex;
	uint8_t mac_addr[6];
	char interface[16];
	uint32_t IP;
} device[MAX_DEVICE];

char IPeth0[INET_ADDRSTRLEN] = "172.0.0.2";   //server2中： 1为内网，0为外网
char IPeth1[INET_ADDRSTRLEN] = "10.0.1.1";	//
char vpn_next[INET_ADDRSTRLEN] = "192.168.0.2"; //对server2来说
uint32_t vpnnext;
uint32_t netmask = 0xFFFFFF00;

//00:0c:29:f5:53:01
uint8_t destMACin[6] = {0x00, 0x0c, 0x29, 0xf5, 0x53, 0x01};  //内网主机的MAC地址
//00:0c:29:3c:e1:51
//00:0c:29:4c:a7:0d 
uint8_t destMACout[6] = {0x00, 0x0c, 0x29, 0x4c, 0xa7, 0x0d}; //外网下一跳的MAC地址：对server2来说，就是router的连到server2的网卡

char IFNAME0[] = "eth0";
char IFNAME1[] = "eth1";
struct sockaddr_ll from;	  //存储recvfrom时对方的socket数据结构:这里是ll
char sendpacket[PACKET_SIZE]; //发送ICMP数据报缓存
char recvpacket[PACKET_SIZE]; //接收ICMP数据报缓存
int sockfd;

void init() //初始化设备信息表：MAC地址和IP地址，设备信息等
{
	device[0].IP = (uint32_t)inet_addr(IPeth0);
	device[1].IP = (uint32_t)inet_addr(IPeth1);
	vpnnext = (uint32_t)inet_addr(vpn_next);
	//deviceind = 1;

	//=======================================below: get ifindex and MACaddr=========================================

	//get IPeth0 info: ifindex and macaddr
	struct ifreq req;
	int sfd;
	memset(&req, 0, sizeof(struct ifreq));

	strcpy(req.ifr_name, IFNAME0);
	strcpy(device[0].interface, IFNAME0);
	printf("连接外网的网卡的名称: %s\n", IFNAME0); //接口名称

	if ((sfd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP))) < 0) //建立socket, 用来模拟ifconfig命令获取接口信息的过程
	{
		perror("无法建立获取接口信息的socket！\n");
		return;
	}

	if (ioctl(sfd, SIOCGIFINDEX, &req) < 0) //一定要建立socket之后才能用socket号来获取那些接口index信息
	{
		perror("无法获得外网网卡ifindex！\n");
		return;
	}
	device[0].ifIndex = req.ifr_ifindex; //直接获得ifindex
	printf("连接外网的网卡的ifIndex = %u\n", device[0].ifIndex);

	//  get mac addr：获取接口的MAC地址
	if (ioctl(sfd, SIOCGIFHWADDR, &req) < 0)
	{
		perror("无法获得外网网卡MAC地址!\n");
		return;
	}
	memcpy(device[0].mac_addr, &(req.ifr_hwaddr.sa_data), ETH_ALEN);
	printf("连接外网的网卡的MAC地址 = %02X:%02X:%02X:%02X:%02X:%02X\n", HWADDR(device[0].mac_addr));

	close(sfd);

	//get IPeth1 info: ifindex and macaddr
	memset(&req, 0, sizeof(struct ifreq));

	strcpy(req.ifr_name, IFNAME1);
	strcpy(device[1].interface, IFNAME1);
	printf("连接内网的网卡的名称: %s\n", IFNAME1); //接口名称

	if ((sfd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP))) < 0) //建立socket, 用来模拟ifconfig命令获取接口信息的过程
	{
		perror("无法建立获取接口信息的socket！\n");
		return;
	}

	if (ioctl(sfd, SIOCGIFINDEX, &req) < 0) //一定要建立socket之后才能用socket号来获取那些接口index信息
	{
		perror("无法获得内网网卡ifindex！\n");
		return;
	}
	device[1].ifIndex = req.ifr_ifindex; //直接获得ifindex
	printf("连接内网的网卡的ifIndex = %u\n", device[1].ifIndex);

	//  get mac addr：获取接口的MAC地址
	if (ioctl(sfd, SIOCGIFHWADDR, &req) < 0)
	{
		perror("无法获得内网网卡MAC地址!\n");
		return;
	}
	memcpy(device[1].mac_addr, &(req.ifr_hwaddr.sa_data), ETH_ALEN);
	printf("连接内网的网卡的MAC地址 = %02X:%02X:%02X:%02X:%02X:%02X\n", HWADDR(device[1].mac_addr));

	close(sfd);
}

unsigned short cal_chksum(unsigned short *addr, int len) //计算校验和
{
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;

	//加法运算
	while (nleft > 1)
	{
		sum += *w++; //sum每次加w指向的地址的值，然后w指向下一个short数
		nleft -= 2;  //每次处理2个字节，故减2
	}

	if (nleft == 1)
	{ //如果剩下1个字节未处理：把它也加起来
		*(unsigned char *)(&answer) = *(unsigned char *)w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xffff); //sum本为32位数，这里将其高16位与低16位相加:就是加上所有的溢出值！
	sum += (sum >> 16);					//然后再加上其高16位：把溢出的值也加起来

	//取反后输出
	answer = ~sum;

	return answer; //最后取反
}

int from_inner(uint32_t in, uint32_t in2)
{
	return (ntohl(in) & netmask) == (ntohl(in2) & netmask);
}

void vpn_routing()
{
	/*任务：
	1. 收到IP包（只收IP包）之后，判断是来自VPN子网内部还是互联网
	2. 若是来自子网内部，则说明是要转发出去的，此时：
		把原来的以太网包头去掉，把其整个IP包的封装到一个新的IP包的内容字段，填充新的IP包头，以太网包头，然后用从指向外网的端口发送
	3. 若是来自互联网，则把其封装在IP包的内容字段的IP包取出，作为真正的IP包，放入当前IP字段。重新填充以太网包头，然后从指向内网的端口发送

	需要用到的参数：
	1. 本机的两个IP地址
	2. 本机的两张网卡的MAC地址
	3. 下一跳VPN服务器的IP地址

	转发规则：收到源IP地址不是当前VPN子网的包，则向从子网端口转发
	收到源IP地址是当前VPN子网的包，则从外网端口转发
	*/

	int n = 0;
	int fromlen = sizeof(from);
	extern int errno;
	struct ip *ip; //此数据结构可以在netinet/ip.h中查看
	struct ethhdr *eth;

	while (1)
	{
		if ((n = recvfrom(sockfd, recvpacket, sizeof(recvpacket), 0, (struct sockaddr *)&from, &fromlen)) < 0)
		{
			if (errno != EINTR)
			{
				perror("recvfrom error!\n");
				return;
			}
		}

		else
			break;
	}

	eth = (struct ethhdr *)recvpacket;

	switch (ntohs(eth->h_proto))
	{
	case ETH_P_IP:
		ip = (struct ip *)(eth + 1);
		//struct ip *ipt = (struct ip *)(ip + 1);
		if (ip->ip_p != IPPROTO_ICMP)
			break;
			
		printf("IP src addr: %s\n", inet_ntoa(ip->ip_src));
		
		//首先判断源地址是不是同一内网的
		if (from_inner(ip->ip_src.s_addr, device[1].IP)) //是从内网发出去的包
		{
			printf("Received packet from VPN subnet!\n");
			//把原来的以太网包头去掉，把其整个IP包的封装到一个新的IP包的内容字段，填充新的IP包头，以太网包头，然后用指向外网的端口发送
			memset(sendpacket, 0, sizeof(sendpacket));
			struct ethhdr *ethsend = (struct ethhdr *)sendpacket;
			struct ip *ipsend = (struct ip *)(ethsend + 1);
			struct icmp *icmpsend = (struct icmp *)(ipsend + 1);
			memcpy(sendpacket + 14 + 20 + 8, recvpacket + 14, 84);
			//memcpy((void *)(icmpsend + 1), (const void *)ip, 84); //包长度:8+56+20=84
			//memcpy((void *)(icmpsend + 1), (const void *)ip, 84);
			//memcpy((void *)(ipsend + 1), (const void *)ip, 84); //包长度:8+56+20
			
			struct icmp *icmpori = (struct icmp *)(recvpacket + 14 + 20);
			
			//icmpsend->icmp_type = ICMP_ECHO;   //设置type为ICMP_ECHO，发送数据包  
			icmpsend->icmp_type = icmpori->icmp_type;
			icmpsend->icmp_code = 0;  //ECHO      
			icmpsend->icmp_cksum = 0;   //校验值 :后面再重新计算，因为要计算整个包的检验和
			icmpsend->icmp_seq = 3;  //包序号      
			icmpsend->icmp_id = 2;
			icmpsend->icmp_cksum = cal_chksum((unsigned short *)icmpsend, 8+84); 
			
			//struct in_addr temp = {vpnnext};
			struct in_addr temp = {device[0].IP};

			ipsend->ip_hl = 5;
			ipsend->ip_v = 4;
			ipsend->ip_id = htons(IPID);
			ipsend->ip_ttl = ip->ip_ttl;
			ipsend->ip_p = IPPROTO_ICMP; //IP包
			//ipsend->ip_len = htons(sizeof(struct ip) + 8 + DATA_LEN);
			ipsend->ip_len = htons(84 + 8 + 20);
			ipsend->ip_sum = 0;
			
			//ipsend->ip_src = ip->ip_src;
			//ipsend->ip_dst = temp;
			
			ipsend->ip_src = temp;
			temp.s_addr = vpnnext;
			ipsend->ip_dst = temp;
			
			ipsend->ip_sum = cal_chksum((unsigned short *)ipsend, 20); //ip header 20

			ethsend->h_proto = htons(ETH_P_IP);
			memcpy(ethsend->h_source, device[0].mac_addr, 6);
			memcpy(ethsend->h_dest, destMACout, 6); //发往外网

			struct sockaddr_ll dest_addr =
			{
				.sll_family = PF_PACKET,
				.sll_protocol = htons(ETH_P_IP),
				.sll_halen = ETH_ALEN,
				.sll_ifindex = device[0].ifIndex,
			};
			memcpy(&dest_addr.sll_addr, destMACout, ETH_ALEN);

			int packetsize = 84 + 8 + 34;
			
			
			if (sendto(sockfd, sendpacket, packetsize, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_ll)) < 0)
			{
				perror("VPN sendto error\n");
				return;
			}
			else
			{	
				printf("向外网转发成功！\n");
			}
		}

		else //从外网接收到的包
		{
			//若是来自互联网，则把其封装在IP包的内容字段的IP包取出，作为真正的IP包，放入当前IP字段。重新填充以太网包头，然后从指向内网的端口发送
			printf("Received packet from internet!\n");
			memset(sendpacket, 0, sizeof(sendpacket));
			struct ethhdr *ethsend = (struct ethhdr *)sendpacket;
			//struct ip *ipsend = (struct ip *)(sendpacket + 14);
			//struct icmp *icmp = (struct icmp *)(recvpacket + 14 + 20);
			memcpy(sendpacket + 14, (recvpacket + 14 + 20 + 8), 84);
			//memcpy((void *)ipsend, (const void *)(icmp + 1), 84); //包长度:8+56+20=84

			//uint32_t destIP = ipsend->ip_dst;
			ethsend->h_proto = htons(ETH_P_IP);
			memcpy(ethsend->h_source, device[1].mac_addr, 6);
			memcpy(ethsend->h_dest, destMACin, 6); //发往内网

			struct sockaddr_ll dest_addr =
			{
				.sll_family = PF_PACKET,
				.sll_protocol = htons(ETH_P_IP),
				.sll_halen = ETH_ALEN,
				.sll_ifindex = device[1].ifIndex,
			};

			int packetsize = 84 + 14;

			memcpy(&dest_addr.sll_addr, destMACin, ETH_ALEN);
			if (sendto(sockfd, sendpacket, packetsize, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_ll)) < 0)
			{
				perror("VPN sendto error\n");
				return;
			}
			else
				printf("向内网转发成功！\n");
		}

		break;

	default:;
	}
}

int main()
{
	init();
	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) //只收IP包
	{
		perror("main() socket init error!\n");
		exit(1);
	}

	while (1)
	{
		memset(recvpacket, 0, sizeof(recvpacket));
		vpn_routing();
	}

	close(sockfd);

	return 0;
}
