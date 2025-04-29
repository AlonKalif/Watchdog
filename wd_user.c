/*
	Developer: Alon Kalif
	File: 	   wd.c
	Reviewer:  Shimon
	Date: 	   12.10.2024
	Status:    Approved	
*/
#include <stdio.h>
#include <stdio.h>		/* For printf*/
#include <signal.h>		/* For sig atomic var */
#include <unistd.h>		/* For getpid */

#include "wd.h"			/* For WDStart, WDStop */

#define GREEN "\033[0;32m"
#define RESET "\033[0m"

int main(int argc, char *argv[])
{
	sig_atomic_t count = 25;

	WDStart(1, 5, argc, argv);
	printf("user (parent) pid = %d\n", getpid());
	while(count--)
	{
		printf(GREEN "count = %u\n" RESET, count);
		sleep(1);
		
	}

    WDStop();

	return 0;
}







