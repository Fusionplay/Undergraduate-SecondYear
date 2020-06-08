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

#define HWADDR(addr) ((unsigned char *)&addr)[0], ((unsigned char *)&addr)[1], ((unsigned char *)&addr)[2], ((unsigned char *)&addr)[3], ((unsigned char *)&addr)[4], ((unsigned char *)&addr)[5]

#define MAX_ARP_SIZE 50
#define MAX_ROUTE_INFO 50
#define MAX_DEVICE 50
#define PACKET_SIZE 4096 //数据包的长度
#define DATA_LEN 56		 //ICMP包数据部分长度

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

struct sockaddr_ll from;	  //存储recvfrom时对方的socket数据结构:这里是ll
char sendpacket[PACKET_SIZE]; //发送ICMP数据报缓存
char recvpacket[PACKET_SIZE]; //接收ICMP数据报缓存
int sockfd;

struct route_item //路由表包含的信息：1. 子网号码  2. 子网号对应的本机路由器接口号  3. 对应的目的MAC地址
{
	uint32_t range;
	char interface[16];
	uint8_t dest_MAC[6];     //有待初始化
	uint32_t nexthopip;      //要路由到的下一跳的IP地址
} route_table[MAX_ROUTE_INFO];
int routeind = 0;

struct device_item     //设备信息表包含的信息：1. 接口信息（ifindex)  2. 接口对应的IP地址  3. 接口的MAC地址  4. 接口名称
{
	uint32_t ifIndex;
	uint32_t IP;       
	uint8_t mac_addr[6];
	char interface[16];
} device[MAX_DEVICE];   //device[0]: PC1, device[1]: R2
int deviceind = 0;

//eth0: 00:0c:29:aa:c7:a6
//eth1: 00:0c:29:aa:c7:b0
char IPPC1[INET_ADDRSTRLEN] = "192.168.1.2";       	//IP:与PC1相连的网卡
char IPR2[INET_ADDRSTRLEN] = "192.168.2.1";  		//IP:与router2相连的网卡
char NextIP[INET_ADDRSTRLEN] = "192.168.2.2";		//下一跳IP：router2的eth1网卡
char net1[INET_ADDRSTRLEN] = "192.168.1.0";
char net2[INET_ADDRSTRLEN] = "192.168.2.0";
char net3[INET_ADDRSTRLEN] = "192.168.3.0";
uint32_t netmask = 0xFFFFFF00;
char IFNAME1[] = "eth0";							//与PC1相连的网卡
char IFNAME2[] = "eth1";							//与router2相连的网卡
//uint8_t destMacPC1[6] = {0x00,0x0c,0x29,0xe4,0x41,0xe2};					//PC1的网卡
//uint8_t destMacR2[6] = {0x00,0x0c,0x29,0x3c,0xe1,0x51};					//router2与router1相连的网卡: router2 eth1
uint8_t destMacPC1[6]; //PC1的网卡
uint8_t destMacR2[6];

void init_router();
void routing();
uint32_t subnet(uint32_t ip);


void init_router() //初始化路由表，设备信息表：MAC地址和IP地址，设备信息等
{	
	device[0].IP = (uint32_t)inet_addr(IPPC1);
	device[1].IP = (uint32_t)inet_addr(IPR2);
	deviceind = 1;

	//=======================================below: get ifindex and MACaddr=========================================

	//get IPPC1 info: ifindex and macaddr
	struct ifreq req;
	int sfd;
    memset(&req, 0, sizeof(struct ifreq));

	strcpy(req.ifr_name, IFNAME1);
	strcpy(device[0].interface, IFNAME1);
	printf("连接PC1的网卡的名称: %s\n", IFNAME1); //接口名称

	if ((sfd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP))) < 0) //建立socket, 用来模拟ifconfig命令获取接口信息的过程
    {
        perror("无法建立获取接口信息的socket！\n");
        return;
    }

    if (ioctl(sfd, SIOCGIFINDEX, &req) < 0) //一定要建立socket之后才能用socket号来获取那些接口index信息
    {
        perror("无法获得PC1网卡ifindex！\n");
        return;
    }
    device[0].ifIndex = req.ifr_ifindex;  //直接获得ifindex
   	printf("连接PC1的网卡的ifIndex = %u\n", device[0].ifIndex);

    //  get mac addr：获取接口的MAC地址
    if (ioctl(sfd, SIOCGIFHWADDR, &req) < 0)
    {
        perror("无法获得PC1网卡MAC地址!\n");
        return;
    }
    memcpy(device[0].mac_addr, &(req.ifr_hwaddr.sa_data), ETH_ALEN);
    printf("连接PC1的网卡的MAC地址 = %02X:%02X:%02X:%02X:%02X:%02X\n", HWADDR(device[0].mac_addr));

    close(sfd);


	//get IPR2 info: ifindex and macaddr
	memset(&req, 0, sizeof(struct ifreq));

    strcpy(req.ifr_name, IFNAME2);
	strcpy(device[1].interface, IFNAME2);
	printf("连接Router2的网卡的名称: %s\n", IFNAME2); //接口名称

	if ((sfd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP))) < 0) //建立socket, 用来模拟ifconfig命令获取接口信息的过程
    {
        perror("无法建立获取接口信息的socket！\n");
        return;
    }

    if (ioctl(sfd, SIOCGIFINDEX, &req) < 0) //一定要建立socket之后才能用socket号来获取那些接口index信息
    {
        perror("无法获得Router2网卡ifindex！\n");
        return;
    }
    device[1].ifIndex = req.ifr_ifindex;  //直接获得ifindex
   	printf("连接Router2的网卡的ifIndex = %u\n", device[1].ifIndex);

    //  get mac addr：获取接口的MAC地址
    if (ioctl(sfd, SIOCGIFHWADDR, &req) < 0)
    {
        perror("无法获得Router2网卡MAC地址!\n");
        return;
    }
    memcpy(device[1].mac_addr, &(req.ifr_hwaddr.sa_data), ETH_ALEN);
    printf("连接Router2的网卡的MAC地址 = %02X:%02X:%02X:%02X:%02X:%02X\n", HWADDR(device[1].mac_addr));

    close(sfd);


	//=======================================below: set route table==========================================

	route_table[0].range = (uint32_t)inet_addr(net2);   //通往子网2：也即router2
	strcpy(route_table[0].interface, IFNAME2);
	memcpy(route_table[0].dest_MAC, destMacR2, 6);
	route_table[0].nexthopip = (uint32_t)inet_addr(NextIP);

	route_table[1].range = (uint32_t)inet_addr(net3);   //通往子网3：也即router2 + PC2
	strcpy(route_table[1].interface, IFNAME2);
	memcpy(route_table[1].dest_MAC, destMacR2, 6);
	route_table[1].nexthopip = (uint32_t)inet_addr(NextIP);

	route_table[2].range = (uint32_t)inet_addr(net1); 	//通往子网1：也即PC1
	strcpy(route_table[2].interface, IFNAME1);
	memcpy(route_table[2].dest_MAC, destMacPC1, 6);

	routeind = 2;
}


uint32_t subnet(uint32_t ip)
{
	return htonl(ntohl(ip) & netmask);
}


void routing()
{
	int n = 0;
	int fromlen = sizeof(from);
	extern int errno;
	struct ip *ip;	 //此数据结构可以在netinet/ip.h中查看
	struct ethhdr *eth;
	struct icmp *icmpptr;
	struct ARP *arp;

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

	//printf("Received a packet!\n");
	eth = (struct ethhdr *)recvpacket;
	switch (ntohs(eth->h_proto))
	{
	case ETH_P_ARP:
		arp = (struct ARP *)(eth + 1);
		if (arp->arp_tpa == device[0].IP && ntohs(arp->arp_op) == ARPOP_REQUEST) //来自PC1的ARP请求
		{
			printf("收到了来自PC1的ARP包\n");
			//arp_table[arpind].IPaddr = arp->arp_spa;
			memcpy(route_table[2].dest_MAC, arp->arp_sha, 6);  //通往子网1
			route_table[2].nexthopip = arp->arp_spa;
			uint32_t ttemp = arp->arp_spa; //PC1的IP地址

			arp->arp_op = htons(ARPOP_REPLY); //reply
			memcpy(arp->arp_sha, device[0].mac_addr, 6);    	//设置源MAC
			arp->arp_spa = device[0].IP;						//设置源IP
			memcpy(arp->arp_tha, route_table[2].dest_MAC, 6);   //设置目的MAC
			arp->arp_tpa = ttemp;					//设置目的IP

			memcpy(eth->h_source, device[0].mac_addr, 6);
			memcpy(eth->h_dest, arp->arp_tha, 6);

			struct sockaddr_ll dest_addr =
			{
				.sll_family = AF_PACKET,
				.sll_protocol = htons(ETH_P_ARP),
				.sll_halen = ETH_ALEN,
				.sll_ifindex = device[0].ifIndex,
			};
			memcpy(&dest_addr.sll_addr, arp->arp_tha, ETH_ALEN); //注意：发送时，目的MAC地址应是当前主机的网关的MAC地址

			if (sendto(sockfd, recvpacket, 42, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_ll)) < 0)
			{
				perror("To PC1 ARP Reply Sendto error!\n");
				exit(-1);
			}

			else
				printf("回复PC1的ARP包发送成功！\n");
		}

		break;


	case ETH_P_IP:
		//printf("IP packet received!\n");

		ip = (struct ip *)(eth + 1);		   //ip头部
		icmpptr = (struct icmp*)(ip + 1);
		if (ip->ip_p != IPPROTO_ICMP)
		{
			//printf("Received IP packet is not an ICMP packet!\n");
			break;
		}

		printf("ICMP source: %s\n", inet_ntoa(ip->ip_src));
		printf("ICMP destination: %s\n", inet_ntoa(ip->ip_dst));
		printf("ICMP data: %s\n", (char*)&icmpptr->icmp_data);
		
		uint32_t destrange = subnet((uint32_t)ip->ip_dst.s_addr);
		struct in_addr desttemp = {destrange};
		printf("IP destination\'s subnet number: %s\n", inet_ntoa(desttemp));
		uint32_t ifind;		//要发送至下一跳的接口的接口信息
		uint8_t destmac[6];	//下一跳的MAC地址
		uint8_t ifmac[6];   //要发送至下一跳的接口的MAC地址
		char tmp[20];		//要发送至下一跳的接口的接口名称
		int i;
		int routetableind;
		int devicetableind;

		for (i = 0; i <= routeind; i++)//根据IP包的目的IP所属的子网段的号码，来确定发送至下一跳应该用路由器的哪个接口，以及下一跳的MAC地址
		{
			if (route_table[i].range == destrange)
			{
				strcpy(tmp, route_table[i].interface);
				memcpy(destmac, route_table[i].dest_MAC, 6);
				routetableind = i;
				printf("匹配到一条路由规则：对应的接口名：%s\n", tmp);
				break;
			}
		}

		if (i == routeind + 1)
		{
			printf("No matched routing rule!\n");
			break;
		}

		for (i = 0; i <= deviceind; i++)
		{
			if (strcmp(tmp, device[i].interface) == 0) //根据发送接口的名称，来在设备信息表中获取接口的接口信息和MAC地址
			{
				ifind = device[i].ifIndex;
				devicetableind = i;   //来提取对应接口的IP地址
				memcpy(ifmac, device[i].mac_addr, 6);
				break;
			}
		}

		//判断需不需要发送ARP请求包来确定PC2的MAC
		int cnt = 0;
		int j;
		for (j = 0; j < 6; j++)
		{
			if (destmac[j] == 0x0)
				cnt++;
		}
		if (cnt == 6) //说明这个destmac空缺
		{
			printf("需要发送arp来寻找路由的下一跳的MAC地址\n");
			//发送ARP包
			uint8_t sendtemp[200];
			uint8_t recvtemp[200];
			memset(sendtemp, 0, sizeof(sendtemp)); //首先置0
			memset(recvtemp, 0, sizeof(recvtemp));
			struct ethhdr *ethtmp = (struct ethhdr *)sendtemp;
			struct ARP *arptmp = (struct ARP *)(ethtmp + 1);

			ethtmp->h_proto = htons(ETH_P_ARP);
			memcpy(ethtmp->h_source, ifmac, 6);
			memset(ethtmp->h_dest, 0xFF, 6);  //广播地址

			arptmp->arp_hrd = htons(1);
			arptmp->arp_pro = htons(ETH_P_IP);
			arptmp->arp_hln = 6;
			arptmp->arp_pln = 4;
			arptmp->arp_op = htons(ARPOP_REQUEST); //request

			memcpy(arptmp->arp_sha, ifmac, 6);
			arptmp->arp_spa = device[devicetableind].IP;
			memset(arptmp->arp_tha, 0, 6);
			arptmp->arp_tpa = route_table[routetableind].nexthopip;

			struct sockaddr_ll ARPdest =
			{
				.sll_family = PF_PACKET,
				.sll_protocol = htons(ETH_P_ARP),
				.sll_halen = ETH_ALEN,
				.sll_ifindex = ifind, //自己的ifIndex
			};
			memset(&ARPdest.sll_addr, 0xFF, ETH_ALEN);

			if (sendto(sockfd, sendtemp, 42, 0, (struct sockaddr *)&ARPdest, sizeof(struct sockaddr_ll)) < 0)
			{
				perror("To Router2 ARP sendto error！\n");
				exit(-1);
			}
			else
				printf("To Router2 ARP请求发送成功!\n");

			//开始接收ARP包
			int num, fromlen;
			struct sockaddr_ll fromtemp;
			fromlen = sizeof(fromtemp);

			while (1)
			{
				while (1)
				{
					if ((num = recvfrom(sockfd, recvtemp, sizeof(recvtemp), 0, (struct sockaddr *)&fromtemp, &fromlen)) < 0)
					{
						if (errno != EINTR)
						{
							perror("Router2 ARP recvfrom error!\n");
							exit(-1);
						}
					}
					else
						break;
				}

				ethtmp = (struct ethhdr *)recvtemp;
				arptmp = (struct ARP *)(ethtmp + 1);
				if (ntohs(ethtmp->h_proto) == ETH_P_ARP && ntohs(arptmp->arp_op) == ARPOP_REPLY && arptmp->arp_tpa == device[devicetableind].IP)
				{
					printf("Received wanted Router2 ARP reply!\n");

					// memcpy(destmac, route_table[i].dest_MAC, 6);
					// routetableind = i;

					memcpy(route_table[routetableind].dest_MAC, arptmp->arp_sha, 6);
					memcpy(destmac, arptmp->arp_sha, 6);

					//arp_table[arpind].IPaddr = targetIP; //目标IP：不一定就是路由器的IP地址！
					//memcpy(arp_table[arpind].macaddr, arp->arp_sha, 6);
					//memcpy(res, arp->arp_sha, 6);
					//arpind++;
					break;
				}

				else
					printf("不是想要的Router2 ARP回应包！\n");
			}
		}


		//最终转发包
		struct sockaddr_ll dstAddr =
			{
				.sll_family = AF_PACKET,
				.sll_protocol = htons(ETH_P_IP),
				.sll_halen = ETH_ALEN,
				.sll_ifindex = ifind,
			};

		memcpy(dstAddr.sll_addr, destmac, ETH_ALEN);
		memcpy(eth->h_source, ifmac, 6);
		memcpy(eth->h_dest, destmac, 6);

		if (sendto(sockfd, recvpacket, n, 0, (struct sockaddr *)&dstAddr, sizeof(struct sockaddr_ll)) < 0)
		{
			perror("router sendto error!\n");
			return;
		}
		else
			printf("转发包发送成功!\n");

		break;
		

	default:
		//printf("Unaccepted protocol!\n");
		break;
	}
	
	printf("\n\n===================================================\n");
}


int main(int argc, char *argv[])
{
	init_router();
	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) //用SOCK_RAW还是SOCK_DGRAM
	{   //目前只收ICMP包
		perror("main() socket init error!\n");
		exit(1);
	}

	while (1)
	{
		memset(recvpacket, 0, sizeof(recvpacket));
		routing();
	}

	close(sockfd);

	return 0;
}


