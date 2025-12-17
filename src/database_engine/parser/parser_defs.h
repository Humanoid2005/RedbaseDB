#pragma once

#include "database_engine/db_defs.h"
#include "database_engine/parser/ast.h"
#include <memory>
#include <string>

int yyparse();

typedef struct yy_buffer_state *YY_BUFFER_STATE;

YY_BUFFER_STATE yy_scan_string(const char *str);

void yy_delete_buffer(YY_BUFFER_STATE buffer);

// High-level parser API
inline std::shared_ptr<ast::TreeNode> parse_sql(const std::string& sql) {
    YY_BUFFER_STATE buf = yy_scan_string(sql.c_str());
    yyparse();
    yy_delete_buffer(buf);
    return ast::parse_tree;
}
