#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BREAK_IF(_cond) if (_cond) { break; } else {}

int stirling2(int n, int k, int c, char *p)
{
    int i;
    if (k == 0)
    {
        puts(p);
    }
    else if (k == 1)
    {
        for (i = 0; i < n; i++)
        {
            *--p = c;
        }
        puts(p);
    }
    else if (n < k)
    {
        return 1;
    }
    else
    {
        *--p = c + k - 1;
        stirling2(n - 1, k - 1, c, p);
        if (n > k)
        {
            for (i = 0; i < k; i++)
            {
                *p = c + i;
                stirling2(n - 1, k, c, p);
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int n, k, c;
    char *s;
    int ret;
    do
    {
        BREAK_IF(argc < 3)
        n = atoi(argv[1]);
        k = atoi(argv[2]);
        BREAK_IF(n <= 0 || k <= 0 || k > n)
        if (argc <= 3 || (c = atoi(argv[3])) == 0)
        {
            c = 1;
        }
        s = (char *) malloc(n + 1);
        BREAK_IF(!s);
        memset(s, 0, n + 1);
        ret = stirling2(n, k, 96 + c, s + n);
        free(s);
        return ret;
    } while (0);
    printf("Usage: %s n k [c=1]\nPrint S(n, k) combinations of strings of "
           "length n of k alphabets (chr(96+c) ..) with insignificant order\n",
           argv[0]);
    return 0;
}
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
