#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fcntl.h"

int blocks = 150;
char* filename = "long.txt";

void
write_blocks() {
	int fd = open(filename, O_RDWR | O_CREATE);
	for(int i = 0; i < blocks; i++) {
		printf("Writing block %d\n", i+1);
		char c = 'a';
		char str[512];
		int cnt = 0;
		while(cnt < 512) {
			int off = cnt%26;
			str[cnt++] = c + off;
		}
		write(fd, str, strlen(str));
	}
}

void
print_help() {
	printf("\nUse this program to create a big file filled with a-z characters.\n");

	printf("Default filename: %s\n", filename);

	printf("Default blocks: %d\n", blocks);

	printf("Usage: blockwriter [OPTION]...\n");

	printf("\n        -h, --help: Show help prompt.");
	printf("\n        -b, --blocks: Number of blocks to write.");
	printf("\n        -o, --output-file: Set output filename.\n");
}

int
main(int argc, char *argv[])
{
	if(argc == 1) {
		blocks = 150;
		strcpy(filename, "long.txt");

		write_blocks();
		
	}

	else if(argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
		print_help();
	}
	
	else if(argc == 3 && (!strcmp(argv[1], "-b") || !strcmp(argv[1], "--blocks"))) {
		blocks = atoi(argv[2]);
		strcpy(filename, "long.txt");

		write_blocks();
	}

	else if(argc == 3 && (!strcmp(argv[1], "-o") || !strcmp(argv[1], "--output-file"))) {
		blocks = 150;
		strcpy(filename, argv[2]);

		write_blocks();
	}

	else if(argc == 5) {
		if(!strcmp(argv[1], "-b") || !strcmp(argv[1], "--blocks")) {
			blocks = atoi(argv[2]);
			strcpy(filename, argv[4]);
		} else if(!strcmp(argv[1], "-o") || !strcmp(argv[1], "--output-file")) {
			blocks = atoi(argv[4]);
			strcpy(filename, argv[2]);
		}
		write_blocks();
	}
	
	else {
		printf("Wrong parameters.\n");
		print_help();
	}
	exit();
}
