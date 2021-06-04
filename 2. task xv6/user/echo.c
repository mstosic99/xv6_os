#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fcntl.h"

int
main(int argc, char *argv[])
{
	int i;

	int fd = open("long.txt", O_RDWR);
	write(fd, "Hello world\n", 12);
	printf("Hello world\n");
	close(fd);

	exit();
}
