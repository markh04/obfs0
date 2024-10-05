#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include "utils.h"

int main(int argc, char * argv[]) {

	signal(SIGPIPE, SIG_IGN);

	srand(time(NULL));

	struct proc_args args;
	process_args(argc, argv, &args);

	print_addr(args.listen_addr, args.listen_port);
	printf(" -> ");
	print_addr(args.target_addr, args.target_port);
	printf("\n");

	if(args.mode == SERVER) {
		printf("Running as SERVER\n");
	} else {
		printf("Running as CLIENT\n");
	}

	start_listening(&args);

	return 0;
}