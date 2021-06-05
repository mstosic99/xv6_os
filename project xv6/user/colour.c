#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fcntl.h"

void
print_help() {
	printf("\nCommand line options: ");
	printf("\n        -h, --help: Show help prompt.");
	printf("\n        -bg, --background: Change background colour using words below.\n");
	printf("        -fg, --foreground: Change foreground colour using words below.\n\n");
	printf("      black, blue, green, aqua, red, purple, yellow, white,\n      Lblack, Lblue, Lgreen, Laqua, Lred, Lpurple, Lyellow, Lwhite\n");
	printf("      colour reset to set xv6 default colours.\n");
}

char *colours_labels[] = {"black", "blue", "green", "aqua", "red", "purple", "yellow", "white", "Lblack", "Lblue", "Lgreen", "Laqua", "Lred", "Lpurple", "Lyellow", "Lwhite"};




int
main(int argc, char *argv[])
{
	
	if(argc == 1){
		print_help();
		exit();
	}
	
	else if(argc == 2){
		if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
			print_help();
		else if(!strcmp(argv[1], "reset")) {
			colour(7, BOTH); 				// 0x07 xv6 default
		} else {
			int n = strlen(argv[1]);
			for(int i = 2; i < n; i++) {
				argv[1][i-2] = argv[1][i];
			}
			argv[1][n] = 0;

			n = strlen(argv[1]);
			int rez = 0;
			for(int i = 0; i < n; i++) {
				if(argv[1][i] >= 'a' && argv[1][i] <= 'f') {
					rez = rez * 16 + argv[1][i] - 'a' + 10;
				} else if(argv[1][i] >= '0' && argv[1][i] <= '9') {
					rez = rez * 16 + argv[1][i] - '0';
				}
			}
			colour(rez, BOTH);
		}
	} 
	
	else if(argc == 3 && (!strcmp(argv[1], "-fg") || !strcmp(argv[1], "--foreground"))) {

		int num;
		for(int i = 0; i < 16; i++) {
			if(!strcmp(argv[2], colours_labels[i])){
				num = i;
				break;
			}
		}
		colour(num, FG);
	}

	else if(argc == 3 && (!strcmp(argv[1], "-bg") || !strcmp(argv[1], "--background"))) {
		int num;
		for(int i = 0; i < 16; i++) {
			if(!strcmp(argv[2], colours_labels[i])){
				num = i;
				break;
			}
		}
		colour(num, BG);
	}

	else if(argc == 5) {
		int bg;
		int fg;
		int num;
		if(!strcmp(argv[1], "-bg") || !strcmp(argv[1], "--background")) {

			for(int i = 0; i < 16; i++) {
				if(!strcmp(argv[2], colours_labels[i])){
					bg = i;
				}

				if(!strcmp(argv[4], colours_labels[i])){
					fg = i;
				}
			}
		}
		else if(!strcmp(argv[1], "-fg") || !strcmp(argv[1], "--foreground")) {
			
			for(int i = 0; i < 16; i++) {
				if(!strcmp(argv[2], colours_labels[i])){
					fg = i;
				}

				if(!strcmp(argv[4], colours_labels[i])){
					bg = i;
				}
			}
		}

		num = (bg << 4) | fg;

		colour(num, BOTH);
	}
	
	else {
		printf("Wrong parameters.\n");
		print_help();
	}	
	// colour(atoi(argv[1]));

	exit();
}
