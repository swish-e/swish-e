#include <stdio.h>


/* This file is not part of swish-e. Just for testing " portable longs " */
/* The second one is faster */

void printlong(fp, num)
FILE *fp;
long num;
{
int tmp;

			tmp=(num % 256);num>>=8;
			tmp=(num % 256);num>>=8;
			tmp=(num % 256);num>>=8;
			tmp=(num % 256);
}

void printlong2(fp, num)
FILE *fp;
long num;
{
int tmp;
              tmp=(num >>24) & 0xFF;
              tmp=(num >>16) & 0xFF;
              tmp=(num >>8) & 0xFF;
              tmp=num & 0xFF;
}

int main()
{
long num=123456789L;
int i;
	for(i=0;i<99999999;i++)
	printlong2(NULL,num);
}
