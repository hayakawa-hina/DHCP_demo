#include "def.h"

int sd;
struct sockaddr_in skt;
struct clitab *now_alarm = NULL;
int timer_first_flag = 0;
struct itimerval timer;
int err_times[3] = {0, 0, 0};

void alarm_start(double sec){
	timer.it_value.tv_sec =  sec;
	timer.it_value.tv_usec = 0.0;
	setitimer(ITIMER_REAL, &timer, NULL);
}

void alarm_stop(){
	timer.it_value.tv_sec = 0.0;
	timer.it_value.tv_usec = 0.0;
	setitimer(ITIMER_REAL, &timer, NULL);
}

//stat_0
void f_stat0_err(struct clitab *ct, struct dhcph *message)
{
	fprintf(stderr, "ERROR: S_INIT\n");
	exit(1);
}

//stat_1
void f_stat1_err(struct clitab *ct, struct dhcph *message)
{
	printf("\nSTAT : S_WAIT_DSC(1)\n");
	fprintf(stderr, "ERROR: NOT DHCPDISCOVER\n");
	print_dhcph(message, 'C');
	if(err_times[0]++ == 3)
		ct->stat = S_END;
}

void f_stat1_msg1(struct clitab *ct, struct dhcph *message)
{
	printf("\nSTAT : S_WAIT_DSC(1)\n");
	print_dhcph(message, 'C');
	if(cf_head.fp != &cf_head){
		set_dhcph(message, DHCPOFFER, CODE_0, 40, 
			  inet_addr(cf_head.fp->address), 
			  inet_addr(cf_head.fp->netmask));
		ct->alloc_addr = inet_addr(cf_head.fp->address);
		ct->netmask = inet_addr(cf_head.fp->netmask);
		//cf_head.fp->use = 1;
		//use_cf();		
		ct->stat = S_WAIT_REQ;
	}else{
		fprintf(stderr, "All IP-address used\n");
		set_dhcph(message, DHCPOFFER, CODE_129, 40, 0, 0);
	}
	//DHCPOFFER send 
	if(sendto(sd, (char *)message, sizeof(*message), 
			  0, (struct sockaddr *)&skt, sizeof(skt)) < 0){
		perror("ERROR: sendto");
		exit(1);
	}
	print_dhcph(message, 'S');
	if(ct->stat == S_WAIT_REQ){
		printf("STAT_CHANGE : S_WAIT_DSC(1) -> S_WAIT_REQ(2)\n");
		struct timeval timeout;
		gettimeofday(&timeout, NULL);
		ct->start_time = (int)(timeout.tv_sec);
		ct->exp_time = (int)(timeout.tv_sec + TIMEOUT);
		insert_time_sort(ct);
		if(timer_first_flag++ == 0){
			//printf("standard_alarm\n");
			now_alarm = ct;
			alarm_start(TIMEOUT);
			//printf("alarm_start(%d)\n", (int)TIMEOUT);
		}else if(now_alarm->exp_time > ct->exp_time){
			alarm_stop();
			//printf("interrupt_alarm\n");
			now_alarm = ct;
			alarm_start(TIMEOUT);
			//printf("alarm_start(%d)\n", (int)TIMEOUT);
		}
		/*
		struct clitab *pp;
		for(pp = timer_ct_head.tout_fp; pp != &timer_ct_head; pp = pp->tout_fp){
			printf("alarm_tab\tIP:: %s\texp_time:: %d\n", inet_ntoa(pp->cli_addr), pp->exp_time);
		}
		*/
	}
}

//stat_2
void f_stat2_err(struct clitab *ct, struct dhcph *message)
{
	printf("\nSTAT : S_WAIT_REQ(2)\n");
	fprintf(stderr, "ERROR: NOT DHCPREQUEST\n");
	print_dhcph(message, 'C');
	if(err_times[1]++ == 3)
		ct->stat = S_END;
}
void f_stat2_msg3(struct clitab *ct, struct dhcph *message)
{
	if(now_alarm != NULL){
		struct clitab *p;
		if(ct->cli_addr.s_addr == now_alarm->cli_addr.s_addr){
			alarm_stop();			
			/*
			printf("\nRecvfrom : No_timeout_10sec : IP-address ");
			convert_addr(now_alarm->cli_addr.s_addr);
			printf("\n");
			*/
			struct clitab *next_alarm;
			next_alarm = NULL;
			if(now_alarm->tout_fp != &timer_ct_head){
				struct timeval now_time;
				gettimeofday(&now_time, NULL);
				next_alarm = now_alarm->tout_fp;
				alarm_start((double)(next_alarm->exp_time - (int)now_time.tv_sec));
				//printf("alarm_start(%d)\n", next_alarm->exp_time - (int)now_time.tv_sec);
			}
			delete_time(now_alarm);
			now_alarm = next_alarm;
			if(timer_ct_head.tout_fp == &timer_ct_head)
				timer_first_flag = 0;
		}else if((p = search_alarm(ct->cli_addr)) != NULL){
			delete_time(p);
		}
	}

	printf("\nSTAT : S_WAIT_REQ(2)\n");
	print_dhcph(message, 'C');
	struct config_list *p;
	p = search_cf(message);
	if(p == NULL){
		fprintf(stderr, "This IP-address used\n");
		set_dhcph(message, DHCPREPLY, CODE_130, TTL, -1, -1);
	}else{
		set_dhcph(message, DHCPREPLY, CODE_0, TTL, -1, -1);
		/*
		p->use = 1;
		use_cf();
		*/
		ct->alloc = p;		
		delete_cf(p);
		
		printf("Allocate IP-address and Netmask\n");
		printf("\tIP-address: ");
		convert_addr(message->address);
		printf("\n");
		printf("\tNetmask: ");
		convert_addr(message->netmask);
		printf("\n");
		printf("\tTime to live: %d\n", message->ttl);
		/*
		struct config_list *ppp;
		for(ppp = cf_head.fp; ppp != &cf_head; ppp = ppp->fp){
			printf("IP:: %s\tNETMASK:: %s\tuse: %d\n", ppp->address, ppp->netmask, ppp->use);
		}
*/
		struct timeval timeout;
		gettimeofday(&timeout, NULL);
		ct->start_time = (int)(timeout.tv_sec);
		ct->exp_time = (int)(timeout.tv_sec + TTL);
		insert_time_sort(ct);
		if(timer_first_flag++ == 0){
			//printf("standard_alarm\n");
			now_alarm = ct;
			alarm_start(TTL);
			//printf("alarm_start(%d)\n", (int)TTL);
		}else if(now_alarm->exp_time > ct->exp_time){
			insert_time_sort(ct);
			alarm_stop();
			//printf("interrupt alarm\n");
			now_alarm = ct;
			alarm_start(TTL);
			//printf("alarm_start(%d)\n", (int)TTL);
		}
		/*
		struct clitab *pp;
		for(pp = timer_ct_head.tout_fp; pp != &timer_ct_head; pp = pp->tout_fp){
			printf("alarm_tab\tIP:: %s\texp_time:: %d\n", inet_ntoa(pp->cli_addr), pp->exp_time);
		}
		*/
		
		ct->stat = S_ALLOC_IP;
	}
	//DHCPREPLY send 
	if(sendto(sd, (char *)message, sizeof(*message), 
			  0, (struct sockaddr *)&skt, sizeof(skt)) < 0){
		perror("ERROR: sendto");
		exit(1);
	}
	print_dhcph(message, 'S');
	if(ct->stat == S_ALLOC_IP)
		printf("STAT_CHANGE : S_WAIT_REQ(2) -> S_ALLOC_IP(3)\n");
}

//stat_3
void f_stat3_err(struct clitab *ct, struct dhcph *message)
{
	printf("\nSTAT : S_ALLOC_IP(3)\n");
	print_dhcph(message, 'C');
	if(err_times[2]++ == 3)
		ct->stat = S_END;
}
void f_stat3_msg3(struct clitab *ct, struct dhcph *message)
{
	printf("\nSTAT : S_ALLOC_IP(3)\n");
	if(message->code == CODE_11){
	
		if(now_alarm != NULL){
			struct clitab *p;
			if(ct->cli_addr.s_addr == now_alarm->cli_addr.s_addr){
				alarm_stop();
				/*
				printf("\nRecvfrom : No_timeout_40sec : IP-address ");
				convert_addr(now_alarm->cli_addr.s_addr);
				printf("\n");
				*/
				struct clitab *next_alarm;
				next_alarm = NULL;
				if(now_alarm->tout_fp != &timer_ct_head){
					struct timeval now_time;
					gettimeofday(&now_time, NULL);
					next_alarm = now_alarm->tout_fp;
					alarm_start((double)(next_alarm->exp_time - (int)now_time.tv_sec));
					//printf("alarm_start(%d)\n", next_alarm->exp_time - (int)now_time.tv_sec);
				}
				delete_time(now_alarm);
				now_alarm = next_alarm;
				if(timer_ct_head.tout_fp == &timer_ct_head)
					timer_first_flag = 0;
			}else if((p = search_alarm(ct->cli_addr)) != NULL){
				delete_time(p);
			}
		}
	
		print_dhcph(message, 'C');
		
		set_dhcph(message, DHCPREPLY, CODE_0, TTL, -1, -1);
		//DHCPREPLY send 
		if(sendto(sd, (char *)message, sizeof(*message), 
				  0, (struct sockaddr *)&skt, sizeof(skt)) < 0){
			perror("ERROR: sendto");
			exit(1);
		}
		print_dhcph(message, 'S');
		
		struct timeval timeout;
		gettimeofday(&timeout, NULL);
		ct->start_time = (int)(timeout.tv_sec);
		ct->exp_time = (int)(timeout.tv_sec + TTL);
		insert_time_sort(ct);
		if(timer_first_flag++ == 0){
			//printf("standard_alarm\n");
			now_alarm = ct;
			alarm_start(TTL);
			//printf("alarm_start(%d)\n", (int)TTL);
		}else if(now_alarm->exp_time > ct->exp_time){
			insert_time_sort(ct);
			alarm_stop();
			//printf("interrupt alarm\n");
			now_alarm = ct;
			alarm_start(TTL);
			//printf("alarm_start(%d)\n", (int)TTL);
		}
		/*
		struct clitab *pp;
		for(pp = timer_ct_head.tout_fp; pp != &timer_ct_head; pp = pp->tout_fp){
			printf("alarm_tab\tIP:: %s\texp_time:: %d\n", inet_ntoa(pp->cli_addr), pp->exp_time);
		}
		*/
	}else{
		fprintf(stderr, "ERROR: NOT CODE_11\n");		
		print_dhcph(message, 'C');
	}
}
void f_stat34_msg5(struct clitab *ct, struct dhcph *message)
{
	printf("\nSTAT : S_ALLOC_IP(3)\n");
	print_dhcph(message, 'C');
	
	if(now_alarm != NULL){
		struct clitab *p;
		if(ct->cli_addr.s_addr == now_alarm->cli_addr.s_addr){
			alarm_stop();
				/*
			printf("Recvfrom : No_timeout_40sec : IP-address ");
			convert_addr(now_alarm->cli_addr.s_addr);
			printf("\n");
			*/
			struct clitab *next_alarm;
			next_alarm = NULL;
			if(now_alarm->tout_fp != &timer_ct_head){
				struct timeval now_time;
				gettimeofday(&now_time, NULL);
				next_alarm = now_alarm->tout_fp;
				alarm_start((double)(next_alarm->exp_time - (int)now_time.tv_sec));
				//printf("alarm_start(%d)\n", next_alarm->exp_time - (int)now_time.tv_sec);
			}
			delete_time(now_alarm);
			now_alarm = next_alarm;
			if(timer_ct_head.tout_fp == &timer_ct_head)
				timer_first_flag = 0;
		}else if((p = search_alarm(ct->cli_addr)) != NULL){
			delete_time(p);
		}
	}
	
	ct->stat = S_END;
	printf("STAT_CHANGE : S_ALLOC_IP(3) -> S_END(4)\n");
}

//functable
void(*functab[4][6])(struct clitab *, struct dhcph *) = {
	{f_stat0_err, f_stat0_err, f_stat0_err, f_stat0_err, f_stat0_err, f_stat0_err},
	{f_stat1_err, f_stat1_msg1, f_stat1_err, f_stat1_err, f_stat1_err, f_stat1_err},
	{f_stat2_err, f_stat2_err, f_stat2_err, f_stat2_msg3, f_stat2_err, f_stat2_err},
	{f_stat3_err, f_stat3_err, f_stat3_err, f_stat3_msg3, f_stat3_err, f_stat34_msg5}
}; 

void alrmfunc(int sig){
	alarm_stop();
	
	if(now_alarm->stat == S_WAIT_REQ)
		printf("\n\nRecvfrom : Timeout_10sec : IP-address ");
	else
		printf("\n\nRecvfrom : Timeout_40sec : IP-address ");	
	convert_addr(now_alarm->cli_addr.s_addr);
	printf("\n");
	

	struct clitab *next_alarm;
	next_alarm = NULL;
	if(now_alarm->tout_fp != &timer_ct_head){
		next_alarm = now_alarm->tout_fp;
		alarm_start((double)(next_alarm->exp_time - now_alarm->exp_time));
		//printf("alarm_start(%d)\n", next_alarm->exp_time - now_alarm->exp_time);
	}

	delete_time(now_alarm);
	now_alarm->stat = S_END;
	printf("\nSTAT : S_END(4)\n");
	
	printf("Release IP-address ");
	convert_addr(now_alarm->cli_addr.s_addr);
	printf("\n");
	/*
	struct config_list *p;
	p = search_cf_t(now_alarm->alloc_addr);
	p->use = 0;
	*/
	if(now_alarm->alloc != NULL){
		insert_cf(cf_head.bp, now_alarm->alloc);
		now_alarm->alloc = NULL;
	}
	/*
	struct config_list *ppp;
		for(ppp = cf_head.fp; ppp != &cf_head; ppp = ppp->fp){
			printf("IP:: %s\tNETMASK:: %s\tuse: %d\n", ppp->address, ppp->netmask, ppp->use);
	}
	*/
	printf("Remove from client table\n");
	delete_ct(now_alarm);
	free(now_alarm);
	now_alarm = next_alarm;
	if(timer_ct_head.tout_fp == &timer_ct_head)
		timer_first_flag = 0;
}

int main(int argc, char *argv[])
{
	in_port_t port = 51230;
	int datalen, s_num;
	struct dhcph message;
	struct sockaddr_in myskt;
	int sktlen = sizeof (skt);
	FILE *fp;
	char readline[BUF_LEN];
	struct config_list *cf;
	struct clitab *ct;
	fd_set fds;

	init_struct_list();
	memset(&timer, 0, sizeof(timer));
	//fopen config-file
	if((fp = fopen(argv[1], "r")) == NULL){
		perror("ERROR: fopen");
		exit(1);
	}
	while(fgets(readline, BUF_LEN, fp) != NULL){
		//printf("%s", readline);
		char *ip, *netmask;
		ip = strtok(readline, " \t\n");
		netmask = strtok(NULL, " \t\n");
	   	//printf("IP: %s\tNETMASK: %s\n", ip, netmask);
		struct config_list *tmp;
		if((tmp = malloc(sizeof(struct config_list))) == NULL){
			printf("Memory allocation error\n");
			exit(1);
		}
		tmp->listno = 0;
		tmp->use = 0;
		strcpy(tmp->address, ip);
		strcpy(tmp->netmask, netmask);
		insert_cf(cf_head.bp, tmp);
	}
	/*
	struct config_list *p;
	for(p = cf_head.fp; p != &cf_head; p = p->fp){
		printf("IP:: %s\tNETMASK:: %s\n", p->address, p->netmask);
	}
	*/
	if((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("ERROR: socket");
		exit(1);
	}

	memset(&myskt, 0, sizeof myskt);
	myskt.sin_family = AF_INET;
	myskt.sin_port = htons(port);
	myskt.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(sd, (struct sockaddr *)&myskt, sizeof myskt) < 0){
		perror("ERROR: bind");
		exit(1);
	}
	
	FD_ZERO(&fds); 
	FD_SET(sd, &fds);

	printf("\nSTAT : S_INIT(0)\n");
	printf("STAT_CHANGE : S_INIT(0) -> S_WAIT_DSC(1)\n");
	
	for(;;){
		signal(SIGALRM, alrmfunc);
	
		if((s_num = select(sd + 1, &fds, NULL, NULL, NULL)) < 0){
			//perror("ERROR: select");
			//exit(1);
			;
		}
		if(FD_ISSET(sd, &fds)){
			if((datalen = recvfrom(sd, (char *)&message, sizeof message,
				   0, (struct sockaddr *)&skt, &sktlen)) < 0){
				perror("ERROR: recvfrom");
				exit(1);
			}
			struct clitab *ct_check;
			if((ct_check = search_ct(skt.sin_addr)) == NULL){//port
				printf("\nCreate new client\n");
				struct clitab *tmp;
				if((tmp = malloc(sizeof(struct clitab))) == NULL){
					printf("Memory allocation error\n");
					exit(1);
				}
				tmp->listno = 0;
				tmp->stat = S_WAIT_DSC;
				tmp->alloc_addr = 0;
				tmp->alloc = NULL;
				tmp->netmask = 0;
				tmp->cli_addr = skt.sin_addr;
				tmp->cli_port = ntohs(skt.sin_port);
				insert_ct(ct_head.bp, tmp);
				ct_check = tmp;
				//printf("%s\t%d\n", inet_ntoa(ct_check->cli_addr), ct_check->cli_port);
			}else{
				//printf("\nThis Client Exist\n");
				;
			}
			/*
			struct clitab *pp;
			for(pp = ct_head.fp; pp != &ct_head; pp = pp->fp){
				printf("Clitab\tIP:: %s\n", inet_ntoa(pp->cli_addr));
			}
			*/
			//printf("message.type: %d\tct->stat: %d\n", message.type, ct_check->stat);
			functab[ct_check->stat][message.type](ct_check, &message);
			
			//stat_4
			if(ct_check->stat == S_END){
				printf("\nSTAT : S_END(4)\n");
				printf("Release IP-address %u\n", ct_check->alloc_addr);
				/*
				struct config_list *p;
				p = search_cf(&message);
				p->use = 0;
				*/
				if(ct_check->alloc != NULL){
					insert_cf(cf_head.bp, ct_check->alloc);
					ct_check->alloc = NULL;
				}
				/*
				struct config_list *ppp;
				for(ppp = cf_head.fp; ppp != &cf_head; ppp = ppp->fp){
					printf("IP:: %s\tNETMASK:: %s\tuse: %d\n", ppp->address, ppp->netmask, ppp->use);
				}
				*/
				printf("Remove from client table\n");
				delete_ct(ct_check);
				free(ct_check);
			}
		}
	}
	close(sd);
	return 0;
}
