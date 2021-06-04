#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
	int sock;
	char message[BUF_SIZE];
	int str_len;

	struct sockaddr_in serv_adr;
	struct ip_mreq join_addr;

	struct timeval ti;
	struct timeval tr;

	tr.tv_sec = 0;
	tr.tv_usec = 50000;

	fd_set readfds;

	if (argc != 3)
	{
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		printf("socket() error");
		exit(0);
	}

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	// JOIN MULTICAST GROUP
	// 주어진 멀티캐스팅 주소로 연결한다.
	join_addr.imr_multiaddr.s_addr = inet_addr("239.0.100.1");
	join_addr.imr_interface.s_addr = htonl(INADDR_ANY);
	setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&join_addr, sizeof(join_addr));

	// read()에 타임아웃 설정
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tr, sizeof(tr));

	if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
	{
		printf("connect() error!");
		exit(0);
	}
	else
		printf("Connected...........\n");

	//////// 서버와 통신 시작! ///////

	while (1)
	{
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);

		ti.tv_sec = 0;
		ti.tv_usec = 50000;

		if (select(1, &readfds, NULL, NULL, &ti) > 0)
		{
			memset(message, 0, BUF_SIZE);

			fgets(message, BUF_SIZE, stdin);

			if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
				break;

			write(sock, message, strlen(message));
			fputs("Input message(Q to quit): ", stdout);
		}

		if (read(sock, message, BUF_SIZE) > 0) {
			printf("Message from server: %s", message);
		}
	}

	close(sock);
	return 0;
}
