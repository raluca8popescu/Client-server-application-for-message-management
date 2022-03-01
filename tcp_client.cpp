#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include "helpers.h"
#include <string>
#include <iostream>
#include<bits/stdc++.h>

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, n, ret;
	struct sockaddr_in cli_addr;
	char buffer[BUFLEN];
	char buf[RECV_MAX];
	string for_read, topic, sf, for_send;
	string message;
	int send_id = 0; 

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar

	if (argc < 3) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	/* Dezactivare algoritmul lui Neagle*/ 
	int flag = 1;
	int res = setsockopt(sockfd,          
                        IPPROTO_TCP,    
                        TCP_NODELAY,   
                        (char *) &flag,  
                        sizeof(int)); 
	DIE(res < 0, "Nagle");

	/* Creare socket client tcp*/
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &cli_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	/* Conectarea clientului la server */
	ret = connect(sockfd, (struct sockaddr*) &cli_addr, sizeof(cli_addr));
	DIE(ret < 0, "connect");
	send_id = 1;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(sockfd, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	while (1) {

		if (send_id == 1) {
			/* Se trimite id-ul clientului catre server */
			n = send(sockfd, argv[1], strlen(argv[1]), 0);
			DIE(n < 0, "send");
			send_id = 0;
		}else {

			tmp_fds = read_fds; 
		
			ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
			DIE(ret < 0, "select");
			if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
				/* Se citesc de la tastatura comezi*/
				cin >> for_read;
				if (for_read == "exit") {
					break;
				}

				/* Creare comanda pentru server */
				if (for_read == "subscribe") {
					cin >> topic >> sf;
					for_send = for_read + "*" + argv[1] + "*" + topic + "*" + sf;
					message = "Subscribed to topic.\n";
				}else {
					cin >> topic;
					for_send = for_read + "*" + argv[1] + "*" + topic;
					message = "Unsubscribed from topic.\n";
				}
				/* Trimitere comanda catre server */
				memset(buffer, 0, BUFLEN);
				strcpy(buffer, for_send.c_str());
				n = send(sockfd, buffer, strlen(buffer), 0);
				DIE(n < 0, "send");
				cout << message;

			}

			if (FD_ISSET(sockfd, &tmp_fds)) {
				memset(buf, 0, RECV_MAX);
				n = recv(sockfd, buf, RECV_MAX, 0);
				DIE(n < 0, "recv");
				/* comenzi de la server */
				if (((string) buf) == "exit") {
					break;
				}else {
					/* comanda cu continutul unui mesaj udp */
					cout << buf;
				}

			}
		}
	}
	/* Inchiderea socketului */
	close(sockfd);

	return 0;
}
