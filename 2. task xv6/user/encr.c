#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fcntl.h"

void
print_help() {
	printf("\nCommand line options: ");
	printf("\n        -h, --help: Show help prompt.");
	printf("\n        -a, --encrypt-all: Encrypt all files in CWD with current key.\n");
}


int
main(int argc, char *argv[])
{
	int returned = 0;
	if(argc == 1) {
		print_help();
	}
	else if(argc == 2) {

		if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			print_help();
		} else {
			int fd = open(argv[1], O_RDWR);
			returned = encr(fd);
		}

	} else {
		printf("\nWrong parameters.\n");
		print_help();
	}

	if(returned == -1) {
		printf("\nFailed to encrypt file: %s (key not set)\n", argv[1]);
	} else if(returned == -2) {
		printf("\nFailed to encrypt file: %s is a device type\n", argv[1]);
	} else if(returned == -3) {
		printf("\nFailed to encrypt file: %s is already encrypted", argv[1]);
	} else if(returned == 0) {
		printf("\nSuccessfully encrypted file: %s\n", argv[1]);
	}
	exit();
}
