#include <stdio.h>
#include <string.h>

#define MAXNAME 30
#define MAXLINE 100

FILE *first, *second;

/*

run ./compare to compare output of two files to see if they are the same or if not, which lines differ
I used it to debug by comparing the decoder output of rle to the decoder output of a new compression scheme

*/

int main()
{
    char f[MAXNAME], s[MAXNAME], str1[MAXLINE], str2[MAXLINE];
    
    printf("type the names of the compared files\n");
    printf("first: ");
    gets(f);
    printf("second: ");
    gets(s);
    if((first = fopen(f, "r")) == NULL)
    {
        perror(f);
        return 1;
    }
    else if((second = fopen(s, "r")) == NULL)
    {
        perror(s);
        return 1;
    }
    else
        printf("files open\n\n"); 
	int counter = 0;
    while(!feof(first) && !feof(second))
    {
        fgets(str1, MAXLINE-1, first);
        fgets(str2, MAXLINE-1, second);
       counter  = counter + 1;
	 if(strcmp(str1,str2) != 0)
             {
                printf("first different strings:\n\n");
                printf("%s\n%s\n", str1, str2);
		printf("%d\n", counter);
                break;
            }
    }
    fclose(first);
    fclose(second);
    return 0;
}

