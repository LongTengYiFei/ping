/*
    Please run this program with root permission.
*/

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
#include <pthread.h>

double getTimeStamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + ((double)tv.tv_usec) / 1000000;
}

uint16_t getCheckSum(uint16_t *buff, int nLen)
{      
    int nleft = nLen;
    int nSum = 0;
    unsigned short * w = buff;
    unsigned short nAnswer=0;

    while(nleft>1)
    {       
        nSum+=*w++;
        nleft-=2;
    }
    
    if( nleft==1)
    {       
        *(unsigned char *)(&nAnswer)=*(unsigned char *)w;
        nSum+=nAnswer;
    }
    nSum=(nSum>>16)+(nSum&0xffff);
    nSum+=(nSum>>16);
    nAnswer=~nSum;
    return nAnswer;
}

struct STSendPackArgs{
    char * pszDestIp;
    uint16_t m_nPid;
    int m_nSockFd;
    struct sockaddr_in stPeerAddress;
};

struct STRecvPackArgs{
    int m_nSockFd;
    uint16_t m_nPid;
    struct sockaddr_in stPeerAddress;
};

void* sendPack(void * args)
{
    struct STSendPackArgs* Args = args;

    struct icmphdr stIcmpHeader;
    stIcmpHeader.type = 8;
    stIcmpHeader.code = 0;
    stIcmpHeader.un.echo.id = Args->m_nPid;
    
    for(uint16_t i=0; ; i++)
    {
        sleep(1);
        stIcmpHeader.un.echo.sequence = i;
        stIcmpHeader.checksum = 0;
        stIcmpHeader.checksum = getCheckSum((uint16_t *)&stIcmpHeader, sizeof(stIcmpHeader));
        int n = sendto(Args->m_nSockFd, &stIcmpHeader, sizeof(stIcmpHeader), 0, (struct sockaddr*)&Args->stPeerAddress, sizeof(Args->stPeerAddress));
    }
    return NULL;
}

void* recvPackAndShow(void * args)
{
    struct STRecvPackArgs* Args = args;
    char recv_buffer[2000] = {0};
    int nAddrLen = sizeof(Args->stPeerAddress);
    while(1)
    {   
        int nBytes = recvfrom(Args->m_nSockFd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr*)&Args->stPeerAddress, &nAddrLen);
        if(nBytes < 0)
            continue;

        struct ip* pstIpHeader = (struct ip *)recv_buffer;
        int nIpHeaderLen = pstIpHeader->ip_hl * 4;
        int nTTL = pstIpHeader->ip_ttl;

        struct icmphdr * pstIcmpHeader = (struct icmphdr *)(recv_buffer + nIpHeaderLen);
        int nIcmpSeq = pstIcmpHeader->un.echo.sequence;
        int nId = pstIcmpHeader->un.echo.id;
        
        if(Args->m_nPid != nId){
            //printf("Recv a icmp packet, but the id is not the host's pid. The id is %d, the pid is %d\n", nId, Args->m_nPid);
            continue;
        }
        printf("%d bytes from %s: icmp_seq=%d ttl=%d\n", nBytes, inet_ntoa(pstIpHeader->ip_src), nIcmpSeq, nTTL);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int nSockFd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    pid_t nPid = getpid();
    char * pszDestIp = "220.181.38.148";

    struct sockaddr_in stPeerAddress;
    memset(&stPeerAddress, 0, sizeof(stPeerAddress));
    stPeerAddress.sin_family = AF_INET;
    int ret = inet_aton(pszDestIp, (struct in_addr*)&stPeerAddress.sin_addr.s_addr);
    if(ret == -1){
        printf("error: %s\n", strerror(errno));
        return -1;
    }

    struct STSendPackArgs stSendPackArgs;
    stSendPackArgs.m_nPid = nPid;
    stSendPackArgs.m_nSockFd = nSockFd;
    stSendPackArgs.pszDestIp = pszDestIp;
    stSendPackArgs.stPeerAddress = stPeerAddress;

    struct STRecvPackArgs stRecvPackArgs;
    stRecvPackArgs.m_nSockFd = nSockFd;
    stRecvPackArgs.m_nPid = nPid;
    stRecvPackArgs.stPeerAddress = stPeerAddress;

    pthread_t tidSendPack;
    pthread_t tidRecvPack;
    
    pthread_create(&tidSendPack, NULL, sendPack, &stSendPackArgs);
    pthread_create(&tidRecvPack, NULL, recvPackAndShow, &stRecvPackArgs);

    pthread_join(tidRecvPack, NULL);
    pthread_join(tidSendPack, NULL);
    return 0;
}
