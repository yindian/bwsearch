/*
 * See Copyright Notice in bwslib.h
 */

#include "dzlib.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")
        || (argc == 2 && !strcmp(argv[1], "-d")))
    {
        printf("Usage: %s [-d] file[s]\n",
               argv[0]);
        printf("Note: original files are kept\n");
        return 0;
    }
    if (strcmp(argv[1], "-d"))
    {
        /* compress */
        int i;
        for (i = 1; i < argc; i++)
        {
            if (dzip_compress(argv[i], 0) < 0)
            {
                fprintf(stderr, "Failed to compress %s\n", argv[i]);
            }
        }
    }
    else
    {
        /* decompress */
        int i;
        for (i = 2; i < argc; i++)
        {
            if (dzip_decompress(argv[i], 0) < 0)
            {
                fprintf(stderr, "Failed to decompress %s\n", argv[i]);
            }
        }
    }
    return 0;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
