#pragma once

#include "interpreter_defs.h"
#include "database_engine/parser/parser.h"
#include "database_engine/query_language/ql.h"
#include "database_engine/system_management/sm.h"
#include <map>

class Interpreter {
  public:
    static InterpreterResult interp_sql(const std::shared_ptr<ast::TreeNode> &root) {
        if (auto x = std::dynamic_pointer_cast<ast::Help>(root)) {
            HelpResult result(
                "Supported SQL syntax:\n"
                "  command ;\n"
                "command:\n"
                "  SHOW DATABASES\n"
                "  CREATE DATABASE database_name\n"
                "  DROP DATABASE database_name\n"
                "  USE database_name\n"
                "  SHOW TABLES\n"
                "  DESC table_name\n"
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
                "  {* | column [, column ...]}\n"
            );
            return InterpreterResult(CMD_HELP, true, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::ShowTables>(root)) {
            ShowTablesResult result = SM_Manager::show_tables();
            return InterpreterResult(CMD_SHOW_TABLES, true, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::ShowDatabases>(root)) {
            std::vector<std::string> databases = SM_Manager::list_databases();
            ShowDatabasesResult result;
            for (const auto& db : databases) {
                result.add_database(db);
            }
            return InterpreterResult(CMD_SHOW_DATABASES, true, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::CreateDatabase>(root)) {
            SM_Manager::create_db(x->db_name);
            DatabaseOperationResult result(x->db_name, true, "Database created successfully");
            return InterpreterResult(CMD_CREATE_DATABASE, true, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::DropDatabase>(root)) {
            SM_Manager::drop_db(x->db_name);
            DatabaseOperationResult result(x->db_name, true, "Database dropped successfully");
            return InterpreterResult(CMD_DROP_DATABASE, true, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::UseDatabase>(root)) {
            SM_Manager::open_db(x->db_name);
            DatabaseOperationResult result(x->db_name, true, "Switched to database");
            return InterpreterResult(CMD_USE_DATABASE, true, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::DescTable>(root)) {
            DescTableResult result = SM_Manager::desc_table(x->tab_name);
            return InterpreterResult(CMD_DESC_TABLE, true, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::CreateTable>(root)) {
            std::vector<ColumnInfo> col_defs;
            for (auto &field : x->fields) {
                if (auto sv_col_def = std::dynamic_pointer_cast<ast::ColDef>(field)) {
                    ColumnInfo col_def(sv_col_def->col_name, 
                                      interp_sv_type(sv_col_def->type_len->type),
                                      sv_col_def->type_len->len);
                    col_defs.push_back(col_def);
                } else {
                    throw InternalError("Unexpected field type");
                }
            }
            SM_Manager::create_table(x->tab_name, col_defs);
            DDLResult result("table", x->tab_name, true, "Table created successfully");
            return InterpreterResult(CMD_CREATE_TABLE, true, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::DropTable>(root)) {
            SM_Manager::drop_table(x->tab_name);
            DDLResult result("table", x->tab_name, true, "Table dropped successfully");
            return InterpreterResult(CMD_DROP_TABLE, true, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::CreateIndex>(root)) {
            SM_Manager::create_index(x->tab_name, x->col_name);
            DDLResult result("index", x->tab_name + "." + x->col_name, true, "Index created successfully");
            return InterpreterResult(CMD_CREATE_INDEX, true, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::DropIndex>(root)) {
            SM_Manager::drop_index(x->tab_name, x->col_name);
            DDLResult result("index", x->tab_name + "." + x->col_name, true, "Index dropped successfully");
            return InterpreterResult(CMD_DROP_INDEX, true, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::InsertStmt>(root)) {
            std::vector<Value> values;
            for (auto &sv_val : x->vals) {
                values.push_back(interp_sv_value(sv_val));
            }
            InsertResult result = QL_Manager::insert_into(x->tab_name, values);
            return InterpreterResult(CMD_INSERT, result.success, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::DeleteStmt>(root)) {
            std::vector<Condition> conds = interp_where_clause(x->conds);
            DeleteResult result = QL_Manager::delete_from(x->tab_name, conds);
            return InterpreterResult(CMD_DELETE, result.success, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::UpdateStmt>(root)) {
            std::vector<Condition> conds = interp_where_clause(x->conds);
            std::vector<SetClause> set_clauses;
            for (auto &sv_set_clause : x->set_clauses) {
                SetClause set_clause(TabCol("", sv_set_clause->col_name), interp_sv_value(sv_set_clause->val));
                set_clauses.push_back(set_clause);
            }
            UpdateResult result = QL_Manager::update_set(x->tab_name, set_clauses, conds);
            return InterpreterResult(CMD_UPDATE, result.success, result);
        } else if (auto x = std::dynamic_pointer_cast<ast::SelectStmt>(root)) {
            std::vector<Condition> conds = interp_where_clause(x->conds);
            std::vector<TabCol> sel_cols;
            for (auto &sv_sel_col : x->cols) {
                TabCol sel_col(sv_sel_col->tab_name, sv_sel_col->col_name);
                sel_cols.push_back(sel_col);
            }
            SelectResult result = QL_Manager::select_from(sel_cols, x->tabs, conds);
            return InterpreterResult(CMD_SELECT, true, result);
        } else {
            throw InternalError("Unexpected AST root");
        }
    }

  private:
    static ColumnType interp_sv_type(ast::SvType sv_type) {
        std::map<ast::SvType, ColumnType> m = {
            {ast::SV_TYPE_INT, TYPE_INT}, 
            {ast::SV_TYPE_FLOAT, TYPE_FLOAT}, 
            {ast::SV_TYPE_STRING, TYPE_STRING},
            {ast::SV_TYPE_DATETIME, TYPE_DATETIME},
        };
        return m.at(sv_type);
    }

    static CompOp interp_sv_comp_op(ast::SvCompOp op) {
        static std::map<ast::SvCompOp, CompOp> m = {
            {ast::SV_OP_EQ, OP_EQ}, {ast::SV_OP_NE, OP_NE}, {ast::SV_OP_LT, OP_LT},
            {ast::SV_OP_GT, OP_GT}, {ast::SV_OP_LE, OP_LE}, {ast::SV_OP_GE, OP_GE},
        };
        return m.at(op);
    }

    static Value interp_sv_value(const std::shared_ptr<ast::Value> &sv_val) {
        Value val;
        if (auto int_lit = std::dynamic_pointer_cast<ast::IntLit>(sv_val)) {
            val.set_int(int_lit->val);
        } else if (auto float_lit = std::dynamic_pointer_cast<ast::FloatLit>(sv_val)) {
            val.set_float(float_lit->val);
        } else if (auto str_lit = std::dynamic_pointer_cast<ast::StringLit>(sv_val)) {
            val.set_str(str_lit->val);
        } else if (auto datetime_lit = std::dynamic_pointer_cast<ast::DateTimeLit>(sv_val)) {
            // DATETIME is stored as DateTime object
            val.set_datetime(datetime_lit->val);
        } else {
            throw InternalError("Unexpected sv value type");
        }
        return val;
    }

    static std::vector<Condition> interp_where_clause(const std::vector<std::shared_ptr<ast::BinaryExpr>> &sv_conds) {
        std::vector<Condition> conds;
        for (auto &expr : sv_conds) {
            Condition cond;
            cond.lhs_col = TabCol(expr->lhs->tab_name, expr->lhs->col_name);
            cond.op = interp_sv_comp_op(expr->op);
            if (auto rhs_val = std::dynamic_pointer_cast<ast::Value>(expr->rhs)) {
                cond.is_rhs_val = true;
                cond.rhs_val = interp_sv_value(rhs_val);
            } else if (auto rhs_col = std::dynamic_pointer_cast<ast::Col>(expr->rhs)) {
                cond.is_rhs_val = false;
                cond.rhs_col = TabCol(rhs_col->tab_name, rhs_col->col_name);
            }
            conds.push_back(cond);
        }
        return conds;
    }
};