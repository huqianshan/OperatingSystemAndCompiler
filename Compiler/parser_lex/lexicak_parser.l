%{
typedef enum {
    T_Le = 256, T_Ge, T_Eq, T_Ne, T_And, T_Or, T_IntConstant,
    T_StringConstant, T_Identifier, T_Void, T_Int, T_While,T_For,
    T_If, T_Else, T_Return, T_Break, T_Continue, T_Printf,
    T_Main
} TokenType;

static void print_token(int token) {
    static char* token_strs[] = {
        "T_Le", "T_Ge", "T_Eq", "T_Ne", "T_And", "T_Or", "T_IntConstant",
        "T_StringConstant", "T_Identifier", "T_Void", "T_Int", "T_While","T_FOr",
        "T_If", "T_Else", "T_Return", "T_Break", "T_Continue", "T_Printf",
        "T_Main"
    };

    if (token < 256) {
        printf("%-30c", token);
    } else {
        printf("%-30s", token_strs[token-256]);
    }
}
void init_scanner();
void lex_error(char* msg, int line);
int line=0;
%}

NUMBER		([0-9]+)
STRING      (\"[^\”\n]*\")
HEADER      ("#"[^\n]*)
      

%%
[\n]                { line++;                   }
[ \t\r\a]+          { /* ignore all spaces */   }

{HEADER}            {/* header*/}

"void"              { return T_Void;            }
"int"               { return T_Int;             }
"while"             { return T_While;           }
"if"                { return T_If;              }
"else"              { return T_Else;            }
"return"            { return T_Return;          }
"break"             { return T_Break;           }
"continue"          { return T_Continue;        }
"printf"            { return T_Printf;          }
"main"              {return T_Main;             }
{NUMBER}            {return T_IntConstant;      }
{STRING}            {return T_StringConstant;   }

.                   {lex_error(yytext,line);    }
<<EOF>>             {return 0;                  }
%%


int main(int argc, char* argv[]) {
    int token;
    init_scanner();
    while (token = yylex()) {
        print_token(token);
        printf(yytext);
        printf("\n");
    }
    return 0;
}

void init_scanner() {
    printf("%-30s%s\n", "TOKEN-TYPE", "TOKEN-VALUE");
    printf("-------------------------------------------------\n");
}

void lex_error(char* msg, int line) {
    printf("\nError at line %-3d: %s\n\n", line, msg);
}

int yywrap(void) {
    return 1;
}

