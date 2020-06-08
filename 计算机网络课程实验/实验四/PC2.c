#pragma pack(1)
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

#define IPID 10
#define HWADDR(addr) ((unsigned char *)&addr)[0], ((unsigned char *)&addr)[1], ((unsigned char *)&addr)[2], ((unsigned char *)&addr)[3], ((unsigned char *)&addr)[4], ((unsigned char *)&addr)[5]
#define PACKET_SIZE 4096  //数据包的长度
#define DATA_LEN 30  //ICMP包数据部分长度
#define MAX_ARP_SIZE 50

struct ARP
{
	unsigned short arp_hrd; /* 硬件类型：1 表示以太网 */
	unsigned short arp_pro; /* 上层协议类型：ipv4 */
	uint8_t arp_hln;		/* 硬件地址长度 = 6 */
	uint8_t arp_pln;		/* 协议地址长度 = 4 */
	unsigned short arp_op;  /* ARP/RARP operation */

	uint8_t arp_sha[6]; /* sender hardware address */
	uint32_t arp_spa;   /* sender protocol address:IP地址 */
	uint8_t arp_tha[6]; /* target hardware address */
	uint32_t arp_tpa;   /* target protocol address：IP地址 */
};

struct ARPtable //ARP表项：无需初始化，一开始就应该是空的
{
	uint32_t IPaddr; //二进制的IP地址
	uint8_t macaddr[6];
};

struct ARPtable arp_table[MAX_ARP_SIZE];
int arpind = 0;

char sendpacket[PACKET_SIZE];   //发送ICMP数据报缓存
char recvpacket[PACKET_SIZE];   //接收ICMP数据报缓存
int nsend = 0;//发送数据报时的序号, 即pack（）中的pack_no参数
int sockfd;
struct sockaddr_ll from;   //存储recvfrom时对方的socket数据结构:这里是ll
struct timeval send_time;  //发送数据包的时间。
struct timeval recv_time;  //接收数据包时的时间。减去send_time, 则可以得到RTT

//PC2: 00:0c:29:24:28:2c 
char IP[INET_ADDRSTRLEN] = "192.168.3.2";       	//IP
char gateway[INET_ADDRSTRLEN] = "192.168.3.3";  	//网关
uint32_t IPbin;                   					//IP的二进制形式
uint32_t gatewaybin;								//网关的二进制形式
uint8_t MAC[6];										//MAC地址
//uint8_t destMAC[6] = {0x00,0x0c,0x29,0x3c,0xe1,0x47};							//目的mac地址: router2 eth0
uint8_t destMAC[6];
uint32_t destIP;									//目的IP的二进制形式
char IFNAME[] = "eth0";  							//网卡的名称
uint32_t ifIndex;  									//发送ARP和ICMP包时要用


unsigned short cal_chksum(unsigned short *addr, int len);
void getIfInfo();
int unpack(char *buf, int len);
void recv_packet();

unsigned short cal_chksum(unsigned short *addr, int len)  //计算校验和
{       
    int nleft = len;
    int sum = 0;        
    unsigned short *w = addr;        
    unsigned short answer = 0;        
    
    //加法运算
    while (nleft>1)
    {       
        sum += *w++; //sum每次加w指向的地址的值，然后w指向下一个short数
        nleft-=2;    //每次处理2个字节，故减2
    }

    if (nleft == 1)
    {   //如果剩下1个字节未处理：把它也加起来
        *(unsigned char *)(&answer)=*(unsigned char *)w;                
        sum+=answer;        
    }        

    sum = (sum >> 16) + (sum & 0xffff);  //sum本为32位数，这里将其高16位与低16位相加:就是加上所有的溢出值！
    sum += (sum >> 16);  //然后再加上其高16位：把溢出的值也加起来
    
    //取反后输出        
    answer = ~sum;        

    return answer;  //最后取反
}


//获取接口信息:index和MAC地址 
void getIfInfo()
{
    struct ifreq req;
	int sfd;
    memset(&req, 0, sizeof(struct ifreq));

    strcpy(req.ifr_name, IFNAME);   //IFNAMSIZ - 1 把借口名称复制到req内
    printf("PC2接口名称: %s\n", IFNAME);  //接口名称
    
    if ((sfd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP))) < 0) //建立socket, 用来模拟ifconfig命令获取接口信息的过程
    {
        perror("无法建立获取接口信息的socket！\n");
        return;
    }

    // get interface index 
    // SIOCGIFINDEX :Retrieve the interface index of the interface into ifr_ifindex.
    if (ioctl(sfd, SIOCGIFINDEX, &req) < 0) //一定要建立socket之后才能用socket号来获取那些接口index信息
    {
        perror("无法获得ifindex！\n");
        return;
    }
    ifIndex = req.ifr_ifindex;  //直接获得ifindex
   	printf("ifIndex of PC2 = %u\n", ifIndex);

    //  get mac addr：获取接口的MAC地址
    if (ioctl(sfd, SIOCGIFHWADDR, &req) < 0)
    {
        perror("无法获得PC2的MAC地址!\n");
        return;
    }
    memcpy(MAC, &(req.ifr_hwaddr.sa_data), ETH_ALEN);
    printf("MAC of PC2 = %02X:%02X:%02X:%02X:%02X:%02X\n", HWADDR(MAC));

    close(sfd);
}


int unpack(char *buf, int len)
{  
    int i, iphdrlen;        
    struct ip *ip;      //此数据结构可以在netinet/ip.h中查看      
    struct icmp *icmp;  //此数据结构在netinet/ip_icmp.h中查看      
	struct ethhdr *eth;

	eth = (struct ethhdr *)buf;
	if (ntohs(eth->h_proto) == ETH_P_IP && strncmp((char *)eth->h_dest, (char *)MAC, 6) == 0)
	{	
		printf("Is IP Protocol && Dest is here!\n");  
		memcpy(eth->h_source, MAC, 6);
		memcpy(eth->h_dest, destMAC, 6);

		ip = (struct ip *)(eth + 1);     //ip头部
		iphdrlen = ip->ip_hl * 4;  		// ip_hl以4字节为单位 :规定就是这样 
		icmp = (struct icmp *)(ip + 1);  //icmp段的地址 :IP指针移动头部的长度后达到
		len -= (iphdrlen + 14);                         //ICMP包长度: IP包总长度减IP包头部长度
		
		if (len < 8)               //ICMP固定长度为8，小于即错误       
		{       
			printf("ICMP packets\'s length is less than 8!\n");                
			return -1;        
		}

		if (ip->ip_dst.s_addr == IPbin && ip->ip_p == IPPROTO_ICMP)
		{
			printf("Dest IP is here && Is ICMP!\n");
			int packsize = 8 + DATA_LEN;
			printf("Receive packet from %s\n", inet_ntoa(ip->ip_src));
			printf("ICMP type: %d\n", icmp->icmp_type);
			
			if (icmp->icmp_type == ICMP_ECHO)
			{
				printf("收到想要的ICMP包!\n"); 
				printf("ICMP data: %s\n", (char*)&icmp->icmp_data);
				
				uint32_t temp = ip->ip_src.s_addr;
				ip->ip_src = ip->ip_dst;
				ip->ip_dst.s_addr = temp;

				ip->ip_sum = 0;
				ip->ip_sum = cal_chksum((unsigned short *)ip, 20); //ip header 20

				icmp->icmp_type = ICMP_ECHOREPLY;
				icmp->icmp_cksum = 0;
				icmp->icmp_cksum = cal_chksum((unsigned short *)icmp, packsize); //计算检验和

				packsize += 20 + 14;
				struct sockaddr_ll dest_addr = 
				{
					.sll_family   = AF_PACKET,
					.sll_protocol = htons(ETH_P_IP),
					.sll_halen    = ETH_ALEN,
					.sll_ifindex  = ifIndex,
				};

				//注意这里：这里原作者用固定的MAC地址代替了自动寻找目的MAC的过程
				//char nextMac[ETH_ALEN]={0x00,0x0c,0x29,0x6e,0x87,0x27};
				//getNextMac(nextMac);
				memcpy(&dest_addr.sll_addr, destMAC, ETH_ALEN); //注意：发送时，目的MAC地址应是当前主机的网关的MAC地址
			
				if (sendto(sockfd, recvpacket, packsize, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_ll)) < 0)
				{
					perror("Reply\'s Sendto error!\n");
					return -1;
				}
				
				else
					printf("成功发送回复包!\n");

				return packsize;
			}

			else if (icmp->icmp_type == ICMP_DEST_UNREACH)
			{
				printf("Received unreachable ping packet!\n");
				return -1;
			}
		}
	}

	else if (ntohs(eth->h_proto) == ETH_P_ARP)
	{
		struct ARP *arp = (struct ARP *)(eth + 1);
		if (ntohs(arp->arp_op) == ARPOP_REQUEST && arp->arp_tpa == IPbin)  //Router2发的ARP包的desDTIP一定是IPbin
		{
			//记录下Router2的IP和MAC地址
			memcpy(destMAC, arp->arp_sha, 6);
			destIP = arp->arp_spa;

			//填写回复的ARP包
			arp->arp_op = htons(ARPOP_REPLY); //reply
			memcpy(arp->arp_sha, MAC, 6);
			arp->arp_spa = IPbin;
			memcpy(arp->arp_tha, destMAC, 6);
			arp->arp_tpa = destIP;

			//填写ARP表
			arp_table[arpind].IPaddr = destIP;
			memcpy(arp_table[arpind].macaddr, destMAC, 6);

			//填写回复的以太网包头
			memcpy(eth->h_source, MAC, 6);
			memcpy(eth->h_dest, destMAC, 6);

			struct sockaddr_ll dest_addr =
			{
				.sll_family = AF_PACKET,
				.sll_protocol = htons(ETH_P_ARP),
				.sll_halen = ETH_ALEN,
				.sll_ifindex = ifIndex,
			};
			memcpy(&dest_addr.sll_addr, destMAC, ETH_ALEN); //注意：发送时，目的MAC地址应是当前主机的网关的MAC地址

			if (sendto(sockfd, recvpacket, 42, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_ll)) < 0)
			{
				perror("ARP Reply Sendto error!\n");
				return -1;
			}

			return len;
		}
	}
	
	return -1;
}


void recv_packet()
{       
    int n, fromlen;        
    extern int errno;
	
    fromlen = sizeof(from);
    
    while (1)
	{
		//不断等待，直到收到包
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
    
  	
  	printf("收到包!\n");
    if (unpack(recvpacket, n) == -1)
    {//进行解包
    	fprintf(stderr, "%s\n", "Recv Error!");   
    }
}


int main(int argc, char *argv[])
{
	//struct sockaddr_in dest_addr;   //ping的目的地址数据结构
    //in_addr_t inaddr = 0; 			//存储ip地址：in_addr_t就是uint32_t
	//inet_pton(AF_INET, ip, (uint8_t*)&IPbin)
	getIfInfo();
	IPbin = (uint32_t)inet_addr(IP);
	gatewaybin = (uint32_t)inet_addr(gateway);

	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) //用SOCK_RAW还是SOCK_DGRAM
	{       
        perror("socket error");
        exit(1);
    }     

    //printf("PING %s: %d bytes data in ICMP packets.\n", argv[1], DATA_LEN);       
    //inet_ntoa把网络地址转换为点的形式：32位网络字节序的二进制IP地址转换成相应的点分十进制的IP地址 

    while (1)
    {
		memset(recvpacket, 0, sizeof(recvpacket));
		recv_packet();               //接收数据报
	}

    close(sockfd); 
	
	return 0;
}
