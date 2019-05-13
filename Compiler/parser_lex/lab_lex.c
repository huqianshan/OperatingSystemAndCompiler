#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKENSIZE 256
#define MAXSIZE 2048
#define MAXLINE 128
#define CODE_FILE_NAME "src.txt"
#define DYD_FILE_NAME "out.dyd"
#define ERR_FILE_NAME "out.err"

char word;
char *num2word[24] = {"0", "$BEGIN", "$END", "$INTER", "$IF",
                      "$THEN", "$ELSE", "$FUNCTION", "$READ",
                      "$WRITE", "$SYMBOL", "$CONST", "= $EQ",
                      "!= $NE", "<= $LE", "< $L", ">= $GE",
                      "> $G", "_ $", "* $", ":= $", "(", ")",
                      ";"};
char code[MAXSIZE] = "";
char *project = code;
char token[TOKENSIZE] = "";
char dyd_file[MAXSIZE] = "";
char err_file[MAXSIZE] = "";

void getchar1()
{
    word = *(project++);
}

void getnbc()
{
    while (word == ' ')
    {
        getchar1();
    }
}

int concat()
{
    strcat(token, &word);
    return 1;
}

int letter()
{
    if ((word >= 'a' && word <= 'z') || (word >= 'A' && word <= 'Z'))
    {
        return 1;
    }
    return 0;
}

int digit()
{
    if (word >= '0' && word <= '9')
    {
        return 1;
    }
    return 0;
}
// char ptr reback and set char word null
void retract()
{
    project--;
    word = '\0';
}

// deal with reserved symbol
int reserved()
{
    if (strcmp(token, "begin") == 0)
        return 1;
    else if (strcmp(token, "end") == 0)
        return 2;
    else if (strcmp(token, "integer") == 0)
        return 3;
    else if (strcmp(token, "if") == 0)
        return 4;
    else if (strcmp(token, "then") == 0)
        return 5;
    else if (strcmp(token, "else") == 0)
        return 6;
    else if (strcmp(token, "function") == 0)
        return 7;
    else if (strcmp(token, "read") == 0)
        return 8;
    else if (strcmp(token, "write") == 0)
        return 9;
    else
        return 0;
}

// error helper function

int error(int line, char *info)
{   
    char *tem = malloc(MAXLINE * sizeof(char));
    sprintf(tem, "**LINE:%d %s\n", line, info);
    strcat(err_file, tem);
    fprintf(stderr,"%s",tem);
    free(tem);
    return 1;
}

int Print(char *str, int num)
{
    char *tem = malloc(MAXLINE * sizeof(char));
    sprintf(tem, "%16s %2d\n",str, num);
    strcat(dyd_file, tem);
    printf("%s", tem);
    free(tem);
    return 1;
}

int LexAnalyze()
{

    int num;
    static int line = 1;
    memset(token, '\0', TOKENSIZE);
    getchar1();
    getnbc();

    if (letter())
    {
        // word is alpha and digit
        while (digit() || letter())
        {
            concat();
            getchar1();
        }

        // TOKEN
        retract();
        if ((num = reserved()) != 0)
        {   
            Print(num2word[num],num);
        }
        else
        {
            Print(token,10);
        }
        return 1;
    }

    if(digit()){
       while (digit())
            {
                concat();
                getchar1();
            }
            retract();
            Print("$CONSTANT", 11);
            return 1;
    }

    switch (word)
    {
    case '\n':
        Print("$EOLN", 24);
        line++;
        break;

    case EOF:
        Print("$EOF", 25);
        return 0;

    case '\0':
        return 0;

    case '=':
        Print("$E", 12);
        break;

    case '<':
        getchar1();
        if (word == '>')
        {
            Print("$NE", 13);
        }
        else if (word == '=')
        {
            Print("$LE", 14);
        }
        else
        {
            retract();
            Print("$L", 15);
        }
        break;

    case '>':
        getchar1();
        if (word == '=')
        {
            Print("$GE", 16);
        }
        else
        {
            retract();
            Print("$G", 17);
        }
        break;

    case '-':

        Print("$MINUS", 18);
        break;

    case '*':

        Print("$MUTIPLY", 19);

    case ':':
        getchar1();
        if (word == '=')
        {
            Print("$ASSIGN", 20);
        }
        else
        {
            retract();
        }
        break;

    case '(':
        Print("$LBRACKET", 21);
        break;

    case ')':
        Print("$RBRACKET", 22);
        break;

    case ';':
        Print("$SEMI", 23);
        break;

    default:
        error(line, "ILLEGAL_CHAR_ERR");
        break;
    }
    return 1;
}

int read()
{
    FILE *fp = NULL;

    fp = fopen(CODE_FILE_NAME, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Open src file failedd \n");
        return -1;
    }

    char *buf = malloc(MAXLINE * sizeof(char));
    memset(code, '\0', MAXSIZE);
    while (fgets(buf, MAXLINE, fp) != NULL)
    {
        strcat(code, buf);
    }
    project = code;
    fclose(fp);
    return 0;
}

int write_base(char *filename,char *buf)
{
    FILE *fp = NULL;

    fp = fopen(filename, "w+");
    if (fp == NULL)
    {
        fprintf(stderr, "Create %s file failedd \n",filename);
        return -1;
    }

    if (fprintf(fp, "%s", buf) != 0)
    {
        fclose(fp);
        return 0;
    }
    else
    {
        fprintf(stderr, "write %s file  failed or no err log\n",filename);
        fclose(fp);
        return -1;
    }
}



int write(){
    write_base(DYD_FILE_NAME,dyd_file);
    write_base(ERR_FILE_NAME,err_file);
}

int main()
{

    read();
    printf("%s\n", project);
    while (LexAnalyze())
        ;
    write();
}