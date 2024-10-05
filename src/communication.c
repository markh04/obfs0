#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include <pthread.h>

#include "utils.h"

static const char client_http_request[] =
	"GET / HTTP/1.1\r\n"
	"Host: example.com\r\n"
	"Accept-Encoding: gzip\r\n"
	"ETag: ";

static const char ok_http_answer[] = {
	"HTTP/1.1 200 OK\r\n"
	"Content-Encoding: gzip\r\n"
	"Content-Length: "
};

static const char redirect_http_answer[] = 
	"HTTP/1.1 301 Moved permanently\r\n"
	"Location: https://example.com\r\n"
	"Content-Length: 53\r\n"
	"\r\n"
	"<a href=\"https://example.com\">https://example.com</a>";

// static const char http_200_answer[] =
// 	"HTTP/1.1 200 OK\r\n"
// 	"Content-Length: 5\r\n"
// 	"\r\n"
// 	"hello";

void log_msg(struct thread_args * args, const char * msg) {
	print_addr(args->listen_addr, args->listen_port);
	printf(" - %s", msg);
	if(errno) {
		printf(": %s", strerror(errno));
	}
	printf("\n");
}

void * communicate(void *);

void start_listening(struct proc_args * args) {
	int server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(server_socket < 0) {
		perror("socket");
		exit(-1);
	}
	unsigned y = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &y, sizeof y);
	struct sockaddr_in srv_addr;
	memset(&srv_addr, '\0', sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(args->listen_port);
	srv_addr.sin_addr.s_addr = htonl(args->listen_addr);

	if(bind(server_socket, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) < 0) {
		perror("bind");
		exit(-1);
	}

	listen(server_socket, BACKLOG);

	for(;;) {
		struct sockaddr_in sa;
		int sa_len = sizeof(sa);

		int s = accept(server_socket, (struct sockaddr *) &sa, &sa_len);
		if(s < 0) {
			perror("accept");
			continue;
		}

		struct thread_args * thr_args = malloc(sizeof(struct thread_args));
		thr_args->listen_addr = ntohl(sa.sin_addr.s_addr);
		thr_args->listen_port = ntohs(sa.sin_port);
		thr_args->target_addr = args->target_addr;
		thr_args->target_port = args->target_port;
		thr_args->mode = args->mode;
		thr_args->in_socket = s;
		thr_args->out_socket = s;

		pthread_t thr;
		pthread_create(&thr, NULL, communicate, thr_args);
		pthread_detach(thr);
	}

}

int client_handshake(int s, struct protocol_args * proto_args);
int server_handshake(int s, struct protocol_args * proto_args);

void xor_buf(char * buf, int len, char * key, int * cnt);

void * communicate(void * _args) {

	struct protocol_args proto_args;

	struct thread_args * args = (struct thread_args *) _args;
	int success;

	args->out_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(args->out_socket < 0) {
		log_msg(args, "socket");
		goto communication_end;
	}
	struct sockaddr_in sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(args->target_port);
	sa.sin_addr.s_addr = htonl(args->target_addr);

	if(connect(args->out_socket, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		log_msg(args, "connect");
		goto communication_end;
	}

	log_msg(args, "Starting connection");

	success = (args->mode == SERVER)
		? server_handshake(args->in_socket, &proto_args)
		: client_handshake(args->out_socket, &proto_args);

	if(!success) {
		if(args->mode == SERVER) {
			write(args->in_socket, redirect_http_answer, sizeof(redirect_http_answer)-1);
		}
		goto communication_end;
	}

	printf("Content-Length: %d\n", proto_args.content_length);
	printf("Key: %08x\n", *((unsigned int*) &proto_args.key));

	int in_cnt = 0, out_cnt = 0;

	log_msg(args, "Starting communication");

	struct pollfd fds[2];
	fds[0].fd = args->in_socket,
	fds[0].events = POLLIN | POLLERR | POLLHUP;
	fds[1].fd = args->out_socket,
	fds[1].events = POLLIN | POLLERR | POLLHUP;
	char in_buf[BUF_SIZE];
	char out_buf[BUF_SIZE];

	for(;;) {
		int r = poll(fds, 2, 60000);
		if(r < 0) {
			log_msg(args, "poll");
			continue;
		}
		if(r == 0) break;
		if((fds[0].revents | fds[1].revents) & POLLHUP) {
			log_msg(args, "Connection reset");
			goto communication_end;
		}
		if((fds[0].revents | fds[1].revents) & POLLERR) {
			log_msg(args, "Connection error");
			goto communication_end;
		}
		if(fds[0].revents & POLLIN) {
			int l = read(args->in_socket, in_buf, BUF_SIZE);
			if(l <= 0) break;
			xor_buf(in_buf, l, proto_args.key, &in_cnt);
			write(args->out_socket, in_buf, l);
		}
		if(fds[1].revents & POLLIN) {
			int l = read(args->out_socket, out_buf, BUF_SIZE);
			if(l <= 0) break;
			xor_buf(out_buf, l, proto_args.key, &out_cnt);
			write(args->in_socket, out_buf, l);
		}
	}



communication_end:

	log_msg(args, "Closing connection");

	close(args->in_socket);
	if(args->out_socket > 0) close(args->out_socket);
	free(args);
	return NULL;
}

void xor_buf(char * buf, int len, char * key, int * cnt) {
	int i = 0;
	while(i < len) {
		buf[i] ^= key[(*cnt) % KEY_SIZE];
		i++;
		(*cnt)++;
	}
}

void gen_key(char * key, int len) {
	for(int i = 0; i < len; i++) {
		int c = rand() % 16;
		if(c < 10) {
			key[i] = '0' + c;
		} else {
			key[i] = 'a' + (c - 10);
		}
	}
}

int decode_key(char * buf, char * key) {
	for(int i = 0; i < 4; i++) {
		if('0' <= key[2*i] && key[2*i] <= '9') {
			buf[i] = (key[2*i] - '0') << 4;
		} else if('a' <= key[2*i] && key[2*i] <= 'f') {
			buf[i] = (key[2*i] - 'a' + 10) << 4;
		} else {
			return 0;
		}
		if('0' <= key[2*i+1] && key[2*i+1] <= '9') {
			buf[i] += (key[2*i+1] - '0');
		} else if('a' <= key[2*i+1] && key[2*i+1] <= 'f') {
			buf[i] += (key[2*i+1] - 'a' + 10);
		} else {
			return 0;
		}
	}
	return 1;
}

int client_handshake(int s, struct protocol_args * proto_args) {
	write(s, client_http_request, sizeof(client_http_request)-1);
	char key[KEY_SIZE*2];
	gen_key(key, sizeof(key));
	write(s, key, sizeof(key));
	decode_key(proto_args->key, key);
	int l = write(s, "\r\n\r\n", 4);
	if(l <= 0) return 0;
	char buf[BUF_SIZE];
	l = read(s, buf, BUF_SIZE);
	int sl = sizeof(ok_http_answer)-1;
	if(l <= sl) {
		return 0;
	}
	sscanf(
		buf+sl,
		"%d", &proto_args->content_length
	);
	return 1;
}

int server_handshake(int s, struct protocol_args * proto_args) {
	char in_buf[BUF_SIZE];
	int l = read(s, in_buf, sizeof(client_http_request)-1);
	if(l != sizeof(client_http_request)-1) return 0;
	if(strncmp(in_buf, client_http_request, l)) return 0;
	char key[KEY_SIZE*2];
	memset(key, '\0', KEY_SIZE*2);
	read(s, key, KEY_SIZE*2);
	write(1, key, KEY_SIZE*2);
	printf("\n");
	if(!decode_key(proto_args->key, key)) {
		return 0;
	}
	read(s, in_buf, 4);
	if(strncmp(in_buf, "\r\n\r\n", 4)) {
		return 0;
	}
	proto_args->content_length = 
		MIN_CONTENT_LENGTH + 
		(rand() % (MAX_CONTENT_LENGTH - MIN_CONTENT_LENGTH));
	char cl[BUF_SIZE];
	sprintf(cl, "%s%d\r\n\r\n\x1f\x8b%n", ok_http_answer, proto_args->content_length, &l);
	write(s, cl, l);
	return 1;
}
