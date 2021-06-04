// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fcntl.h"
#include "kernel/colours.h"

char *argv[] = { "sh", 0 };

short colours[6] = {0xf1, 0x70, 0x26, 0x62, 0x52, 0x07};

int
main(void)
{
	int wpid;

	if(getpid() != 1){
		fprintf(2, "init: already running\n");
		exit();
	}

	if(open("/dev/console", O_RDWR) < 0){
		mknod("/dev/console", 1, 1);
		open("/dev/console", O_RDWR);
	}
	dup(0);  // stdout
	dup(0);  // stderr

	for(int i = 1; i <= 6; i++){
		int pid;
		char terminal_name[] = "/dev/tty*";
		terminal_name[strlen(terminal_name) - 1] = i + '0';

		pid = fork();
		if(pid < 0){
			printf("init: fork failed\n");
			exit();
		}
		if(pid == 0){
			close(0);
			close(1);
			close(2);

			if(open(terminal_name, O_RDWR) < 0) {
				mknod(terminal_name, 1, i);
				open(terminal_name, O_RDWR);
			}
			colour(colours[i-1], BOTH);

			dup(0);
			dup(0);

			printf("starting sh on %s\n", terminal_name);

			exec("/bin/sh", argv);
			printf("init: exec sh failed\n");
			exit();
		}
		while((wpid=wait()) >= 0 && wpid != pid)
			printf("zombie!\n");
	}
}
