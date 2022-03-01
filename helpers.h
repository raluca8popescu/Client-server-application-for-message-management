#ifndef _HELPERS_H
#define _HELPERS_H 1


/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define DIE(assertion, call_description)  \
	do {								  \
		if (assertion) {				  \
			fprintf(stderr, "(%s, %d): ", \
					__FILE__, __LINE__);  \
			perror(call_description);	  \
			exit(EXIT_FAILURE);			  \
		}							      \
	} while(0)                            

/* Dimensiunea maxima a calupului de date */
#define BUFLEN 128
/* Dimensiunea maxima a mesajelor udp trimise de catre server la tcp_client*/
#define RECV_MAX 1600
/* Numarul maxim de clienti tcp*/
#define MAX_CLIENTS 100
/* Dimensiunea maxima a payload-ului udp*/
#define MAX 1500

#endif
