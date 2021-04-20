#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fcntl.h"

void
print_help() {
	printf("\nCommand line options: ");
	printf("\n        -h, --help: Show help prompt.");
	printf("\n        -s, --secret: Enter the key via STDIN. Hide key when entering it.\n");
}

int
main(int argc, char *argv[])
{
	char str[20];
	int in;
	if(argc == 1) {
		print_help();
	}
	else if(argc == 2) {

		if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			print_help();
		} else if(!strcmp(argv[1], "-s") || !strcmp(argv[1], "--secret")) {
			
			printf("\nEnter the key:");
			setecho(0);
			read(0, str, sizeof(str));
			setecho(1);
			in = atoi(str);
			setkey(in);


		} else {
			in = atoi(argv[1]);
			setkey(in);
		}

	} else {
		printf("\nWrong parameters.\n");
		print_help();
	}
	exit();
}
