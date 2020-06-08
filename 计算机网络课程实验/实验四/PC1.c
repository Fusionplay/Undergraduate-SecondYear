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
//PC1 MAC: 00:0c:29:e4:41:e2
#include <memory.h>
#include <assert.h>
#include <time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#define MAX_ARP_SIZE 50
extern int errno;

struct ARP
{
    unsigned short arp_hrd; /* 硬件类型：1 表示以太网 */
    unsigned short arp_pro; /* 上层协议类型：ipv4 */ 
    uint8_t arp_hln;        /* 硬件地址长度 = 6 */  
    uint8_t arp_pln;        /* 协议地址长度 = 4 */
    unsigned short arp_op;  /* ARP/RARP operation */

    uint8_t arp_sha[6];     /* sender hardware address */
    uint32_t arp_spa;       /* sender protocol address:IP地址 */
    uint8_t arp_tha[6];     /* target hardware address */
    uint32_t arp_tpa;       /* target protocol address：IP地址 */
};

struct ARPtable  //ARP表项：无需初始化，一开始就应该是空的
{
    uint32_t IPaddr;  //二进制的IP地址
    uint8_t macaddr[6];
};

struct ARPtable arp_table[MAX_ARP_SIZE];
int arpind = 0;

#define IPID 10
#define HWADDR(addr) ((unsigned char *)&addr)[0], ((unsigned char *)&addr)[1], ((unsigned char *)&addr)[2], ((unsigned char *)&addr)[3], ((unsigned char *)&addr)[4], ((unsigned char *)&addr)[5]
#define PACKET_SIZE 4096  //数据包的长度
#define DATA_LEN 30  //ICMP包数据部分长度

char sendpacket[PACKET_SIZE];   //发送ICMP数据报缓存
char recvpacket[PACKET_SIZE];   //接收ICMP数据报缓存
int nsend = 0;//发送数据报时的序号, 即pack（）中的pack_no参数
int sockfd;
struct sockaddr_ll from;   //存储recvfrom时对方的socket数据结构:这里是ll
struct timeval send_time;  //发送数据包的时间。
struct timeval recv_time;  //接收数据包时的时间。减去send_time, 则可以得到RTT
uint32_t netmask = 0xFFFFFF00;


char IP[INET_ADDRSTRLEN] = "192.168.1.1";       	//IP
char gateway[INET_ADDRSTRLEN] = "192.168.1.2";  	//网关
uint32_t IPbin;                   					//IP的二进制形式
uint32_t gatewaybin;								//网关的二进制形式
uint8_t MAC[6];								        //本机MAC地址
//uint8_t destMAC[6] = {0x00, 0x0c,0x29,0xaa,0xc7,0xa6};	//目的mac地址: should be the MAC of router1 eth0
uint8_t destMAC[6];
uint32_t destIP;                                    //目的IP的二进制形式
char IFNAME[] = "eth0";  							//网卡的名称
uint32_t ifIndex;  									//发送ARP和ICMP包时要用
struct in_addr src;
struct in_addr dest;
	

unsigned short cal_chksum(unsigned short *addr, int len);
void getIfInfo();
int pack(int pack_no);
void send_packet();
void time_differ(struct timeval *out, struct timeval *in);
int unpack(char *buf, int len);
void recv_packet();
uint32_t subnet(uint32_t ip);
void get_mac_addr(uint32_t targetIP, uint8_t *res);


uint32_t subnet(uint32_t ip)
{
    return htonl(ntohl(ip) & netmask);
}


unsigned short cal_chksum(unsigned short *addr, int len) //计算校验和
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
    printf("PC1接口名称: %s\n", IFNAME);  //接口名称
    
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
   	printf("ifIndex of PC1 = %u\n", ifIndex);

    //  get mac addr：获取接口的MAC地址
    if (ioctl(sfd, SIOCGIFHWADDR, &req) < 0)
    {
        perror("无法获得PC1的MAC地址!\n");
        return;
    }
    memcpy(MAC, &(req.ifr_hwaddr.sa_data), ETH_ALEN);
    printf("MAC of PC1 = %02X:%02X:%02X:%02X:%02X:%02X\n", HWADDR(MAC));

    close(sfd);
}


void get_mac_addr(uint32_t targetIP, uint8_t *res)
{
    //1. 先查找现有的ARP表，没有再发包
    int i;
    for (i = 0; i < arpind; i++)
    {
        if (arp_table[i].IPaddr == targetIP)
        {
            memcpy(res, arp_table[i].macaddr, 6);
            printf("MAC addr already in here!\n");
            return;
        }
    }

    printf("需要发送ARP!\n");
    uint32_t localsubnet = subnet(IPbin);
    uint32_t targetsubnet = subnet(targetIP);
    uint32_t arp_target_ip;

    if (localsubnet == targetsubnet)
        arp_target_ip = targetIP;
    else
        arp_target_ip = gatewaybin;


    uint8_t sendtemp[200];
    uint8_t recvtemp[200];
    memset(sendtemp, 0, sizeof(sendtemp)); //首先置0
    memset(recvtemp, 0, sizeof(recvtemp));
    struct ethhdr *eth = (struct ethhdr *)sendtemp;
    struct ARP *arp = (struct ARP *)(eth + 1);


    eth->h_proto = htons(ETH_P_ARP);
    memcpy(eth->h_source, MAC, 6);
    memset(eth->h_dest, 0xFF, 6);

    arp->arp_hrd = htons(1);
    arp->arp_pro = htons(ETH_P_IP);
    arp->arp_hln = 6;
    arp->arp_pln = 4;
    arp->arp_op = htons(ARPOP_REQUEST); //request

    memcpy(arp->arp_sha, MAC, 6);
    arp->arp_spa = IPbin;
    memset(arp->arp_tha, 0x00, 6);
    arp->arp_tpa = arp_target_ip;
	printf("Target MAC of ARP = %02X:%02X:%02X:%02X:%02X:%02X\n", HWADDR(arp->arp_tha));
	struct in_addr desttemp = {arp->arp_tpa};
	printf("Target IP = %s\n", inet_ntoa(desttemp));
	desttemp.s_addr = htonl(arp->arp_tpa);
	printf("Target IP rev = %s\n", inet_ntoa(desttemp));

    struct sockaddr_ll dest_addr =
    {
        .sll_family = PF_PACKET,
        .sll_protocol = htons(ETH_P_ARP),
        .sll_halen = ETH_ALEN,
        .sll_ifindex = ifIndex, //自己的ifIndex
    };
    memset(&dest_addr.sll_addr, 0xFF, ETH_ALEN);

    if (sendto(sockfd, sendtemp, 42, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_ll)) < 0)
    {
        perror("ARP sendto error！\n");
        exit(-1);
    }
    else
        printf("ARP请求发送成功!\n");

    //开始接收ARP包
    int n, fromlen;
    struct sockaddr_ll fromtemp;
    fromlen = sizeof(fromtemp);

    while (1)
    {
        while (1)
        {
            if ((n = recvfrom(sockfd, recvtemp, sizeof(recvtemp), 0, (struct sockaddr *)&fromtemp, &fromlen)) < 0)
            {
                if (errno != EINTR)
                {
                    perror("ARP recvfrom error!\n");
                    exit(-1);
                }
            }

            else
                break;
        }

        eth = (struct ethhdr *)recvtemp;
        arp = (struct ARP *)(eth + 1);
        if (ntohs(eth->h_proto) == ETH_P_ARP && ntohs(arp->arp_op) == ARPOP_REPLY && arp->arp_tpa == IPbin)
        {
            printf("Received wanted ARP!\n");

            //arp_table[arpind].IPaddr = arp->arp_spa;
            arp_table[arpind].IPaddr = targetIP;  //目标IP：不一定就是路由器的IP地址！
            memcpy(arp_table[arpind].macaddr, arp->arp_sha, 6);
            memcpy(res, arp->arp_sha, 6);
            arpind++;
            return;
        }

        else
            printf("不是想要的ARP回应包！\n");
    }
}


int pack(int pack_no)
{       
	struct ip *ip;      //此数据结构可以在netinet/ip.h中查看      
    struct icmp *icmp;  //此数据结构在netinet/ip_icmp.h中查看  
	struct ethhdr *eth;
	int packsize;

	memset(sendpacket, 0, sizeof(sendpacket)); //首先置0
	eth = (struct ethhdr*)sendpacket; 
	ip = (struct ip*)(eth + 1); 
	uint32_t iphdrlen = 5;  // ip_hl以4字节为单位 :规定就是这样 
    icmp = (struct icmp *)(ip + 1);  //icmp段的地址 :IP指针移动头部的长度后达到
	
	
	//设置各个字段
    //memcpy(destMAC, )
    uint8_t mactemp[6];
    get_mac_addr(destIP, mactemp);
    memcpy(destMAC, mactemp, 6);
    eth->h_proto = htons(ETH_P_IP);
    memcpy(eth->h_source, MAC, 6);
	memcpy(eth->h_dest, destMAC, 6);

    ip->ip_hl = iphdrlen;
    ip->ip_v = 4;
    ip->ip_id = htons(IPID);
    ip->ip_ttl = htons(64);
    ip->ip_p = IPPROTO_ICMP; //ICMP包
    ip->ip_len = htons(sizeof(struct ip) + 8 + DATA_LEN);
    ip->ip_sum = 0;
    ip->ip_src = src;
    ip->ip_dst = dest;
    ip->ip_sum = cal_chksum((unsigned short *)ip, 20); //ip header 20
    
    icmp->icmp_type = ICMP_ECHO;   //设置type为ICMP_ECHO，发送数据包
    icmp->icmp_code = 0;    //ECHO      
    icmp->icmp_cksum = 0;   //校验值 :后面再重新计算，因为要计算整个包的检验和
    icmp->icmp_seq = pack_no;  //包序号      
    icmp->icmp_id = 2;    //ID，使用PID唯一标识来标识？？？并没有必要,自己随便设置一个就行了
    packsize = 8 + DATA_LEN; //包头长度(8)+数据长度(DATA_LEN)  
	//packsize = sizeof(struct icmp);
	
	gettimeofday(&send_time, NULL);
	strcpy((char*)&icmp->icmp_data, "123456789");
    icmp->icmp_cksum = cal_chksum((unsigned short *)icmp, packsize); //计算检验和:整个ICMP包的检验和！
    
	return packsize + 20 + 14;
}


void send_packet()
{       
    int packetsize = pack(nsend); //打包           
    
	//这里到时候要修改
	struct sockaddr_ll dest_addr = 
    {
        .sll_family   = PF_PACKET,
        .sll_protocol = htons(ETH_P_IP),
        .sll_halen    = ETH_ALEN,
        .sll_ifindex  = ifIndex,  //自己的ifIndex
    };

    //注意这里：这里原作者用固定的MAC地址代替了自动寻找目的MAC的过程
    //char nextMac[ETH_ALEN]={0x00,0x0c,0x29,0x6e,0x87,0x27};
    //getNextMac(nextMac);
    memcpy(&dest_addr.sll_addr, destMAC, ETH_ALEN); //注意：发送时，目的MAC地址应是当前主机的网关的MAC地址:也即下一跳的MAC地址

    //发送icmp包
    //int ret = sendto(sockfd, buf, sizeof(struct IPPack), 0, (struct sockaddr *)&dest_addr, sizeof( struct sockaddr_ll));
   
    if (sendto(sockfd, sendpacket, packetsize, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_ll)) < 0)
    {
        perror("sendto error\n");
        return;
    }
}


void time_differ(struct timeval *out, struct timeval *in)   //计算接收时间与发送时间的差值
{   //如果接收时间小于发送时间，进行借位。  
    out->tv_usec -= in->tv_usec; //微秒
    out->tv_sec -= in->tv_sec; //秒
    
    if (out->tv_usec < 0)
    {
    	out->tv_sec--;
    	out->tv_usec += 1000000;
    }
}


int unpack(char *buf, int len)
{  
    int i, iphdrlen;        
    struct ip *ip;      //此数据结构可以在netinet/ip.h中查看      
    struct icmp *icmp;  //此数据结构在netinet/ip_icmp.h中查看      
	struct ethhdr *eth;
    double rtt;         //RTT(Round Trip Time)：一个连接的往返时间，即数据发送时刻到接收到确认的时刻的差值；

	eth = (struct ethhdr *)buf;
	len -= 14;
	if (ntohs(eth->h_proto) == ETH_P_IP && strncmp((char *)eth->h_dest, (char *)MAC, 6) == 0)
	{
		ip = (struct ip *)(eth + 1);     //ip头部        
		iphdrlen = ip->ip_hl * 4;  // ip_hl以4字节为单位 :规定就是这样 
		icmp = (struct icmp *)(ip + 1);  //icmp段的地址 :IP指针移动头部的长度后达到
		len -= iphdrlen;                         //ICMP包长度: IP包总长度减IP包头部长度
		
		if (len < 8)               //ICMP固定长度为8，小于即错误       
		{       
			printf("ICMP packets\'s length is less than 8!\n");                
			return -1;        
		}

		if ( icmp->icmp_type == ICMP_ECHOREPLY && icmp->icmp_id == 2 )  //判断是不是想要的包   
		{    
			time_differ(&recv_time, &send_time);             
			rtt = ((double)recv_time.tv_sec) * 1000.0 + ((double)recv_time.tv_usec) / 1000.0;  
			//ttl 生存时间和往返时间 rtt（单位是毫秒)
			
			//打印提示信息
			printf("%d byte from %s: icmp_seq = %u ttl = %d rtt = %.3f ms\n",
			len, inet_ntoa(ip->ip_src), icmp->icmp_seq, ip->ip_ttl, rtt); //from存储对方的地址

			return len;
		}    //inet_ntoa把网络地址转换为点的形式：32位网络字节序的二进制IP地址转换成相应的点分十进制的IP地址


		else
		{
			//printf("icmp->icmp_id: %d\n", icmp->icmp_id);
			
			/*
			if (icmp->icmp_type == ICMP_ECHOREPLY)
				printf("Is ICMP_ECHOREPLY\n");
			else 
				printf("Not ICMP_ECHOREPLY\n");
			*/
			
			return -1;
		}
	}

    else 
		return -1;
}


void recv_packet()
{       
    int n, fromlen;
    fromlen = sizeof(from);
    
    while (1)
    {
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
    
		gettimeofday(&recv_time, NULL);
		if (unpack(recvpacket, n) != -1)
		{//进行解包
			//fprintf(stderr, "%s\n", "Recv Error! Unreachable\n");   
			break;
		}
		//unpack(recvpacket, n);
    }
    
    	
    /*
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
    
    gettimeofday(&recv_time, NULL);
    unpack(recvpacket, n);
    */
    /*
    if (unpack(recvpacket, n) == -1)
    {//进行解包
    	fprintf(stderr, "%s\n", "Recv Error! Unreachable\n");   
    }
    */
}


int main(int argc, char *argv[])
{
	//struct sockaddr_in dest_addr;   //ping的目的地址数据结构
    //in_addr_t inaddr = 0; 			//存储ip地址：in_addr_t就是uint32_t
	//inet_pton(AF_INET, ip, (uint8_t*)&IPbin)
	getIfInfo();
	IPbin = (uint32_t)inet_addr(IP);
	gatewaybin = (uint32_t)inet_addr(gateway);
	
	printf("ARP size = %d\n", sizeof(struct ARP));
    if (argc < 2)        
    {       
        printf("usage:%s hostname/IP address\n", argv[0]);       
        exit(1);        
    }
    
	destIP = (uint32_t)inet_addr(argv[1]);

    if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) //用SOCK_RAW还是SOCK_DGRAM
    {
        perror("socket error");
        exit(1);
    }     
    
    src.s_addr = IPbin;
    dest.s_addr = destIP;
	
	
	if (IPbin == destIP)
		printf("eq\n");
	else 
		printf("noteq\n");
	
	
	printf("dest IP: %s\n", inet_ntoa(dest));
	
    printf("PING %s: %d bytes data in ICMP packets.\n", argv[1], DATA_LEN);       
    //inet_ntoa把网络地址转换为点的形式：32位网络字节序的二进制IP地址转换成相应的点分十进制的IP地址 
	printf("\n\n");
	
    while (1)
    {
        nsend++;
        send_packet();    //发送数据报
        recv_packet();               //接收数据报
        sleep(1);
    }

    close(sockfd); 
	
	return 0;
}

