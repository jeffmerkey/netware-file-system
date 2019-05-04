
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    unsigned char ch;
    FILE *fp, *fl;

    if (argc < 3)
    {
       printf("USAGE:  dos2unix <input> <output>\n");
       exit(0);
    }

    fp = fopen(argv[1], "rb");
    if (!fp)
    {
       printf("error opening input file %s\n", argv[1]);
       exit(0);
    }

    fl = fopen(argv[2], "wb");
    if (!fl)
    {
       printf("error opening ouput file %s\n", argv[2]);
       fclose(fp);
       exit(0);
    }

    while (!feof(fp))
    {
       ch = getc(fp);
       if (ch == '\r')
          continue;
       putc(ch, fl);
    }
    fclose(fp);
    fclose(fl);

    exit(0);
}
