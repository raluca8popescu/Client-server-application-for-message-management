#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "helpers.h"
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <iostream>
#include <list>
#include <iterator>
#include <map>
#include <math.h> 

using namespace std;

/* Structura pentru un client tcp*/
typedef struct {
    string id;
	int sock;
	bool active;
	list<string> saved; /* Mesajele udp cu sf 1 care vor fi trimise la reconectare*/
	map <string, char> topics; /* Topicurile la care este abonat un client */
    }Subscriber;

/* Structura pentru un mesaj udp */
typedef struct {
	char topic[50];
	char data_type;
	char payload[MAX];
}Message_udp;

/* Primeste un numar in format string si il aduce la precizia corecta*/
string fix(string str, int p)
{
	size_t poz = str.find('.');
	for (std::string::size_type i = str.size() - 1; i > poz + p ; i--) {
		   str.pop_back();
    }
	return str;
}

/* Adauga topicul si sf-ul primite de la client in structuri*/
void add_subscriber(map <string, Subscriber> *all, map <string, list<int>> *subscribers, char *pch) {
	pch = strtok(NULL, "*");
	string id = ((string)pch);
	std::map<string, Subscriber>::iterator it = (*all).find(id); 
	if (it != (*all).end()) {
		pch = strtok(NULL, "*");
		string topic = ((string)pch);
		pch = strtok(NULL, "*");
		char sf = pch[0];
		it->second.topics.insert(make_pair(topic, sf));
		int sock = it->second.sock;
		std::map<string, list<int>>::iterator iter = (*subscribers).find(topic);
		if (iter != (*subscribers).end()) {
			iter->second.push_back(sock);
		}else {
			list<int> sockets;
			sockets.push_back(sock);
			(*subscribers).insert(make_pair(topic, sockets));
		}
	}
}
/* Elimina topicul primit de la client din structuri*/
void remove_subscriber(map <string, Subscriber> *all, map <string, list<int>> *subscribers, char *pch) {
	pch = strtok(NULL, "*");
	string id = ((string)pch);
	std::map<string, Subscriber>::iterator it = (*all).find(id); 
	if (it != (*all).end()) {
		pch = strtok(NULL, "*");
		string topic = ((string)pch);
		it->second.topics.erase(topic);
		int sock = it->second.sock;
		std::map<string, list<int>>::iterator iter = (*subscribers).find(topic);
		if (iter != (*subscribers).end()) {
			iter->second.remove(sock);
			if(iter->second.empty()) {
				(*subscribers).erase(topic);
			}
		}
	}
}

string create_string_subscribers(int port, char *ip, Message_udp udp) {
	int number;

	/* daca datele sunt INT*/
	if (udp.data_type == 0) {
		int sign, result;
		sign = ((int)udp.payload[0]);
		memcpy(&number, udp.payload + 1, 4);
		result = ntohl(number);
		if (sign == 1) {
			result = -result;
		}
		/* creare mesaj trimis catre client*/
		return (string(ip) + ":" + to_string(port) + " - " + udp.topic + " - " + "INT" + " - " + fix(to_string(result), 0) + "\n");
	}
	/* daca datele sunt SHORT_REAL */
	if (udp.data_type == 1) {
		double result;
		memcpy(&number, udp.payload, 2);
		number = ntohs(number);
		result = ((double)number) / 100;
		return (string(ip) + ":" + to_string(port) + " - " + udp.topic + " - " + "SHORT_REAL" + " - " + fix(to_string(result), 2) + "\n");
	}
	/* daca datele sunt FLOAT */
	if(udp.data_type == 2) {
		int sign;
		uint8_t number2;
		double result;
		sign = ((int)udp.payload[0]);
		memcpy(&number, udp.payload + 1, 4);
		number = ntohl(number);
		memcpy(&number2, udp.payload + 5, 1);
		if (number2 == 0) {
			result = number;
		}else {
			result = number / pow(10.0, number2);
		}
		if (sign == 1) {
			result = -result;
		}
		return (string(ip) + ":" + to_string(port) + " - " + udp.topic + " - " + "FLOAT" + " - " + fix(to_string(result), number2) + "\n");
	}
	/* daca datele sunt STRING */
	if (udp.data_type == 3) {
		return (string(ip) + ":" + to_string(port) + " - " + udp.topic + " - " + "STRING" + " - " + ((string) udp.payload) + "\n");
	}

	return NULL;

}
/* Se trimit mesajele de la udp pentru un anumit topic catre subscriberi*/
void send_message_subscribers(Message_udp udp, string message, 
			map <string, list<int>> subscribers, map <int, string> clients,
		 	map <string, Subscriber> *all, list<string> inactive) {

	std::map<string, list<int>>::iterator it;
	char buff[RECV_MAX];
	memset(buff, 0, RECV_MAX);
	strcpy(buff, message.c_str());
	it = subscribers.find(udp.topic);
	/* Pentru clientii care sunt conectati la server*/
	if (it != subscribers.end()) {
		list<int> sockets = subscribers[udp.topic];
		for (auto iter = sockets.begin(); iter != sockets.end(); ++iter) {
			int n = send(*iter, buff, strlen(buff), 0);
			DIE(n < 0, "send");
		}
	}
	/* Pentru clientii care nu mai sunt conectati la server dar sunt subscriberi la topic*/
	for(auto i = inactive.begin(); i != inactive.end(); ++i) {
		std::map<string, Subscriber>::iterator it = (*all).find(*i);
		if (it->second.topics.find(udp.topic) != it->second.topics.end()) {
			/* Se salveaza mesajele in lista saved */
			if (it->second.topics[udp.topic] == '1') {
				it->second.saved.push_back(message);
			}
		} 
	}

}
/* Se trimit mesajele pastrate pentru clientii care s-au reconectat */
void send_messages_saved(map <string, Subscriber> *all, string id) {
	Subscriber sub = (*all)[id];
	char buff[RECV_MAX];
	list<string> saved_mess = sub.saved;
	std::list<std::string>::const_iterator i;
	for (i = saved_mess.begin(); i != saved_mess.end(); ++i) {
		memset(buff, 0, RECV_MAX);
		strcpy(buff, (*i).c_str());
		int n = send(sub.sock, buff, strlen(buff), 0);
		DIE(n < 0, "send");
	}
	std::map<string, Subscriber>::iterator it = (*all).find(id);
	it->second.saved.clear();
}

int main(int argc, char* argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int listenfd = -1;
    struct sockaddr_in serv_addr_udp, serv_addr_tcp, cli_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);
    int sockfd, newsockfd, portno;
    char buffer[BUFLEN];
    int fd;
    int n, i, ret, dest, chrRead;
	socklen_t clilen;
	string command;
	char *pch;

	/* map cu socket + id */
	map <int, string> clients;
	/* map cu topic + lista de socketi */
	map <string, list<int>> subscribers;
	/*map cu  id + Subscriber */
	map <string, Subscriber> all;
	/* lista cu id-urile clientilor deconectati */
	list <string> inactive_id;

	Message_udp upd_buffer;
	std::map<int, string>::iterator it1;
	std::map<string, list<Subscriber>>::iterator it2;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds


     // creare socket server tcp 
    FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	/* dezactivare algoritmul lui Neagle*/ 
	int flag = 1;
	int res = setsockopt(sockfd,   
                        IPPROTO_TCP,    
                        TCP_NODELAY,  
                        (char *) &flag,
                        sizeof(int)); 
	DIE(res < 0, "Nagle");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr_tcp, 0, sizeof(serv_addr_tcp));
	serv_addr_tcp.sin_family = AF_INET;
	serv_addr_tcp.sin_port = htons(portno);
	serv_addr_tcp.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *) &serv_addr_tcp, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");


	 // creare socket server udp
	int sockid = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(sockid == -1, "Open socket");
	
	serv_addr_udp.sin_family = AF_INET;
	serv_addr_udp.sin_port = htons(atoi(argv[1]));
	serv_addr_udp.sin_addr.s_addr = INADDR_ANY;

    socklen_t szfrmst = sizeof(serv_addr_udp);
	int rs = bind(sockid, (struct sockaddr *)(&serv_addr_udp), sizeof(serv_addr_udp));
	DIE(rs < 0, "Open bind");
	int rf = 0;

	/* se adauga file descriptorii in multimea read_fds */
	FD_SET(sockfd, &read_fds);
	FD_SET(sockid, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	fdmax = sockid;

	while (1) {
		
		tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				if (i == STDIN_FILENO) {
					cin >> command;
					if (command == "exit") {

						for (int j = 5; j <= fdmax; j++) {
							n = send(j, "exit", 5, 0);
						}
						close(sockfd);
						close(sockid);
						return 0;
					}
				}
					if (i == sockfd) {
						/* a venit o cerere de conexiune pe socketul inactiv si este acceptata */
						clilen = sizeof(cli_addr);
						newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
						DIE(newsockfd < 0, "accept");

						/* se primeste mesaj cu id-ul de la clientul tcp */
						memset(buffer, 0, BUFLEN);
						n = recv(newsockfd, buffer, sizeof(buffer), 0);
						DIE(n < 0, "recv");
						string id = ((string)buffer);

						/* Daca id-ul nu exista deja */
						if (all.find(id) == all.end()) {
							/* Se adauga legatura socket-id in clients */
							clients.insert(make_pair(newsockfd, id));
							/* Se creeaza o noua structura de subscriber pt clientul tcp */
							Subscriber new_sub;
							new_sub.id = id;
							new_sub.sock = newsockfd;
							new_sub.active = true;
							all.insert(make_pair(id, new_sub));
							/* Se adauga noul socket in read_fds */
							FD_SET(newsockfd, &read_fds);
							if (newsockfd > fdmax) { 
								fdmax = newsockfd;
							}
							printf("New client %s connected from %s:%d.\n", buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
						}else {
							/* Daca id-ul exista deja si este conectat la server */
							if (all[id].active) {
								cout << "Client " << id << " already connected.\n";
								n = send(newsockfd, "exit", 5, 0);
							/* Daca se realizeaza reconectarea unui client */
							}else {
								clients.insert(make_pair(newsockfd, id));
								/* Se scoate de la clienti inactivi */
								inactive_id.remove(id);
								all[id].active = true;
								all[id].sock = newsockfd;
								/* Se adauga socketul in read_fds */
								FD_SET(newsockfd, &read_fds);
								if (newsockfd > fdmax) { 
									fdmax = newsockfd;
								}
								/* Se trimit mesajele pastrate */
								send_messages_saved(&all, id);
								/* Se adauga socketul nou la toate topicurile la care era abonat */
								for (auto iter = subscribers.begin(); iter != subscribers.end(); ++iter) {
									if (all[id].topics.find(iter->first) != all[id].topics.end()){
										iter->second.push_back(newsockfd);
									}
								}
								printf("New client %s connected from %s:%d.\n", buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
							}
						}


					} else {
						/* Daca este mesaj de la clientul udp */
						if (i == sockid) {
							memset(&upd_buffer, 0, sizeof(Message_udp));
							rf = recv(sockid, &upd_buffer, sizeof(Message_udp), 0);
							DIE(rf < 0, "recv udp");
							/* Se creeaza mesajul si este trimis catre clientii tcp */
							string st = create_string_subscribers(ntohs(serv_addr_udp.sin_port), inet_ntoa(serv_addr_udp.sin_addr), upd_buffer);
							send_message_subscribers(upd_buffer, st, subscribers, clients, &all, inactive_id);
						}else {
							/* Se primesc date de la clientii tcp */
							memset(buffer, 0, BUFLEN);
							n = recv(i, buffer, sizeof(buffer), 0);
							DIE(n < 0, "recv");

							if (n == 0) {
								/* Conexiunea clientului tcp s-a inchis */
								cout << "Client " << clients[i] << " disconnected.\n";
								std::map<string, Subscriber>::iterator it = all.find(clients[i]); 
								if (it != all.end()) {
									it->second.active = false;
								}
								/* Se sterge socketul din topicuri */
								for (auto iter = subscribers.begin(); iter != subscribers.end(); ++iter) {
									if (it->second.topics.find(iter->first) != it->second.topics.end()){
										iter->second.remove(i);
									}
								}
								/* Se trece la client inactiv */
								inactive_id.push_back(clients[i]);
								clients.erase(i);
								close(i);
								
								/* Se scoate socketul din multimea de citire */
								FD_CLR(i, &read_fds);
							} else {
								
								pch = strtok(buffer, "*");
								/* Daca clientul face subscribe la un topic */
								if (((string)pch) == "subscribe") {
									add_subscriber(&all, &subscribers, pch);
								}else {
									/* Daca clientul face unsubscribe la un topic */
									remove_subscriber(&all, &subscribers, pch);
								}
							}
						}
					}
			}
		}
	}
	/* Se inchid socketii */
	close(sockfd);
	close(sockid);
    return 0;
}