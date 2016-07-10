#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#define DHCPINIT 0
#define DHCPDISCOVER 1
#define DHCPOFFER 2
#define DHCPREQUEST 3
#define DHCPREPLY 4
#define DHCPRELEASE 5 

#define C_INIT 0
#define C_WAIT_OFFER 1
#define C_WAIT_ACK 2
#define C_GET_IP 3
#define C_END 4

#define S_INIT 0
#define S_WAIT_DSC 1
#define S_WAIT_REQ 2
#define S_ALLOC_IP 3
#define S_END 4

#define TTL 40.0
#define TIMEOUT 10.0

#define CODE_10 10
#define CODE_129 129
#define CODE_11 11
#define CODE_0 0
#define CODE_130 130

#define DHCP_PORT "51230"
#define BUF_LEN 256
#define NO_CHANGE -1

struct config_list {
	int listno;
	struct config_list *fp;
	struct config_list *bp;
	char address[256];
	char netmask[256];
	int use;
};


struct clitab {
	int listno;
	struct clitab *fp;
	struct clitab *bp;
	struct clitab *tout_fp;
	struct clitab *tout_bp;
	int stat;
	int start_time;
	int exp_time;
	struct in_addr cli_addr;//IP-address client
	uint16_t cli_port;
	in_addr_t alloc_addr;//割り当てられたIP-address
	struct config_list *alloc;
	uint32_t netmask;
};

struct dhcph {
	uint8_t type;
	uint8_t code;
	uint16_t ttl;
	in_addr_t address;
	uint32_t netmask;
};

struct config_list cf_head;
struct clitab ct_head;
struct clitab timer_ct_head;

void use_cf();
void insert_ct(struct clitab *, struct clitab *);
void delete_cf(struct config_list *);
void insert_cf(struct config_list *, struct config_list *);
void init_struct_list();
void print_dhcph(struct dhcph *, char );
void set_dhcph(struct dhcph *message, uint8_t type, uint8_t code, uint16_t ttl, in_addr_t address, uint32_t netmask);