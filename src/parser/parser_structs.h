#ifndef PARSER_STRUCTS_H
#define PARSER_STRUCTS_H

int yyparse();

typedef struct yy_buffer_state * YY_BUFFER_STATE;

YY_BUFFER_STATE yy_scan_string(const char * string);

void yy_delete_buffer(YY_BUFFER_STATE buffer);

#endif