#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"

void die(int status, const char * msg) {
	fprintf(stderr, "Error: %s\n", msg);
	exit(status);
}

int parse_addr(const char * addr, unsigned int * ip, int * port) {
	int a, b, c, d, p;
	a = b = c = d = p = -1;
	sscanf(addr, "%d.%d.%d.%d:%d", &a, &b, &c, &d, &p);
	if(a < 0 || a >= 256 || b < 0 || b >= 256 ||
		c < 0 || c >= 256 || d < 0 || d >= 256) {
		char msg[100];
		sprintf(msg, "Invalid ip %s", addr);
		die(1, msg);
	}
	if(p < 1 || p >= 65536) {
		char msg[100];
		sprintf(msg, "Invalid port %d", p);
		die(1, msg);
	}
	*ip = (a << 24) | (b << 16) | (c << 8) | d;
	*port = p;
}

void print_addr(unsigned int ip, int port) {
	printf("%d.%d.%d.%d:%d",
		(ip >> 24) & 0xFF,
		(ip >> 16) & 0xFF,
		(ip >> 8) & 0xFF,
		ip & 0xFF,
		port
	);
}

void process_args(int argc, char * argv[], struct proc_args * args) {
	memset(args, '\0', sizeof(struct proc_args));
	args->mode = -1;
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "--help")) {
			printf("Usage: %s [--server|--client] [--listen ip:port] target_addr\n", argv[0]);
			printf(
				"   --server     -  run as server (default)\n"
				"   --client     -  run as client\n"
				"   --listen     -  listen ip:port (default 0.0.0.0:80)\n"
				"   target_addr  -  target ip:port\n"
			);
			exit(0);
		}
		if(!strcmp(argv[i], "--version")) {
			printf(
				"obfs0\n"
				"version 0.1\n"
			);
			exit(0);
		}
		if(!strcmp(argv[i], "--server")) {
			if(args->mode != -1) {
				die(1, "--server or --client must be set once");
			}
			args->mode = SERVER;
			continue;
		}
		if(!strcmp(argv[i], "--client")) {
			if(args->mode != -1) {
				die(1, "--server or --client must be set once");
			}
			args->mode = CLIENT;
			continue;
		}
		if(!strcmp(argv[i], "--listen")) {
			if(i == argc-1 || argv[i+1][0] == '-') {
				die(1, "--listen flag must have an argument (default 0.0.0.0:80)");
			}
			if(args->listen_port != 0) {
				die(1, "--listen must be set once");
			}
			parse_addr(argv[++i], &args->listen_addr, &args->listen_port);
			continue;
		}
		if(args->target_port == 0) {
			parse_addr(argv[i], &args->target_addr, &args->target_port);
			continue;
		}
	}
	if(args->mode == -1) {
		args->mode = SERVER;
	}
	if(args->listen_port == 0) {
		args->listen_addr = 0;
		args->listen_port = 80;
	}
	if(args->target_port == 0) {
		die(1, "target_addr is necessary");
	}
}