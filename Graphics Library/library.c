////////////////////////////////////////
//Author: William McKIbbin
//File: library.c
//Date: 1/28/14
//This is a simple graphics library.
//////////////////////////////////////////

#define FRAME_BUFFER "/dev/fb0"
#define STDIN 0
#define STDOUT 1

#include <sys/syscall.h>
#include <termios.h>
#include "library.h"
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>

//Global Variables
//terminal
struct termios terminal;

//Frame Buffer
int fb;
void* mapPtr;
int memSize;
struct{
	int x;
	int y;
}resol;
struct fb_var_screeninfo varInfo;
struct fb_fix_screeninfo fixInfo;

//time vars for sleep
struct timespec tspec;

//select() vars for getChar()
struct timeval tv;
fd_set readSet;

////////////////////////////////////////////////////////////////////////////////
void init_graphics()
{	
	fb = open(FRAME_BUFFER, O_RDWR);
	if( fb < 0 )
	{
		perror("Error. Frame buffer device file was not opened successfully.\n\n");
		return;
	}
	if(ioctl(fb, FBIOGET_VSCREENINFO, &varInfo))
	{
		perror("Error. Frame buffer var info could not be read.\n\n");
		return;
	}
	if(ioctl(fb, FBIOGET_FSCREENINFO, &fixInfo))
	{
		perror("Error. Frame buffer fix info could not be read.\n\n");
		return;
	}
	//Calculate resolution
	resol.y = varInfo.yres_virtual;
	resol.x = fixInfo.line_length / 2;
	//calculate memory map size *NOTE* sizeof(color_t) is included on line_length
	memSize = varInfo.yres_virtual * fixInfo.line_length;
	mapPtr = (void*)mmap(NULL, memSize, PROT_READ | PROT_WRITE, MAP_SHARED, fb,0);
	if(!mapPtr)
	{
		perror("Error. Frame buffer could not be mapped into user space.\n\n");
	}
	if (ioctl(1,TCGETS, &terminal))
	{
		perror("Error could not ioctl request terminal TCGETS\n\n");
		return;
	}
	//Turn off canonical mode and echo
	terminal.c_lflag &= ~ICANON;
	terminal.c_lflag &= ~ECHO;
	//Set out flags to the terminal
	if (ioctl(1, TCSETS, &terminal))
	{
		perror("Error could not ioctl request terminal TCSETS\n\n");
		return;
	}
	clear_screen();
	//Initialize select syscall vars
	FD_ZERO(&readSet);
	FD_SET(STDIN, &readSet);
	tv.tv_sec = 0;
	tv.tv_usec = 100;
}
/////////////////////////////////////////////////////////////////////
void exit_graphics()
//cleans up file descriptions and memory, clears the screen, renables ECHO and Cannonical
{
	clear_screen();
	if (ioctl(1,TCGETS, &terminal))
	{
		perror("Error could not ioctl request terminal TCGETS\n\n");
		return;
	}
	terminal.c_lflag |= ICANON;
	terminal.c_lflag |= ECHO;
	if (ioctl(1, TCSETS, &terminal))
	{
		perror("Error could not ioctl request terminal TCSETS\n\n");
		return;
	}
	if(munmap(mapPtr, memSize))
	{
		perror("Error. Memory could not unmapped.\n\n");
	}
	close(fb);
}
///////////////////////////////////////////////////////////////////
//converts an x, y index to a memory address using row major conversion
void* rowMajor(int x , int y)
{
	 return (void*)((color_t*)mapPtr + y * resol.x  + x);
}
//draw_pixel writes a color at the x, y coordinates specified
void draw_pixel(int x, int y, color_t color)
{
	if(x <= resol.x && y <= resol.y)
	{
		*((color_t*)rowMajor(x, y)) = color;
	}
}
////////////////////////////////////////////////////////////////////////
//Writes an ANSI escape sequence to STDOUT to Clear the screen
void clear_screen()
{
	char * buf = "\033[2J";
	write(1, (void*)buf , 7);
}
///////////////////////////////////////////////////////////////////////
//Uses non-blocking IO to get charaters from the user
char getkey()
{
	int ready;
	char retChar;
	ready = select(STDIN+1, &readSet, NULL, NULL, NULL);
	switch(ready)
	{
		case -1:
			perror("Error. Select STDIN error\n");
		case 0:
			return 0;
			break;
		default:
			read(STDIN, (void*)&retChar, sizeof(char));
	}
	return(retChar);
}
///////////////////////////////////////////////////////////////////////
//sleeps ms milleseconds
void sleep_ms(long ms)
{	
	tspec.tv_sec = 0;
	tspec.tv_nsec = 1000000 * ms;

	nanosleep(&tspec, NULL);
}
///////////////////////////////////////////////////////////////////////
//draws a rectangle of given width, height, and color with the bottom left corner at(x,y) 
void draw_rect(int x1, int y1, int width, int height, color_t c)
{
	int i;
	//draw top and bottom
	for(i = x1; i <= x1 + width; i++)
	{
		draw_pixel(i, y1, c);
		sleep_ms(10);
		draw_pixel(i, y1 + height, c);
		sleep_ms(10);
	}
	//draw left and right sides
	for(i = y1; i <= y1 + height; i++)
	{
		draw_pixel(x1, i, c);
		sleep_ms(10);
		draw_pixel(x1+ width, i, c);
		sleep_ms(10);
	}
}
///////////////////////////////////////////////////////////////////////
//Code adapted from: 
//http://en.wikipedia.org/wiki/Midpoint_circle_algorithm#Example
void draw_circle(int x, int y, int r, color_t color)
{

  int xcur = r;
  int ycur = 0;
  int radiusError = 1-x;
 
  while(xcur >= ycur)
  {
    draw_pixel(xcur + x, ycur + y, color);
    draw_pixel(ycur + x, xcur + y, color);
    draw_pixel(-xcur + x, ycur + y, color);
    draw_pixel(-ycur + x, xcur + y, color);
    draw_pixel(-xcur + x, -ycur + y, color);
    draw_pixel(-ycur + x, -xcur + y, color);
    draw_pixel(xcur + x, -ycur + y, color);
    draw_pixel(ycur + x, -xcur + y, color);
    sleep_ms(100);
    ycur++;
    if (radiusError<0)
    {
      radiusError += 2 * ycur + 1;
    }
    else
    {
      xcur--;
      radiusError += 2 * (ycur - xcur + 1);
    }
  }
}
