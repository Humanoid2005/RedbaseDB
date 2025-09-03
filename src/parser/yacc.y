%{
#include "abstract_syntax_tree.h"
#include "../datetime.h"
#include "yacc.tab.h"
#include <iostream>
#include <memory>

int yylex(YYSTYPE *yylval, YYLTYPE *yylloc);

void yyerror(YYLTYPE *locp, const char* s) {
    std::cerr << "Parser Error at line " << locp->first_line << " column " << locp->first_column << ": " << s << std::endl;
}

using namespace ast;
%}

%union {
    std::shared_ptr<ast::TreeNode> sv_tree_node;
    std::shared_ptr<ast::Field> sv_field;
    std::vector<std::shared_ptr<ast::Field>> sv_fields;
    std::shared_ptr<ast::TypeLen> sv_type_len;
    ast::SV_ComparatorOps sv_comparator_op;
    std::shared_ptr<ast::Expression> sv_expression;
    std::shared_ptr<ast::Value> sv_value;
    std::vector<std::shared_ptr<ast::Value>> sv_values;
    std::string sv_string;
    std::vector<std::string> sv_strs;
    std::shared_ptr<ast::Column> sv_column;
    std::vector<std::shared_ptr<ast::Column>> sv_columns;
    std::shared_ptr<ast::SetClause> sv_set_clause;
    std::vector<std::shared_ptr<ast::SetClause>> sv_set_clauses;
    std::shared_ptr<ast::BinaryExpression> sv_condition;
    std::vector<std::shared_ptr<ast::BinaryExpression>> sv_conditions;
    int sv_int;
    float sv_float;
    DateTime sv_datetime;
}

// request a pure (reentrant) parser
%define api.pure full
// enable location in error handler
%locations
// enable verbose syntax error message
%define parse.error verbose

// keywords
%token SHOW TABLES CREATE TABLE DROP DESC INSERT INTO VALUES DELETE FROM USE
WHERE UPDATE SET SELECT INT CHAR FLOAT DATETIME INDEX AND EXIT HELP DATABASE DATABASES
// non-keywords
%token LEQ NEQ GEQ T_EOF

// type-specific tokens
%token <sv_string> IDENTIFIER VALUE_STRING
%token <sv_int> VALUE_INT
%token <sv_float> VALUE_FLOAT
%token <sv_datetime> VALUE_DATETIME

// specify types for non-terminal symbol
%type <sv_tree_node> stmt dbStmt ddl dml
%type <sv_field> field
%type <sv_fields> fieldList
%type <sv_type_len> type
%type <sv_comparator_op> op
%type <sv_expression> expr
%type <sv_value> value
%type <sv_values> valueList
%type <sv_string> tbName colName dbName
%type <sv_strs> tableList
%type <sv_column> col
%type <sv_columns> colList selector
%type <sv_set_clause> setClause
%type <sv_set_clauses> setClauses
%type <sv_condition> condition
%type <sv_conditions> whereClause optWhereClause

%%
start:
        stmt ';'
    {
        parse_tree = $1;
        YYACCEPT;
    }
    |   HELP
    {
        parse_tree = std::make_shared<Help>();
        YYACCEPT;
    }
    |   EXIT
    {
        parse_tree = nullptr;
        YYACCEPT;
    }
    |   T_EOF
    {
        parse_tree = nullptr;
        YYACCEPT;
    }
    ;

stmt:
        dbStmt
    |   ddl
    |   dml
    ;

dbStmt:
        SHOW TABLES
    {
        $$ = std::make_shared<ShowTables>();
    }
    |   SHOW DATABASES
    {
        $$ = std::make_shared<ShowTables>(); // Note: AST doesn't have ShowDatabases, using ShowTables
    }
    |   CREATE DATABASE dbName
    {
        $$ = std::make_shared<CreateDB>($3);
    }
    |   USE DATABASE dbName
    {
        $$ = std::make_shared<UseDB>($3);
    }
    |   USE dbName
    {
        $$ = std::make_shared<UseDB>($2);
    }
    |   EXIT DATABASE
    {
        $$ = std::make_shared<ExitDB>("");
    }
    |   DROP DATABASE dbName
    {
        $$ = std::make_shared<DropDB>($3);
    }
    ;

ddl:
        CREATE TABLE tbName '(' fieldList ')'
    {
        $$ = std::make_shared<CreateTable>($3, $5);
    }
    |   DROP TABLE tbName
    {
        $$ = std::make_shared<DropTable>($3);
    }
    |   DESC tbName
    {
        $$ = std::make_shared<DescTable>($2);
    }
    |   CREATE INDEX tbName '(' colName ')'
    {
        $$ = std::make_shared<CreateIndex>($3, $5);
    }
    |   DROP INDEX tbName '(' colName ')'
    {
        $$ = std::make_shared<DropIndex>($3, $5);
    }
    ;

dml:
        INSERT INTO tbName VALUES '(' valueList ')'
    {
        $$ = std::make_shared<InsertStatement>($3, $6);
    }
    |   DELETE FROM tbName optWhereClause
    {
        $$ = std::make_shared<DeleteStatement>($3, $4);
    }
    |   UPDATE tbName SET setClauses optWhereClause
    {
        $$ = std::make_shared<UpdateStatement>($2, $4, $5);
    }
    |   SELECT selector FROM tableList optWhereClause
    {
        $$ = std::make_shared<SelectStatement>($2, $4, $5);
    }
    ;

fieldList:
        field
    {
        $$ = std::vector<std::shared_ptr<Field>>{$1};
    }
    |   fieldList ',' field
    {
        $$.push_back($3);
    }
    ;

field:
        colName type
    {
        $$ = std::make_shared<ColumnDefination>($1, $2);
    }
    ;

type:
        INT
    {
        $$ = std::make_shared<TypeLen>(SV_TYPE_INT, sizeof(int));
    }
    |   CHAR '(' VALUE_INT ')'
    {
        $$ = std::make_shared<TypeLen>(SV_TYPE_STRING, $3);
    }
    |   FLOAT
    {
        $$ = std::make_shared<TypeLen>(SV_TYPE_FLOAT, sizeof(float));
    }
    |   DATETIME
    {
        $$ = std::make_shared<TypeLen>(SV_TYPE_DATETIME, sizeof(DateTime));
    }
    ;

valueList:
        value
    {
        $$ = std::vector<std::shared_ptr<Value>>{$1};
    }
    |   valueList ',' value
    {
        $$.push_back($3);
    }
    ;

value:
        VALUE_INT
    {
        $$ = std::make_shared<IntLiteral>($1);
    }
    |   VALUE_FLOAT
    {
        $$ = std::make_shared<FloatLiteral>($1);
    }
    |   VALUE_STRING
    {
        $$ = std::make_shared<StringLiteral>($1);
    }
    |   VALUE_DATETIME
    {
        $$ = std::make_shared<DateTimeLiteral>($1);
    }
    ;

condition:
        col op expr
    {
        $$ = std::make_shared<BinaryExpression>($1, $2, $3);
    }
    ;

optWhereClause:
        /* epsilon */ 
    {
        $$ = std::vector<std::shared_ptr<BinaryExpression>>{};
    }
    |   WHERE whereClause
    {
        $$ = $2;
    }
    ;

whereClause:
        condition
    {
        $$ = std::vector<std::shared_ptr<BinaryExpression>>{$1};
    }
    |   whereClause AND condition
    {
        $$.push_back($3);
    }
    ;

col:
        tbName '.' colName
    {
        $$ = std::make_shared<Column>($1, $3);
    }
    |   colName
    {
        $$ = std::make_shared<Column>("", $1);
    }
    ;

colList:
        col
    {
        $$ = std::vector<std::shared_ptr<Column>>{$1};
    }
    |   colList ',' col
    {
        $$.push_back($3);
    }
    ;

op:
        '='
    {
        $$ = SV_OP_EQ;
    }
    |   '<'
    {
        $$ = SV_OP_LT;
    }
    |   '>'
    {
        $$ = SV_OP_GT;
    }
    |   NEQ
    {
        $$ = SV_OP_NE;
    }
    |   LEQ
    {
        $$ = SV_OP_LE;
    }
    |   GEQ
    {
        $$ = SV_OP_GE;
    }
    ;

expr:
        value
    {
        $$ = std::static_pointer_cast<Expression>($1);
    }
    |   col
    {
        $$ = std::static_pointer_cast<Expression>($1);
    }
    ;

setClauses:
        setClause
    {
        $$ = std::vector<std::shared_ptr<SetClause>>{$1};
    }
    |   setClauses ',' setClause
    {
        $$.push_back($3);
    }
    ;

setClause:
        colName '=' value
    {
        $$ = std::make_shared<SetClause>($1, $3);
    }
    ;

selector:
        '*'
    {
        $$ = {};
    }
    |   colList
    ;

tableList:
        tbName
    {
        $$ = std::vector<std::string>{$1};
    }
    |   tableList ',' tbName
    {
        $$.push_back($3);
    }
    ;

tbName: IDENTIFIER;

colName: IDENTIFIER;

dbName: IDENTIFIER;
%%