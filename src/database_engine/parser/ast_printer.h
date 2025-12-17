#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "database_engine/parser/ast.h"
#include <cassert>
#include <sstream>
#include <map>

namespace ast {

class TreePrinter {
  public:
    static std::string print(const std::shared_ptr<TreeNode> &node) { 
        std::ostringstream oss;
        print_node(node, 0, oss); 
        return oss.str();
    }

  private:
    static std::string offset2string(int offset) { return std::string(offset, ' '); }

    template <typename T>
    static void print_val(const T &val, int offset, std::ostringstream &oss) {
        oss << offset2string(offset) << val << '\n';
    }

    template <typename T>
    static void print_val_list(const std::vector<T> &vals, int offset, std::ostringstream &oss) {
        oss << offset2string(offset) << "LIST\n";
        offset += 2;
        for (auto &val : vals) {
            print_val(val, offset, oss);
        }
    }

    static std::string type2str(SvType type) {
        static std::map<SvType, std::string> m{
            {SV_TYPE_INT, "INT"},
            {SV_TYPE_FLOAT, "FLOAT"},
            {SV_TYPE_STRING, "STRING"},
            {SV_TYPE_DATETIME, "DATETIME"},
        };
        return m.at(type);
    }

    static std::string op2str(SvCompOp op) {
        static std::map<SvCompOp, std::string> m{
            {SV_OP_EQ, "=="}, {SV_OP_NE, "!="}, {SV_OP_LT, "<"}, {SV_OP_GT, ">"}, {SV_OP_LE, "<="}, {SV_OP_GE, ">="},
        };
        return m.at(op);
    }

    template <typename T>
    static void print_node_list(std::vector<T> nodes, int offset, std::ostringstream &oss) {
        oss << offset2string(offset);
        offset += 2;
        oss << "LIST\n";
        for (auto &node : nodes) {
            print_node(node, offset, oss);
        }
    }

    static void print_node(const std::shared_ptr<TreeNode> &node, int offset, std::ostringstream &oss) {
        oss << offset2string(offset);
        offset += 2;
        if (auto x = std::dynamic_pointer_cast<Help>(node)) {
            oss << "HELP\n";
        } else if (auto x = std::dynamic_pointer_cast<ShowTables>(node)) {
            oss << "SHOW_TABLES\n";
        } else if (auto x = std::dynamic_pointer_cast<ShowDatabases>(node)) {
            oss << "SHOW_DATABASES\n";
        } else if (auto x = std::dynamic_pointer_cast<CreateDatabase>(node)) {
            oss << "CREATE_DATABASE\n";
            print_val(x->db_name, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<DropDatabase>(node)) {
            oss << "DROP_DATABASE\n";
            print_val(x->db_name, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<UseDatabase>(node)) {
            oss << "USE_DATABASE\n";
            print_val(x->db_name, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<CreateTable>(node)) {
            oss << "CREATE_TABLE\n";
            print_val(x->tab_name, offset, oss);
            print_node_list(x->fields, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<DropTable>(node)) {
            oss << "DROP_TABLE\n";
            print_val(x->tab_name, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<DescTable>(node)) {
            oss << "DESC_TABLE\n";
            print_val(x->tab_name, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<CreateIndex>(node)) {
            oss << "CREATE_INDEX\n";
            print_val(x->tab_name, offset, oss);
            print_val(x->col_name, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<DropIndex>(node)) {
            oss << "DROP_INDEX\n";
            print_val(x->tab_name, offset, oss);
            print_val(x->col_name, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<ColDef>(node)) {
            oss << "COL_DEF\n";
            print_val(x->col_name, offset, oss);
            print_node(x->type_len, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<Col>(node)) {
            oss << "COL\n";
            print_val(x->tab_name, offset, oss);
            print_val(x->col_name, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<TypeLen>(node)) {
            oss << "TYPE_LEN\n";
            print_val(type2str(x->type), offset, oss);
            print_val(x->len, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<IntLit>(node)) {
            oss << "INT_LIT\n";
            print_val(x->val, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<FloatLit>(node)) {
            oss << "FLOAT_LIT\n";
            print_val(x->val, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<StringLit>(node)) {
            oss << "STRING_LIT\n";
            print_val(x->val, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<DateTimeLit>(node)) {
            oss << "DATETIME_LIT\n";
            print_val(x->val, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<SetClause>(node)) {
            oss << "SET_CLAUSE\n";
            print_val(x->col_name, offset, oss);
            print_node(x->val, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<BinaryExpr>(node)) {
            oss << "BINARY_EXPR\n";
            print_node(x->lhs, offset, oss);
            print_val(op2str(x->op), offset, oss);
            print_node(x->rhs, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<InsertStmt>(node)) {
            oss << "INSERT\n";
            print_val(x->tab_name, offset, oss);
            print_node_list(x->vals, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<DeleteStmt>(node)) {
            oss << "DELETE\n";
            print_val(x->tab_name, offset, oss);
            print_node_list(x->conds, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<UpdateStmt>(node)) {
            oss << "UPDATE\n";
            print_val(x->tab_name, offset, oss);
            print_node_list(x->set_clauses, offset, oss);
            print_node_list(x->conds, offset, oss);
        } else if (auto x = std::dynamic_pointer_cast<SelectStmt>(node)) {
            oss << "SELECT\n";
            print_node_list(x->cols, offset, oss);
            print_val_list(x->tabs, offset, oss);
            print_node_list(x->conds, offset, oss);
        } else {
            assert(0);
        }
    }
};

}

#endif
