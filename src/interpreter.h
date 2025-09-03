#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "parser/parser.h"
#include "query_language/ql.h"
#include "system_management/sm.h"
#include <map>

class Interpreter{
private:
    static ColumnType interpret_sv_type(ast::SV_Type type){
        switch (type){
            case ast::SV_TYPE_INT:
                return TYPE_INT;
            case ast::SV_TYPE_FLOAT:
                return TYPE_FLOAT;
            case ast::SV_TYPE_STRING:
                return TYPE_STRING;
            case ast::SV_TYPE_DATETIME:
                return TYPE_DATETIME;
            default:
                break;
        }
    }

    static ComparatorOps interpret_sv_comp_op(ast::SV_ComparatorOps op){
        switch (op){
            case ast::SV_OP_EQ:
                return OP_EQ;
            case ast::SV_OP_NE:
                return OP_NE;
            case ast::SV_OP_LT:
                return OP_LT;
            case ast::SV_OP_GT:
                return OP_GT;
            case ast::SV_OP_LE:
                return OP_LE;
            case ast::SV_OP_GE:
                return OP_GE;
            default:
                break;
        }
    }

    static Value interpret_sv_value(const std::shared_ptr<ast::Value> &sv_val) {
        Value val;
        if (auto int_lit = std::dynamic_pointer_cast<ast::IntLiteral>(sv_val)) {
            val.set_int(int_lit->val);
        } else if (auto float_lit = std::dynamic_pointer_cast<ast::FloatLiteral>(sv_val)) {
            val.set_float(float_lit->val);
        } else if (auto str_lit = std::dynamic_pointer_cast<ast::StringLiteral>(sv_val)) {
            val.set_str(str_lit->val);
        } else {
            throw InternalError("Unexpected sv value type");
        }
        return val;
    }

    static std::vector<Condition> interp_where_clause(const std::vector<std::shared_ptr<ast::BinaryExpression>> &sv_conds) {
        std::vector<Condition> conds;
        for (auto &expr : sv_conds) {
            Condition cond;
            cond.lhs_col = TableColumn(expr->lhs->table_name, expr->lhs->column_name);
            cond.op = interpret_sv_comp_op(expr->op);
            if (auto rhs_val = std::dynamic_pointer_cast<ast::Value>(expr->rhs)) {
                cond.is_rhs_val = true;
                cond.rhs_val = interpret_sv_value(rhs_val);
            } else if (auto rhs_col = std::dynamic_pointer_cast<ast::Column>(expr->rhs)) {
                cond.is_rhs_val = false;
                cond.rhs_col = TableColumn(rhs_col->table_name, rhs_col->column_name);
            }
            conds.push_back(cond);
        }
        return conds;
    }

public:
    static void interpret_sql(const std::shared_ptr<ast::TreeNode> &root) {
        if (auto x = std::dynamic_pointer_cast<ast::Help>(root)) {
            std::cout
                << "Supported SQL syntax:\n"
                "  command ;\n"
                "command:\n"
                "  CREATE DATABASE database_name"
                "  USE DATABASE database_name"
                "  CREATE TABLE table_name (column_name type [, column_name type ...])\n"
                "  DROP TABLE table_name\n"
                "  CREATE INDEX table_name (column_name)\n"
                "  DROP INDEX table_name (column_name)\n"
                "  INSERT INTO table_name VALUES (value [, value ...])\n"
                "  DELETE FROM table_name [WHERE where_clause]\n"
                "  UPDATE table_name SET column_name = value [, column_name = value ...] [WHERE where_clause]\n"
                "  SELECT selector FROM table_name [WHERE where_clause]\n"
                "type:\n"
                "  {INT | FLOAT | CHAR(n) | DATETIME}\n"
                "where_clause:\n"
                "  condition [AND condition ...]\n"
                "condition:\n"
                "  column op {column | value}\n"
                "column:\n"
                "  [table_name.]column_name\n"
                "op:\n"
                "  {= | <> | < | > | <= | >=}\n"
                "selector:\n"
                "  {* | column [, column ...]}\n";
        } else if (auto x = std::dynamic_pointer_cast<ast::ShowTables>(root)) {
            SM_Manager::show_tables();
        }else if(auto x = std::dynamic_pointer_cast<ast::CreateDB>(root)){
            SM_Manager::create_db(x->db_name);
        }else if(auto x = std::dynamic_pointer_cast<ast::DropDB>(root)){
            SM_Manager::drop_db(x->db_name);
        }else if(auto x = std::dynamic_pointer_cast<ast::UseDB>(root)){
            SM_Manager::use_db(x->db_name);
        }else if(auto x = std::dynamic_pointer_cast<ast::ExitDB>(root)){
            SM_Manager::exit_db();
        }else if (auto x = std::dynamic_pointer_cast<ast::DescTable>(root)) {
            SM_Manager::desc_table(x->table_name);
        } else if (auto x = std::dynamic_pointer_cast<ast::CreateTable>(root)) {
            std::vector<ast::ColumnDefination> col_defs;
            for (auto &field : x->fields) {
                if (auto sv_col_def = std::dynamic_pointer_cast<ast::ColumnDefination>(field)) {
                    ast::ColumnDefination col_def(sv_col_def->column_name,sv_col_def->type_len);
                    col_defs.push_back(col_def);
                } else {
                    throw InternalError("Unexpected field type");
                }
            }
            std::vector<Column>cols;
            for(int i=0;i<col_defs.size();i++){
                Column col;
                col.column_name = col_defs[i].column_name;
                col.length = col_defs[i].type_len.get()->len;
                if(col_defs[i].type_len.get()->type==ast::SV_TYPE_INT){
                    col.type = TYPE_INT;
                }
                else if(col_defs[i].type_len.get()->type==ast::SV_TYPE_FLOAT){
                    col.type = TYPE_FLOAT;
                }
                else if(col_defs[i].type_len.get()->type==ast::SV_TYPE_STRING){
                    col.type = TYPE_STRING;
                }
                else if(col_defs[i].type_len.get()->type==ast::SV_TYPE_DATETIME){
                    col.type = TYPE_DATETIME;
                }
                cols.push_back(col);                
            }
            SM_Manager::create_table(x->table_name,cols);
        } else if (auto x = std::dynamic_pointer_cast<ast::DropTable>(root)) {
            SM_Manager::drop_table(x->table_name);
        } else if (auto x = std::dynamic_pointer_cast<ast::CreateIndex>(root)) {
            SM_Manager::create_index(x->table_name, x->column_name);
        } else if (auto x = std::dynamic_pointer_cast<ast::DropIndex>(root)) {
            SM_Manager::drop_index(x->table_name, x->column_name);
        } else if (auto x = std::dynamic_pointer_cast<ast::InsertStatement>(root)) {
            std::vector<Value> values;
            for (auto &sv_val : x->values) {
                values.push_back(interpret_sv_value(sv_val));
            }
            QL_Manager::insert_into(x->table_name, values);
        } else if (auto x = std::dynamic_pointer_cast<ast::DeleteStatement>(root)) {
            std::vector<Condition> conds = interp_where_clause(x->conditions);
            QL_Manager::delete_from(x->table_name, conds);
        } else if (auto x = std::dynamic_pointer_cast<ast::UpdateStatement>(root)) {
            std::vector<Condition> conds = interp_where_clause(x->conditions);
            std::vector<SetClause> set_clauses;
            for (auto &sv_set_clause : x->set_clauses) {
                SetClause set_clause(TableColumn("", sv_set_clause->column_name), interpret_sv_value(sv_set_clause->val));
                set_clauses.push_back(set_clause);
            }
            QL_Manager::update_set(x->table_name, set_clauses, conds);
        } else if (auto x = std::dynamic_pointer_cast<ast::SelectStatement>(root)) {
            std::vector<Condition> conds = interp_where_clause(x->conditions);
            std::vector<TableColumn> sel_cols;
            for (auto &sv_sel_col : x->columns) {
                TableColumn sel_col(sv_sel_col->table_name, sv_sel_col->column_name);
                sel_cols.push_back(sel_col);
            }
            QL_Manager::select_from(sel_cols, x->tables, conds);
        } else {
            throw InternalError("Unexpected AST root");
        }
    }
};

#endif