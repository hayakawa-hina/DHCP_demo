#include "def.h"

void set_dhcph(struct dhcph *message, uint8_t type, uint8_t code, uint16_t ttl, in_addr_t address, uint32_t netmask)
{
	message->type = type;
	message->code = code;
	message->ttl = ttl;
	message->address = (address == -1) ? message->address: address;
	message->netmask = (netmask == -1) ? message->netmask: netmask;
}

void print_dhcph(struct dhcph *message, char type)
{
	if(type == 'S'){
		printf("Sendto ");
	}else if(type == 'C'){
		printf("Recvfrom ");
	}
	printf("Message\n");
	printf("\tType: %u\tCode: %u\n", message->type, message->code);
	printf("\tTime to Live: %d\n", message->ttl);
	printf("\tIP address: %u\tNetmask: %u\n", message->address, message->netmask);
}

void init_struct_list()
{
		cf_head.fp = cf_head.bp = &cf_head;
		cf_head.listno = -1;
		cf_head.use = -1;
		
		ct_head.fp = ct_head.bp = &ct_head;
		ct_head.listno = -1;
		
		timer_ct_head.tout_fp = timer_ct_head.tout_bp = &timer_ct_head;
		timer_ct_head.listno = -1;
		timer_ct_head.exp_time = 0;
}

void insert_cf(struct config_list *p, struct config_list *n)
{
	n->bp = p;
	n->fp = p->fp;
	(p->fp)->bp = n;
	p->fp = n;
}

void delete_cf(struct config_list *p)
{
	(p->bp)->fp = p->fp;
	(p->fp)->bp = p->bp;
}

void insert_ct(struct clitab *p, struct clitab *n)
{
	n->bp = p;
	n->fp = p->fp;
	(p->fp)->bp = n;
	p->fp = n;
}
void delete_ct(struct clitab *p)
{
	(p->bp)->fp = p->fp;
	(p->fp)->bp = p->bp;
}

void insert_time(struct clitab *p, struct clitab *n)
{
	n->tout_bp = p;
	n->tout_fp = p->tout_fp;
	(p->tout_fp)->tout_bp = n;
	p->tout_fp = n;
}

void delete_time(struct clitab *p)
{
	(p->tout_bp)->tout_fp = p->tout_fp;
	(p->tout_fp)->tout_bp = p->tout_bp;
}

void use_cf()
{
	struct config_list *tmp;
	tmp = cf_head.fp;
	delete_cf(tmp);
	insert_cf(cf_head.bp, tmp);
}

void convert_addr(uint32_t addr)
{
	struct in_addr s;
	unsigned char tmp[5];
	tmp[4] = '\0';
	memcpy(tmp, &addr, sizeof(uint32_t));	
	s.s_addr = *((unsigned long*)tmp);
	printf("%s", inet_ntoa(s));
}

struct config_list *search_cf(struct dhcph *msg)
{
	struct config_list *p;
	for (p = cf_head.fp; p != &cf_head; p = p->fp)
		if(inet_addr(p->address) == msg->address)
			return p;
	return NULL;
}
struct clitab *search_ct(struct in_addr address)
{
	struct clitab *p;
	for (p = ct_head.fp; p != &ct_head; p = p->fp)
		if(p->cli_addr.s_addr == address.s_addr)
			return p;
	return NULL;
}
//専用関数_クソ
struct config_list *search_cf_t(in_addr_t address)
{
	struct config_list *p;
	for (p = cf_head.fp; p != &cf_head; p = p->fp)
		if(inet_addr(p->address) == address)
			return p;
	return NULL;
}

struct clitab *search_alarm(struct in_addr address)
{
	struct clitab *p;
	for (p = timer_ct_head.tout_fp->tout_fp; p != &timer_ct_head; p = p->tout_fp)
		if(p->cli_addr.s_addr == address.s_addr)
			return p;
	return NULL;
}

int insert_time_sort(struct clitab *ct)
{
	struct clitab *p;
	for (p = timer_ct_head.tout_fp; p != &timer_ct_head; p = p->tout_fp){
		//printf("alarm_sort: %d\n", p->exp_time);
		if(p->exp_time > ct->exp_time){
			//printf("IN_alarm_sort: %d\n", p->exp_time);
			insert_time(p->tout_bp, ct);
			return 0;
		}
	}
	//printf("IN_alarm_sort: %d\n", p->exp_time);
	insert_time(timer_ct_head.tout_bp, ct);
	return -1;
}


