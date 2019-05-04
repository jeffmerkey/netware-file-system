
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

unsigned char *dumpAsBytes(unsigned char *p, unsigned long size, unsigned long base)
{
   register unsigned long i, r, total, count;
   register unsigned char *op = p;

   count = size / 16;

   printf("           0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

   for (r=0; r < count; r++)
   {
      printf("%08X ", (p - op) + base);
      for (total = 0, i=0; i < 16; i++, total++)
      {
	 printf(" %02X", (unsigned char) p[i]);
      }
      printf("  ");
      for (i=0; i < total; i++)
      {
	 if (p[i] < 32 || p[i] > 126)
	    printf(".");
	 else printf("%c", p[i]);
      }
      printf("\n");

      p = (void *)((unsigned long) p + (unsigned long) total);
   }
   return p;

}

unsigned char *dumpAsLong(unsigned char *p, unsigned long size, unsigned long base)
{
   register int i, r, count;
   register unsigned char *op = p;
   unsigned long *lp;

   count = size / 16;

   lp = (unsigned long *) p;

   for (r=0; r < count; r++)
   {
      printf("%08X ", (p - op) + base);
      for (i=0; i < (16 / 4); i++)
      {
	 printf(" %08X", (unsigned long) lp[i]);
      }
      printf("  ");
      for (i=0; i < 16; i++)
      {
	 if (p[i] < 32 || p[i] > 126) printf(".");
	 else printf("%c", p[i]);
      }
      printf("\n");

      p = (void *)((unsigned long) p + (unsigned long) 16);
      lp = (unsigned long *) p;
   }
   return p;

}

int getch(void)
{
   return getc(stdin);
}

unsigned long pause(void)
{
   extern int getch(void);
   register unsigned long key;

   printf(" --- More --- ");
   key = getch();
   printf("%c              %c", '\r', '\r');
   if (key == 0x1B) // ESCAPE
      return 1;
   else
      return 0;

}
unsigned char buffer[512];


int main(int argc, char *argv[])
{
    long rc, total = 0, offset = 0;
    FILE *fp;

    if (argc < 2)
    {
       printf("USAGE:  dump <filename.ext> <offset>\n");
       return 1;
    }

    if (argc == 3)
       offset = atol(argv[2]);


    fp = fopen(argv[1], "rb");
    if (!fp)
    {
       printf("error opening file [%s]\n", argv[1]);
       return 1;
    }

    rc = fseek(fp, offset, SEEK_SET);
    if (rc == -1)
    {
       printf("error seeking file offset %d\n", offset);
       return 1;
    }

    total = offset;
    while (!feof(fp))
    {
       rc = fread(buffer, 256, 1, fp);
       dumpAsBytes(buffer, 256, total);
       total += 256;

       if (pause())
	  break;
    }

    fclose(fp);
    return 0;
}
