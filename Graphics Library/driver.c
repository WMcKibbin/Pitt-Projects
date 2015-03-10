////////////////////////////////////////
//Author: William McKIbbin
//File: driver.c
//Date: 1/28/14
//This is a simple driver for library.c
//////////////////////////////////////////
#define STDOUT 1
#define STDIN 0

#define BLACK 0x0000	
#define WHITE 0xFFFF	
#define RED 0xF800			
#define GREEN 0x7E0	
#define BLUE 0x1F	
#include "library.h"
#include <sys/syscall.h>
/////////////////////////////////////////////////////
void freeDraw(color_t color)
{
	char key;
	char toggle;
	int x = (640-20)/2;
	int y = (480-20)/2;
	do
	{
		//draw a black rectangle to erase the old one
		key = getkey();
		if(key == 't' && toggle == 1) 
		{	
			toggle = 0;
		}
		else if(key == 't')
		{
			toggle = 1;
		}
		if(toggle ==1)
		{
			draw_pixel(x, y, color);
		}
		if(key == 'w') y -= 1;
		else if(key == 's') y += 1;
		else if(key == 'a') x -= 1;
		else if(key == 'd') x += 1;
		sleep_ms(20);
	} while(key != 'q');
}
////////////////////////////////////////////////
char getMenuChoice()
{
	char choice = 0;
	write(STDOUT, "Choose 1, 2, 3, or 4\n\n", 22);
	write(STDOUT,"1. Free Draw\n", 13);
	write(STDOUT,"2. Draw Rectangle\n", 18);
	write(STDOUT,"3. Draw Circle\n", 15);
	write(STDOUT,"4. Exit\n", 8);
	while(choice == 0)
	{	
		choice = getkey();
	}
	return(choice);
}
int main()
{
	int x = (640-20)/2;
	int y = (480-20)/2;
	int radius = 50;
	int height = 40;
	int width = 40;
	char colChoice;
	char menuChoice;
	color_t color;
while(1)//start control loop
{
	//start graphics
	init_graphics();
	//get their menu selection
	menuChoice = getMenuChoice();
	if(menuChoice == '4')
	{
		clear_screen();
		exit_graphics();
		return 0;
	}
	//get a color choice from a given list
	write(STDOUT, "Enter color:\n", 13);
	write(STDOUT,"1.BLACK 0x0000\n2.WHITE 0xFFFF\n3.RED 0xF800\n4.GREEN 0x7E0\n5.BLUE 0x1F\n", 69);
	colChoice = 0;
	
	while(colChoice == 0)
		colChoice = getkey();
	
	switch(colChoice)
	{
		case '1':
			color = BLACK;
			break;
		case '2':
			color = WHITE;
			break;
		case '3':
			color = RED;
			break;
		case '4':
			color = GREEN;
			break;
		case '5':
			color = BLUE;
			break;
		default:
			color = BLUE;
	}
	//start graphics
	init_graphics();
	//execute desired function
	switch(menuChoice)
	{
		case '1':
			freeDraw(color);		
			break;
		case '2':
			draw_rect(x, y, width, height, color);
			sleep_ms(100);
			sleep_ms(100);
			break;
		case '3':
			draw_circle(x, y, radius, color);
			sleep_ms(100);
			sleep_ms(100);
			break;
		case '4':
			clear_screen();
			exit_graphics();
			return(0);
	}
	clear_screen();
	//exit graphics and restart menu
	exit_graphics();
}//end control loop
}