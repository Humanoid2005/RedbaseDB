# Parser

## YACC (Yet Another Compiler Compiler)

- outputs a parse tree in LALR(1) (Look Ahead from Left to Right 1 look ahead) format
- used to parse tokens provided by lex tool
- yacc specifications : .y file

### Structure of .y file (YACC Specifications)

- Definations : % definations
                has all declarations of tokens,headers,constants
                %
- Rules : %%  rules 
              specifies actions to be taken when the token is matched with the grammar
          %%
- Auxilary Routine: % specifies function 
                      definations (main also)
                      return 0 -->success
                      return 1--> not successful
                    %


### Running of yacc file

.y -is passed to-->yacc compiler--generates-> .tab.c file
y.tab.c -- c compiler --> a.out (syntactic analyser i.e parse generator) 
tokens --> a.out --> parse tree


## Lex tool

- program that generates lexical analysers which is program that generates tokens

### Sequence of Lex program execution

.l ---lex compiler----> c program ---c compiler---> object code (lexical analyser)
stream of bits --object code--> tokens
