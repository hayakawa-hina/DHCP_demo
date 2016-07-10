#include "def.h"

int client_stat;
int alloc_flag = 0;

void alrmfunc(int sig){
	if(client_stat == C_GET_IP){
		fprintf(stderr, "one-Half of Time to live\n");
		client_stat = C_WAIT_ACK;
		printf("STAT_CHANGE : C_GET_IP(3) -> C_WAIT_ACK(2)\n");
		alloc_flag = 1;
	}else{
		perror("ERROR: SIGALRM");
		exit(1);
	}
}

void client_end(int sig){
	if(client_stat == C_GET_IP){
		fprintf(stderr, "\nCatch SIGINT\n");
		client_stat = C_END;
		printf("STAT_CHANGE : C_GET_IP(3) -> C_END(4)\n");
	}else{
		perror("\nSIGINT");
		exit(1);
	}
}


int main(int argc, char *argv[])
{
	int test_flag = 0;

	char *serv;
	char host[BUF_LEN];
	int sd, err;
	int datalen;
	int s_num;
	int s1_count = 0;
	int s2_count = 0;
	int retry_flag = 0;
	int alloc_flag2 = 0;
  
	struct timeval timeout;//recv(10sec)
	struct itimerval timer;//TTL(40sec)
    memset(&timer, 0, sizeof(timer));
	
	struct addrinfo hints, *res;
	struct sockaddr_in skt;
	struct dhcph message;
	int sktlen = sizeof (skt);
	fd_set rfds;
	serv =  DHCP_PORT;

	if(argc != 2){
		fprintf(stderr, "ERORR: ./mydhcpc server-IP-address");
		exit(1);
	}

	strncpy(host, argv[1], BUF_LEN - 1);
	host[strlen(host)] = '\0';

	memset(&hints, 0, sizeof(hints));
	hints.ai_protocol = 0;
	hints.ai_family = AF_INET;
	//hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_DGRAM;

	if ((err = getaddrinfo(host, serv, &hints, &res)) < 0) {
           fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		   exit(1);
	}

	if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
		perror("socket");
		exit(1);
	}

	client_stat = C_INIT;
	message.type = DHCPINIT;

	signal(SIGINT, client_end);
	
	for(;;){
		signal(SIGALRM, alrmfunc);
	
		switch (client_stat) {

		case C_INIT:
			printf("\nSTAT : C_INIT(0)\n");
			//DISCOVER send
			set_dhcph(&message, DHCPDISCOVER, 0, 0, 0, 0);
			if(sendto(sd, (char *)&message, 
					  sizeof(message), 0, res->ai_addr, res->ai_addrlen) < 0){
				perror("ERROR: sendto");
				exit(1);
			}
			print_dhcph(&message, 'S');
			client_stat = C_WAIT_OFFER;
			printf("STAT_CHANGE : C_INIT(0) -> C_WAIT_OFFER(1)\n");
			break;
		case C_WAIT_OFFER:
			printf("\nSTAT : C_WAIT_OFFER(1)\n");
			timeout.tv_sec = TIMEOUT;
			timeout.tv_usec = 0.0;
			FD_ZERO(&rfds);
			FD_SET(sd, &rfds);
			if((s_num = select(sd + 1, &rfds, NULL, NULL, &timeout)) < 0){
				perror("ERROR: select");
				exit(1);
			}else if(s_num == 0){
				fprintf(stderr, "Recvfrom : Timeout_10sec\n");
				client_stat = C_INIT;
				printf("STAT_CHANGE: C_WAIT_OFFER(1) -> C_INIT(0)\n");
				s1_count++;
				if(s1_count == 3){
					fprintf(stderr, "Timeout_3times : Exit(1)\n");
					exit(1);
				}
			}
			if(FD_ISSET(sd, &rfds)){
				//OFFER recv
				if(datalen = recvfrom(sd, (char *)&message, sizeof(message), 
									  0, (struct sockaddr *)&skt, &sktlen) < 0){
					perror("ERROR: revfrom");
					exit(1);
				}
				print_dhcph(&message, 'C');
				if(message.type == DHCPOFFER && message.code == CODE_0){
					//for(;;);
					//REQUEST 10 send
					set_dhcph(&message, DHCPREQUEST, CODE_10, TTL, -1, -1);
					if(sendto(sd, (char *)&message, 
							  sizeof(message), 0, 
							  res->ai_addr, res->ai_addrlen) < 0){
						perror("ERROR: sendto");
						exit(1);
					}
					print_dhcph(&message, 'S');
					client_stat = C_WAIT_ACK;
					printf("STAT_CHANGE : C_WAIT_OFFER(1) -> C_WAIT_ACK(2)\n");
				}else if(message.type == DHCPOFFER && message.code == CODE_129){
					fprintf(stderr, "CODE 129\n");
					client_stat = C_INIT;
					retry_flag++;
					if(retry_flag == 3){
						fprintf(stderr, "Sendto DHCPDISCOVER_3times: Exit(1)\n");
						exit(1);
					}
					printf("STAT_CHANGE : C_WAIT_OFFER(1) -> C_INIT(0)\n");
				}else{
					fprintf(stderr, "ERROR: Wrong type message / Wrong code message\n");
					exit(1);
				}
			}
			break;
		case C_WAIT_ACK:
			printf("\nSTAT : C_WAIT_ACK(2)\n");
			if(alloc_flag == 1){
				//REQUEST 11 send
				fprintf(stderr, "Re-allocate IP-address and Netmask\n");
				set_dhcph(&message, DHCPREQUEST, CODE_11, TTL, -1, -1);
				if(sendto(sd, (char *)&message, 
						  sizeof(message), 0, 
						  res->ai_addr, res->ai_addrlen) < 0){
					perror("ERROR: sendto");
					exit(1);
				}
				print_dhcph(&message, 'S');
				alloc_flag = 0;
			}
			
			timeout.tv_sec = TIMEOUT;
			timeout.tv_usec = 0.0;
			FD_ZERO(&rfds);
			FD_SET(sd, &rfds);
			if((s_num = select(sd + 1, &rfds, NULL, NULL, &timeout)) < 0){
				perror("ERROR: select");
				exit(1);
			}else if(s_num == 0){
				fprintf(stderr, "Recvfrom : Timeout_10sec\n");
				client_stat = C_INIT;
				printf("STAT_CHANGE : C_WAIT_ACK(2) -> C_INIT(0)\n");
				s2_count++;
				if(s2_count == 3){
					fprintf(stderr, "Timeout_3times : Exit(1)\n");
					exit(1);
				}
			}
			if(FD_ISSET(sd, &rfds)){
				//ACK recv
				if(datalen = recvfrom(sd, (char *)&message, sizeof(message), 
									  0, (struct sockaddr *)&skt, &sktlen) < 0){
					perror("ERROR: revfrom");
					exit(1);
				}
				print_dhcph(&message, 'C');
				if(message.type == DHCPREPLY && message.code == CODE_0){
					client_stat = C_GET_IP;
					if(alloc_flag2++ == 0){
						printf("Allocate IP-address and Netmask\n");
						printf("\tIP-address: ");
						convert_addr(message.address);
						printf("\n");
						printf("\tNetmask: ");
						convert_addr(message.netmask);
						printf("\n");
						printf("\tTime to live: %d\n", message.ttl);
					}
					printf("STAT_CHANGE : C_WAIT_ACK(2) -> C_GET_IP(3)\n");
				}else if(message.type == DHCPREPLY && message.code == CODE_130){
					fprintf(stderr, "CODE 130\n");
					client_stat = C_INIT;
					printf("STAT_CHANGE : C_WAIT_ACK(2) -> C_INIT(0)\n");
				}else{
					fprintf(stderr, "ERROR: Wrong type message / Wrong code message\n");
					exit(1);
				}
			}
			break;
		case C_GET_IP:
			printf("\nSTAT : C_GET_IP(3)\n");
			//for(;;);
			//if(test_flag++ != 0)
				//for(;;);
			timer.it_value.tv_sec = TTL / 2.0;
			timer.it_value.tv_usec = 0.0;
			setitimer(ITIMER_REAL, &timer, NULL);
			pause();
			break;
		case C_END:
			printf("\nSTAT : C_END(4)\n");
			//RELEASE send
			set_dhcph(&message, DHCPRELEASE, 0, 0, -1, 0);
			if(sendto(sd, (char *)&message, 
				  sizeof(message), 0, 
				  res->ai_addr, res->ai_addrlen) < 0){
				perror("ERROR: sendto");
				exit(1);
			}
			print_dhcph(&message, 'S');
			client_stat = -1;
			close(sd);
			break;
		}
		if(client_stat == -1)
			break;
	}
	return 0;
}
