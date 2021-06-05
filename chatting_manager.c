#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#define MAX_BUF 1024
#define MAX_SOCK 50
#define TTL 5 //time to live

#define CMD_SIZE 4
#define NAME_SIZE 30
#define MSG_SIZE 1024

int client[MAX_SOCK];


int main(int argc, char **argv)
{
    /*************
    int m_sockfd;  //multicast socket
    int hb_sockfd; //heartbeat socket

    *************/

    int server_sockfd, client_sockfd, sockfd;
    int state, client_len;
    int i, max_client, maxfd;

    int ttl = TTL;

    struct sockaddr_in clientaddr, serveraddr;
    struct timeval tv;
    fd_set readfds, otherfds, allfds;

    char buf[MAX_BUF];
    int on = 1;

    state = 0;

    //error
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error : ");
        exit(0);
    }
    //

    memset(&serveraddr, 0x00, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(atoi(argv[1])); //port

    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
    setsockopt(server_sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

    state = bind(server_sockfd, (struct sockaddr *)&serveraddr,
                 sizeof(serveraddr));
    if (state == -1)
    {
        perror("bind error : ");
        exit(0);
    }

    state = listen(server_sockfd, 5);
    if (state == -1)
    {
        perror("listen error : ");
        exit(0);
    }

    client_sockfd = server_sockfd;

    // init
    max_client = -1;
    maxfd = server_sockfd;

    for (i = 0; i < MAX_SOCK; i++)
    {
        client[i] = -1;
    }

    FD_ZERO(&readfds);               //readfds reset
    FD_SET(server_sockfd, &readfds); //server socket open

    /*
    FD_SET(m_sockfd, &readfds);   //multicast socket open
    FD_SET(hb_sockfd, &readfds); //heartbeat socket open
    */

    printf("\nTCP Server Starting ... \n\n\n");
    fflush(stdout);

    while (1)
    {
        allfds = readfds;                                     //readfds backup
        state = select(maxfd + 1, &allfds, NULL, NULL, NULL); //select**

        // Server Socket - accept from client
        // 클라이언트 연결
        if (FD_ISSET(server_sockfd, &allfds))
        {
            client_len = sizeof(clientaddr);
            client_sockfd = accept(server_sockfd,
                                   (struct sockaddr *)&clientaddr, &client_len);
            printf("\nconnection from (%s , %d)",
                   inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

            for (i = 0; i < MAX_SOCK; i++)
            {
                if (client[i] < 0)  //클라이언트 번호 설정이 안된곳 찾음
                {
                    client[i] = client_sockfd; 
                    printf("\nclientNUM=%d\nclientFD=%d\n", i + 1, client_sockfd);
                    break;
                }
            }

            printf("accept [%d]\n", client_sockfd);
            printf("===================================\n");
            if (i == MAX_SOCK)  //클라이언트가 너무 많이 접속했을때
            {
                perror("too many clients\n");
            }
            FD_SET(client_sockfd, &readfds);

            if (client_sockfd > maxfd)
                maxfd = client_sockfd;

            if (i > max_client)
                max_client = i;

            if (--state <= 0)
                continue;
        }

        // client socket check
        // 클라이언트와 연결 여부 체크
        for (i = 0; i <= max_client; i++)
        {
            sockfd = client[i];
            if (sockfd < 0)
            {
                continue;
            }

            // allfds에 sockfd가 저장되어 있음
            // 즉, sockfd가 accepted된 클라이언트일 때
            if (FD_ISSET(sockfd, &allfds))
            {
                memset(buf, 0, MAX_BUF);

                if (read(sockfd, buf, MAX_BUF) <= 0) // 메시지를 읽을 클라이언트가 사라짐
                {
                    printf("Close sockfd : %d\n", sockfd);
                    printf("===================================\n");

                    // Disconnect from Client
                    close(sockfd);
                    FD_CLR(sockfd, &readfds);
                    client[i] = -1;
                }
                else  // 메시지 읽어오기 성공
                {
                    printf("[%d]RECV: %s\n", sockfd, buf);
                    

                    // 모든 클라이언트에 수신받은 메시지 전송
                    for (int j = 0; j <= max_client; j++)
                    {
                        if (client[j] == -1) continue;
                        write(client[j], buf, MAX_BUF);
                        printf("Sent to client[%d]\n", client[j]);
                    }

                    // 모든 클라이언트에 UDP 메시지 전송
                    for (int j = 0; j <= max_client; j++)
                    {
                        if (client[j] == -1) continue;
                        sendto(client[j], buf, MAX_BUF, 0, (struct sockaddr *)&clientaddr, client_len);
                        printf("Sent UDPMSG to client[%d]\n", client[j]);
                    }
                }

                if (--state <= 0)
                    break;
            }
        } // for
    }     // while
}
