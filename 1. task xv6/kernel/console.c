// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void consputc(int);

static int panicked = 0;

static struct {
	struct spinlock lock;
	int locking;
} cons;

static void
printint(int xx, int base, int sign)
{
	static char digits[] = "0123456789abcdef";
	char buf[16];
	int i;
	uint x;

	if(sign && (sign = xx < 0))
		x = -xx;
	else
		x = xx;

	i = 0;
	do{
		buf[i++] = digits[x % base];
	}while((x /= base) != 0);

	if(sign)
		buf[i++] = '-';

	while(--i >= 0)
		consputc(buf[i]);
}

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
	int i, c, locking;
	uint *argp;
	char *s;

	locking = cons.locking;
	if(locking)
		acquire(&cons.lock);

	if (fmt == 0)
		panic("null fmt");

	argp = (uint*)(void*)(&fmt + 1);
	for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
		if(c != '%'){
			consputc(c);
			continue;
		}
		c = fmt[++i] & 0xff;
		if(c == 0)
			break;
		switch(c){
		case 'd':
			printint(*argp++, 10, 1);
			break;
		case 'x':
		case 'p':
			printint(*argp++, 16, 0);
			break;
		case 's':
			if((s = (char*)*argp++) == 0)
				s = "(null)";
			for(; *s; s++)
				consputc(*s);
			break;
		case '%':
			consputc('%');
			break;
		default:
			// Print unknown % sequence to draw attention.
			consputc('%');
			consputc(c);
			break;
		}
	}

	if(locking)
		release(&cons.lock);
}

void
panic(char *s)
{
	int i;
	uint pcs[10];

	cli();
	cons.locking = 0;
	// use lapiccpunum so that we can call panic from mycpu()
	cprintf("lapicid %d: panic: ", lapicid());
	cprintf(s);
	cprintf("\n");
	getcallerpcs(&s, pcs);
	for(i=0; i<10; i++)
		cprintf(" %p", pcs[i]);
	panicked = 1; // freeze other CPU
	for(;;)
		;
}

#define BACKSPACE 0x100
#define CRTPORT 0x3d4
#define VMSTART 0xb8000
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory


static void
cgaputc(int c)
{
	int pos;

	// Cursor position: col + 80*row.
	outb(CRTPORT, 14);
	pos = inb(CRTPORT+1) << 8;
	outb(CRTPORT, 15);
	pos |= inb(CRTPORT+1);

	if(c == '\n')
		pos += 80 - pos%80;
	else if(c == BACKSPACE){
		if(pos > 0) --pos;
	} else
		crt[pos++] = (c&0xff) | 0x0700;  // black on white

	if(pos < 0 || pos > 25*80)
		panic("pos under/overflow");

	if((pos/80) >= 24){  // Scroll up.
		memmove(crt, crt+80, sizeof(crt[0])*23*80);
		pos -= 80;
		memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
	}

	outb(CRTPORT, 14);
	outb(CRTPORT+1, pos>>8);
	outb(CRTPORT, 15);
	outb(CRTPORT+1, pos);
	crt[pos] = ' ' | 0x0700;
}

void
consputc(int c)
{
	if(panicked){
		cli();
		for(;;)
			;
	}

	if(c == BACKSPACE){
		uartputc('\b'); uartputc(' '); uartputc('\b');
	} else
		uartputc(c);
	cgaputc(c);
}

#define INPUT_BUF 128
struct {
	char buf[INPUT_BUF];
	uint r;  // Read index
	uint w;  // Write index
	uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x

int altValidator;

int colorsActive = 0;


ushort consolebuf[80*24];

uint currentSelection = 0x0000;

int currentColor = 0x0700;

typedef struct Pair{
	int x, y;
}Pair;

Pair pair = {7, 0};

void
paintPalette() {
	
	for(int i = 0; i < 80*24; i++) {
		crt[i] = consolebuf[i];
	}
	// black on white
	for(int position = 0; position < 80*24; position++) {
		if(position == 61) {
			crt[position++] = 0x2f | 0x0700; // '\'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x46 | 0x0700; // 'F'
			crt[position++] = 0x47 | 0x0700; // 'G'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x20 | 0x0700; // ' '
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x42 | 0x0700; // 'B'
			crt[position++] = 0x47 | 0x0700; // 'G'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position] = 0x5c | 0x0700;   // '/'
		} else if(position == 61 + 80) {												// BLACK

			int isSelected = 0;
			if(pair.y == 0 && (currentSelection == 0x0000 || currentSelection == 0x0800)) {
				isSelected = 1;
			}

			crt[position++] = 0x7c | 0x0700; 	

			!isSelected ? (crt[position++] = 0x42 | 0x0700) : (crt[position++] = 0x42 | 0x7000);
			!isSelected ? (crt[position++] = 0x6c | 0x0700) : (crt[position++] = 0x6c | 0x7000);
			!isSelected ? (crt[position++] = 0x61 | 0x0700) : (crt[position++] = 0x61 | 0x7000);
			!isSelected ? (crt[position++] = 0x63 | 0x0700) : (crt[position++] = 0x63 | 0x7000);
			!isSelected ? (crt[position++] = 0x6b | 0x0700) : (crt[position++] = 0x6b | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);

			crt[position++] = 0x7c | 0x0700;
			
			if(pair.y == 1 && (currentSelection == 0x0000 || currentSelection == 0x8000)){
				if(!isSelected) isSelected = 1;
				else isSelected = 0;
			} else {
				isSelected = 0;
			}
			 													
			!isSelected ? (crt[position++] = 0x42 | 0x0700) : (crt[position++] = 0x42 | 0x7000); 
			!isSelected ? (crt[position++] = 0x6c | 0x0700) : (crt[position++] = 0x6c | 0x7000); 
			!isSelected ? (crt[position++] = 0x61 | 0x0700) : (crt[position++] = 0x61 | 0x7000); 
			!isSelected ? (crt[position++] = 0x63 | 0x0700) : (crt[position++] = 0x63 | 0x7000); 
			!isSelected ? (crt[position++] = 0x6b | 0x0700) : (crt[position++] = 0x6b | 0x7000); 
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000); 
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000); 
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000); 

			crt[position] = 0x7c | 0x0700; 
		} else if(position == 61 + 80*2) {												// BLUE

			int isSelected = 0;
			if(pair.y == 0 && (currentSelection == 0x0100 || currentSelection == 0x0900)) {
				isSelected = 1;
			}

			crt[position++] = 0x7c | 0x0700;

			!isSelected ? (crt[position++] = 0x42 | 0x0700) : (crt[position++] = 0x42 | 0x7000);			
			!isSelected ? (crt[position++] = 0x6c | 0x0700) : (crt[position++] = 0x6c | 0x7000);			
			!isSelected ? (crt[position++] = 0x75 | 0x0700) : (crt[position++] = 0x75 | 0x7000);			
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000);			
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);			
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);			
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);			
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);			
		
			crt[position++] = 0x7c | 0x0700; 

			if(pair.y == 1 && (currentSelection == 0x1000 || currentSelection == 0x9000)){
				if(!isSelected) isSelected = 1;
				else isSelected = 0;
			} else {
				isSelected = 0;
			}

			!isSelected ? (crt[position++] = 0x42 | 0x0700) : (crt[position++] = 0x42 | 0x7000);			
			!isSelected ? (crt[position++] = 0x6c | 0x0700) : (crt[position++] = 0x6c | 0x7000);			
			!isSelected ? (crt[position++] = 0x75 | 0x0700) : (crt[position++] = 0x75 | 0x7000);			
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000);			
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);			
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);			
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);			
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);

			crt[position] = 0x7c | 0x0700; 

		} else if(position == 61 + 80*3) {												// GREEN

			int isSelected = 0;
			if(pair.y == 0 && (currentSelection == 0x0200 || currentSelection == 0x0a00)) {
				isSelected = 1;
			}

			crt[position++] = 0x7c | 0x0700;

			!isSelected ? (crt[position++] = 0x47 | 0x0700) : (crt[position++] = 0x47 | 0x7000);
			!isSelected ? (crt[position++] = 0x72 | 0x0700) : (crt[position++] = 0x72 | 0x7000);
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000);
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000);
			!isSelected ? (crt[position++] = 0x6e | 0x0700) : (crt[position++] = 0x6e | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);

			crt[position++] = 0x7c | 0x0700;

			if(pair.y == 1 && (currentSelection == 0x2000 || currentSelection == 0xa000)){
				if(!isSelected) isSelected = 1;
				else isSelected = 0;
			} else {
				isSelected = 0;
			}

			!isSelected ? (crt[position++] = 0x47 | 0x0700) : (crt[position++] = 0x47 | 0x7000);
			!isSelected ? (crt[position++] = 0x72 | 0x0700) : (crt[position++] = 0x72 | 0x7000);
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000);
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000);
			!isSelected ? (crt[position++] = 0x6e | 0x0700) : (crt[position++] = 0x6e | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			crt[position] = 0x7c | 0x0700; 
		} else if(position == 61 + 80*4) {												// AQUA

			int isSelected = 0;
			if(pair.y == 0 && (currentSelection == 0x0300 || currentSelection == 0x0b00)) {
				isSelected = 1;
			}

			crt[position++] = 0x7c | 0x0700; 
			
			!isSelected ? (crt[position++] = 0x41 | 0x0700) : (crt[position++] = 0x41 | 0x7000);
			!isSelected ? (crt[position++] = 0x71 | 0x0700) : (crt[position++] = 0x71 | 0x7000);
			!isSelected ? (crt[position++] = 0x75 | 0x0700) : (crt[position++] = 0x75 | 0x7000);
			!isSelected ? (crt[position++] = 0x61 | 0x0700) : (crt[position++] = 0x61 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			
			crt[position++] = 0x7c | 0x0700; 

			if(pair.y == 1 && (currentSelection == 0x3000 || currentSelection == 0xb000)){
				if(!isSelected) isSelected = 1;
				else isSelected = 0;
			} else {
				isSelected = 0;
			}

			!isSelected ? (crt[position++] = 0x41 | 0x0700) : (crt[position++] = 0x41 | 0x7000);
			!isSelected ? (crt[position++] = 0x71 | 0x0700) : (crt[position++] = 0x71 | 0x7000);
			!isSelected ? (crt[position++] = 0x75 | 0x0700) : (crt[position++] = 0x75 | 0x7000);
			!isSelected ? (crt[position++] = 0x61 | 0x0700) : (crt[position++] = 0x61 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);

			crt[position] = 0x7c | 0x0700; 

		} else if(position == 61 + 80*5) {												// RED

			int isSelected = 0;
			if(pair.y == 0 && (currentSelection == 0x0400 || currentSelection == 0x0c00)) {
				isSelected = 1;
			}

			crt[position++] = 0x7c | 0x0700;

			!isSelected ? (crt[position++] = 0x52 | 0x0700) : (crt[position++] = 0x52 | 0x7000);
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000);
			!isSelected ? (crt[position++] = 0x64 | 0x0700) : (crt[position++] = 0x64 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			
			crt[position++] = 0x7c | 0x0700;

			if(pair.y == 1 && (currentSelection == 0x4000 || currentSelection == 0xc000)){
				if(!isSelected)  isSelected = 1;
				else isSelected = 0;
			} else {
				isSelected = 0;
			}

			!isSelected ? (crt[position++] = 0x52 | 0x0700) : (crt[position++] = 0x52 | 0x7000);
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000);
			!isSelected ? (crt[position++] = 0x64 | 0x0700) : (crt[position++] = 0x64 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);

			crt[position] = 0x7c | 0x0700;

		} else if(position == 61 + 80*6) {												// PURPLE

			int isSelected = 0;
			if(pair.y == 0 && (currentSelection == 0x0500 || currentSelection == 0x0d00)) {
				isSelected = 1;
			}

			crt[position++] = 0x7c | 0x0700;

			!isSelected ? (crt[position++] = 0x50 | 0x0700) : (crt[position++] = 0x50 | 0x7000);
			!isSelected ? (crt[position++] = 0x75 | 0x0700) : (crt[position++] = 0x75 | 0x7000);
			!isSelected ? (crt[position++] = 0x72 | 0x0700) : (crt[position++] = 0x72 | 0x7000);
			!isSelected ? (crt[position++] = 0x70 | 0x0700) : (crt[position++] = 0x70 | 0x7000);
			!isSelected ? (crt[position++] = 0x6c | 0x0700) : (crt[position++] = 0x6c | 0x7000);
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);

			crt[position++] = 0x7c | 0x0700; 

			if(pair.y == 1 && (currentSelection== 0x5000 || currentSelection == 0xd000)){
				if(!isSelected)  isSelected = 1;
				else isSelected = 0;
			} else {
				isSelected = 0;
			}

			!isSelected ? (crt[position++] = 0x50 | 0x0700) : (crt[position++] = 0x50 | 0x7000);
			!isSelected ? (crt[position++] = 0x75 | 0x0700) : (crt[position++] = 0x75 | 0x7000);
			!isSelected ? (crt[position++] = 0x72 | 0x0700) : (crt[position++] = 0x72 | 0x7000);
			!isSelected ? (crt[position++] = 0x70 | 0x0700) : (crt[position++] = 0x70 | 0x7000);
			!isSelected ? (crt[position++] = 0x6c | 0x0700) : (crt[position++] = 0x6c | 0x7000);
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);

			crt[position] = 0x7c | 0x0700;

		} else if(position == 61 + 80*7) {												// YELLOW

			int isSelected = 0;
			if(pair.y == 0 && (currentSelection== 0x0600 || currentSelection == 0x0e00)) {
				isSelected = 1;
			}

			crt[position++] = 0x7c | 0x0700;

			!isSelected ? (crt[position++] = 0x59 | 0x0700) : (crt[position++] = 0x59 | 0x7000); 
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000); 
			!isSelected ? (crt[position++] = 0x6c | 0x0700) : (crt[position++] = 0x6c | 0x7000); 
			!isSelected ? (crt[position++] = 0x6c | 0x0700) : (crt[position++] = 0x6c | 0x7000); 
			!isSelected ? (crt[position++] = 0x6f | 0x0700) : (crt[position++] = 0x6f | 0x7000); 
			!isSelected ? (crt[position++] = 0x77 | 0x0700) : (crt[position++] = 0x77 | 0x7000); 
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000); 
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000); 

			crt[position++] = 0x7c | 0x0700;

			if(pair.y == 1 && (currentSelection== 0x6000 || currentSelection == 0xe000)){
				if(!isSelected)  isSelected = 1;
				else isSelected = 0;
			} else {
				isSelected = 0;
			}

			!isSelected ? (crt[position++] = 0x59 | 0x0700) : (crt[position++] = 0x59 | 0x7000); 
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000); 
			!isSelected ? (crt[position++] = 0x6c | 0x0700) : (crt[position++] = 0x6c | 0x7000); 
			!isSelected ? (crt[position++] = 0x6c | 0x0700) : (crt[position++] = 0x6c | 0x7000); 
			!isSelected ? (crt[position++] = 0x6f | 0x0700) : (crt[position++] = 0x6f | 0x7000); 
			!isSelected ? (crt[position++] = 0x77 | 0x0700) : (crt[position++] = 0x77 | 0x7000); 
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000); 
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);

			crt[position] = 0x7c | 0x0700;
		} else if(position == 61 + 80*8) {												// WHITE

			int isSelected = 0;
			if(pair.y == 0 && (currentSelection == 0x0700 || currentSelection == 0x0f00)) {
				isSelected = 1;
			}

			crt[position++] = 0x7c | 0x0700;

			!isSelected ? (crt[position++] = 0x57 | 0x0700) : (crt[position++] = 0x57 | 0x7000);
			!isSelected ? (crt[position++] = 0x68 | 0x0700) : (crt[position++] = 0x68 | 0x7000);
			!isSelected ? (crt[position++] = 0x69 | 0x0700) : (crt[position++] = 0x69 | 0x7000);
			!isSelected ? (crt[position++] = 0x74 | 0x0700) : (crt[position++] = 0x74 | 0x7000);
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			
			crt[position++] = 0x7c | 0x0700;

			if(pair.y == 1 && (currentSelection == 0x7000 || currentSelection == 0xf000)){
				if(!isSelected)  isSelected = 1;
				else isSelected = 0;
			} else {
				isSelected = 0;
			}

			!isSelected ? (crt[position++] = 0x57 | 0x0700) : (crt[position++] = 0x57 | 0x7000);
			!isSelected ? (crt[position++] = 0x68 | 0x0700) : (crt[position++] = 0x68 | 0x7000);
			!isSelected ? (crt[position++] = 0x69 | 0x0700) : (crt[position++] = 0x69 | 0x7000);
			!isSelected ? (crt[position++] = 0x74 | 0x0700) : (crt[position++] = 0x74 | 0x7000);
			!isSelected ? (crt[position++] = 0x65 | 0x0700) : (crt[position++] = 0x65 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);
			!isSelected ? (crt[position++] = 0x20 | 0x0700) : (crt[position++] = 0x20 | 0x7000);

			crt[position] = 0x7c | 0x0700;

		} else if(position == 61 + 80*9) {
			crt[position++] = 0x5c | 0x0700; // '\'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position++] = 0x2d | 0x0700; // '-'
			crt[position] = 0x2f | 0x0700;   // '/'
		}
	}

}

void
showColors(){
	if(altValidator != 3)
		return;
	if(!colorsActive) {
		
		for(int i = 0; i < 80*24; i++) {
			consolebuf[i] = crt[i];
		}
		paintPalette();
		colorsActive = 1;

	} else {

		for(int i = 0; i < 80*24; i++) {
			crt[i] = consolebuf[i];
		}
		colorsActive = 0;

	}
	
}



void
consoleintr(int (*getc)(void))
{

	int c, doprocdump = 0;

	acquire(&cons.lock);
	while((c = getc()) >= 0){
		switch(c){
		case C('P'):  // Process listing.
			// procdump() locks cons.lock indirectly; invoke later
			if(colorsActive) break;
			doprocdump = 1;
			break;
		case C('U'):  // Kill line.
			if(colorsActive) break;
			while(input.e != input.w &&
			      input.buf[(input.e-1) % INPUT_BUF] != '\n'){
				input.e--;
				consputc(BACKSPACE);
			}
			break;
		case C('H'): case '\x7f':  // Backspace
			if(colorsActive) break;
			if(input.e != input.w){
				input.e--;
				consputc(BACKSPACE);
			}
			break;
		case C('_'): // Alt + C
			altValidator = 1;
			showColors();
			break;
		case C(']'): // Alt + O
			if(altValidator == 1)
				altValidator = 2;
			else 
				altValidator = 0;
			showColors();
			break;
		case C('^'): // Alt + L
			if(altValidator == 2)
				altValidator = 3;
			else
				altValidator = 0;
			showColors();
			break;
		default:
			if(colorsActive) {
				if (c == 'w') {
					if(pair.x == 0) {
						if(pair.y == 0) {
							currentSelection = 0x0700;
						} else if(pair.y == 1) {
							currentSelection = 0x7000;
						}
						pair.x = 7;
					} else {
						if(pair.y == 0) {
							currentSelection = (((currentSelection >> 8) - 1) % 8) << 8;
						} else {
							currentSelection = (((currentSelection >> 12) - 1) % 8) << 12;
						}
						pair.x--;
					}
				} else if (c == 'a' || c == 'd'){
					
					if(pair.y == 0) {
						currentSelection = currentSelection << 4;
						pair.y = 1;
					} else {
						currentSelection = currentSelection >> 4;
						pair.y = 0;
					}				
				} else if (c == 's') {
					if(pair.x == 7) {
						currentSelection = 0x0000;
						pair.x = 0;
					} else {
						if(pair.y == 0) {
							currentSelection = (((currentSelection >> 8) + 1) % 8) << 8;
						} else {
							currentSelection = (((currentSelection >> 12) + 1) % 8) << 12;
						}
						pair.x++;
					}
				} else if (c == 'e') {
					// TODO

				} else if (c == 'r') {
					// TODO
				}
				paintPalette();
				break;
			}
			if(c != 0 && input.e-input.r < INPUT_BUF){
				c = (c == '\r') ? '\n' : c;
				input.buf[input.e++ % INPUT_BUF] = c;
				consputc(c);
				if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
					input.w = input.e;
					wakeup(&input.r);
				}
			}
			break;
		}
	}
	release(&cons.lock);
	if(doprocdump) {
		procdump();  // now call procdump() wo. cons.lock held
	}
}

int
consoleread(struct inode *ip, char *dst, int n)
{
	uint target;
	int c;

	iunlock(ip);
	target = n;
	acquire(&cons.lock);
	while(n > 0){
		while(input.r == input.w){
			if(myproc()->killed){
				release(&cons.lock);
				ilock(ip);
				return -1;
			}
			sleep(&input.r, &cons.lock);
		}
		c = input.buf[input.r++ % INPUT_BUF];
		if(c == C('D')){  // EOF
			if(n < target){
				// Save ^D for next time, to make sure
				// caller gets a 0-byte result.
				input.r--;
			}
			break;
		}
		*dst++ = c;
		--n;
		if(c == '\n')
			break;
	}
	release(&cons.lock);
	ilock(ip);

	return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n)
{
	int i;

	iunlock(ip);
	acquire(&cons.lock);
	for(i = 0; i < n; i++)
		consputc(buf[i] & 0xff);
	release(&cons.lock);
	ilock(ip);

	return n;
}

void
consoleinit(void)
{
	initlock(&cons.lock, "console");

	devsw[CONSOLE].write = consolewrite;
	devsw[CONSOLE].read = consoleread;
	cons.locking = 1;

	ioapicenable(IRQ_KBD, 0);
}

