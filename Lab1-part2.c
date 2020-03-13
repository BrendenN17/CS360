#include <stdio.h>
#include <string.h>



typedef unsigned int u32;

char *ctable = "0123456789ABCDEF";
int  BASE = 10; 

int main(int argc, char *argv[], char *env[])
{
u32 x;
x = 25;
char *s = "test";
int y = -5;
myprintf("cha=%c string=%s      dec=%d hex=%x oct=%o neg=%d\n",  'A', "this is a test", 100,    100,   100,  -100);

/*Helper functions testing*/
//printu(x);//working
//prints(s);//working
//printd(y);//working
//printx(x);//working
//printo(x);//working
}





int rpu(u32 x)
{  
    char c;
    if (x){
       c = ctable[x % BASE];
       rpu(x / BASE);
       putchar(c);
    }
}

int printu(u32 x)
{
   (x==0)? putchar('0') : rpu(x);
   putchar(' ');
}

int prints(char *s)
{
    for(int i = 0; i < strlen(s); ++i)//while more characters to be read
    {
        putchar(s[i]);//printing character
       // s++;//advancing to next character
    }
}

int printd(int x)//prints an integer, but could be negative
{
    if (x < 0)//x is negative
    {
        printf("-");//add negative sign in front
        x = -x;
    }
    rpu(x);
    
}

int printx(u32 x)//prints x in HEX  starting with 0x...
{
    (x==0)? putchar('0'):  printf("0x"); rpx(x);
    //if x is 0 print 0, else print 0x (# in hex) and run helper function
}

int rpx(u32 x)
{
    char c;
    if (x)
    {
        c = ctable[x % 16];
        rpx(x / 16);
        putchar(c);
    }
}

int printo(u32 x)//prints x in octal starting with 0..
{
    (x==0)? putchar('0'): printf("0"); rpo(x);
    //if x is 0 print 0, else print 0 then # in octal and run helper
}

int rpo(u32 x)
{
    char c;
    if (x)
    {
        c = ctable[x % 8];
        rpo(x / 8);
        putchar(c);
    }
}


//BREAKS PROGRAM UPON ENTRY
void myprintf(char *fmt, ...)//not working
{
char *cp = fmt;//format string

int *t = (int *)fmt;
int *ip = t + 1; //first item to be read

while (*cp) //while reading the string
{
    if (*cp != '%')
    {
        if (putchar(*cp) == '\n')
        {
            putchar('\r');
        }
    }
    else //if (cp == '%')//reading a %
    {
        //will print everything after % for the first item
        ++cp;//advance to char right after %
        switch (*cp)
        {
        case 'c':
        putchar(*ip);
        ++ip;
        break;

        case 's':
        prints(*ip);
        ++ip;
        break;

        case 'u':
        printu(*ip);
        ++ip;
        break;

        case 'd':
        printd(*ip);
        ++ip;
        break;

        case 'o':
        printo(*ip);
        ++ip;
        break;

        case 'x':
        printx(*ip);
        ++ip;
        break;
        
        default:
            break;
        }  
    }
    ++cp;
}
}