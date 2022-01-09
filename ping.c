#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <errno.h>
/*
    please run this program with root permission.
*/

// uint16_t cal_chksum(uint16_t *buff, int nLen)
// {      
//     int nleft = nLen;
//     int nSum = 0;
//     unsigned short * w = buff;
//     unsigned short answer=0;

//     /*把ICMP报头二进制数据以2字节为单位累加起来*/
//     while(nleft>1)
//     {       
//         nSum+=*w++;
//         nleft-=2;
//     }
//     /*若ICMP报头为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
//     if( nleft==1)
//     {       
//         *(unsigned char *)(&answer)=*(unsigned char *)w;
//         nSum+=answer;
//     }
//     nSum=(nSum>>16)+(nSum&0xffff);
//     nSum+=(nSum>>16);
//     answer=~nSum;
//     return answer;
// }

uint16_t getCheckSum(uint16_t *buff, int nLen)
{      

}

int main()
{
    int nSockFd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    int nPid = getpid();
    char * dest_ip = "220.181.38.148";

    // send a icmp packet
    struct icmphdr stIcmpHeader;
    stIcmpHeader.type = 8;
    stIcmpHeader.code = 0;
    stIcmpHeader.checksum = 0;
    stIcmpHeader.un.echo.id = nPid;
    stIcmpHeader.un.echo.sequence = 0;
    stIcmpHeader.checksum = getCheckSum(&stIcmpHeader, sizeof(stIcmpHeader));

    struct sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    int ret = inet_aton(dest_ip, (struct in_addr*)&peer_addr.sin_addr.s_addr);
    if(ret == -1){
        printf("error: %s\n", strerror(errno));
        return -1;
    }
    int n = sendto(nSockFd, &stIcmpHeader, sizeof(stIcmpHeader), 0, (struct sockaddr*)&peer_addr, sizeof(peer_addr));
    
    // recv a packet
    char recv_buffer[2000] = {0};
    int nAddrLen = sizeof(peer_addr);
    int bytes = recvfrom(nSockFd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr*)&peer_addr, &nAddrLen);
    printf("%d\n", bytes);
    struct ip* stIpHeader = (struct ip *)recv_buffer;
    int nIpHeaderLen = stIpHeader->ip_hl * 4; // why?
    printf("%s\n", inet_ntoa(stIpHeader->ip_src));
    printf("%s\n", inet_ntoa(stIpHeader->ip_dst));
    return 0;
}