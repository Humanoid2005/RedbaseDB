#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "abstract_syntax_tree.h"
#include <cassert>
#include <iostream>
#include <map>

namespace ast{

class TreePrinter{
public:
    static void print(const std::shared_ptr<TreeNode>&node){
        print_node(node,0);
    }

private:
    static std::string offset2string(int offset){
        return std::string(offset,' ');
    }

    template <typename T>
    static void print_val(const T &val, int offset){
        std::cout << offset2string(offset)<<val<<"\n";
    }


    template<typename T>
    static void print_val_list(const std::vector<T>&vals,int offset){
        std::cout<<offset2string(offset)<<"LIST\n";
        offset += 2;
        for(auto &val:vals){
            print_val(val,offset);
        }
    }

    static std::string type2str(SV_Type type){
        switch (type){
            case SV_TYPE_INT:
                return "INT";
                break;
            case SV_TYPE_FLOAT:
                return "FLOAT";
                break;
            case SV_TYPE_STRING:
                return "STRING";
                break;
            case SV_TYPE_DATETIME:
                return "DATETIME";
                break;
            default:
                break;
        };
    }

    static std::string op2str(SV_ComparatorOps op){
        switch (op){
            case SV_OP_EQ:
                return "==";
                break;
            case SV_OP_NE:
                return "!=";
                break;
            case SV_OP_LT:
                return "<";
                break;
            case SV_OP_GT:
                return ">";
                break;
            case SV_OP_LE:
                return "<=";
                break;
            case SV_OP_GE:
                return ">=";
                break;
            default:
                break;
        }
    }

    template <typename T>
    static void print_node_list(std::vector<T>nodes,int offset){
        std::cout<<offset2string(offset);
        offset += 2;
        std::cout<<"LIST\n";
        for(auto &node:nodes){
            print_node(node,offset);
        }
    }

    static void print_node(const std::shared_ptr<TreeNode>&node,int offset){
        if(!node){
            std::cout<<offset2string(offset)<<"NULL\n";
            return;
        }
        std::cout<<offset2string(offset);
        offset += 2;
        if(auto help = std::dynamic_pointer_cast<Help>(node)){
            std::cout<<"HELP\n";
        }else if(auto show_tables = std::dynamic_pointer_cast<ShowTables>(node)){
            std::cout<<"SHOW TABLES\n";
        }else if(auto create_table = std::dynamic_pointer_cast<CreateTable>(node)){
            std::cout<<"CREATE TABLE\n";
            print_val(create_table->table_name,offset);
            print_node_list(create_table->fields,offset);
        }else if(auto drop_table = std::dynamic_pointer_cast<DropTable>(node)){
            std::cout<<"DROP TABLE\n";
            print_val(drop_table->table_name,offset);
        }else if(auto desc_table = std::dynamic_pointer_cast<DescTable>(node)){
            std::cout<<"DESC TABLE\n";
            print_val(desc_table->table_name,offset);
        }else if(auto create_index = std::dynamic_pointer_cast<CreateIndex>(node)){
            std::cout<<"CREATE INDEX\n";
            print_val(create_index->table_name,offset);
            print_val(create_index->column_name,offset);
        }else if(auto drop_index = std::dynamic_pointer_cast<DropIndex>(node)){
            std::cout<<"DROP INDEX\n";
            print_val(drop_index->table_name,offset);
            print_val(drop_index->column_name,offset);
        }else if(auto create_db = std::dynamic_pointer_cast<CreateDB>(node)){
            std::cout<<"CREATE DATABASE\n";
            print_val(create_db->db_name,offset);
        }else if(auto drop_db = std::dynamic_pointer_cast<DropDB>(node)){
            std::cout<<"DROP DATABASE\n";
            print_val(drop_db->db_name,offset);
        }else if(auto use_db = std::dynamic_pointer_cast<UseDB>(node)){
            std::cout<<"USE DATABASE\n";
            print_val(use_db->db_name,offset);
        }else if(auto exit_db = std::dynamic_pointer_cast<ExitDB>(node)){
            std::cout<<"EXIT DATABASE\n";
            print_val(exit_db->db_name,offset);
        }else if(auto col_def = std::dynamic_pointer_cast<ColumnDefination>(node)){
            std::cout<<"COLUMN DEFINITION\n";
            print_val(col_def->column_name,offset);
            print_val(col_def->type_len->type,offset);
        }
        else if(auto type_len = std::dynamic_pointer_cast<TypeLen>(node)){
            std::cout<<"TYPE LENGTH\n";
            print_val(type2str(type_len->type),offset);
            print_val(type_len->len,offset);
        }else if(auto int_literal = std::dynamic_pointer_cast<IntLiteral>(node)){
            std::cout<<"INT LITERAL\n";
            print_val(int_literal->val,offset);
        }else if(auto float_literal = std::dynamic_pointer_cast<FloatLiteral>(node)){
            std::cout<<"FLOAT LITERAL\n";
            print_val(float_literal->val,offset);
        }else if(auto string_literal = std::dynamic_pointer_cast<StringLiteral>(node)){
            std::cout<<"STRING LITERAL\n";
            print_val(string_literal->val,offset);
        }else if(auto datetime_literal = std::dynamic_pointer_cast<DateTimeLiteral>(node)){
            std::cout<<"DATETIME LITERAL\n";
            print_val(datetime_literal->val,offset);
        }else if(auto binary_expr = std::dynamic_pointer_cast<BinaryExpression>(node)){
            std::cout<<"BINARY EXPRESSION\n";
            print_node(binary_expr->lhs,offset);
            print_val(op2str(binary_expr->op),offset);
            print_node(binary_expr->rhs,offset);
        }else if(auto delete_stmt = std::dynamic_pointer_cast<DeleteStatement>(node)){
            std::cout<<"DELETE STATEMENT\n";
            print_val(delete_stmt->table_name,offset);
            print_node_list(delete_stmt->conditions,offset);
        }else if(auto update_stmt = std::dynamic_pointer_cast<UpdateStatement>(node)){
            std::cout<<"UPDATE STATEMENT\n";
            print_val(update_stmt->table_name,offset);
            print_node_list(update_stmt->set_clauses,offset);
            print_node_list(update_stmt->conditions,offset);
        }else if(auto insert_stmt = std::dynamic_pointer_cast<InsertStatement>(node)){
            std::cout<<"INSERT STATEMENT\n";
            print_val(insert_stmt->table_name,offset);
            print_node_list(insert_stmt->values,offset);
        }else if(auto select_stmt = std::dynamic_pointer_cast<SelectStatement>(node)){
            std::cout<<"SELECT STATEMENT\n";
            print_node_list(select_stmt->columns,offset);
            print_val_list(select_stmt->tables,offset);
            print_node_list(select_stmt->conditions,offset);
        }else if(auto column = std::dynamic_pointer_cast<Column>(node)){
            std::cout<<"COLUMN\n";
            print_val(column->table_name,offset);
            print_val(column->column_name,offset);
        }else if(auto set_clause = std::dynamic_pointer_cast<SetClause>(node)){
            std::cout<<"SET CLAUSE\n";
            print_val(set_clause->column_name,offset);
            print_node(set_clause->val,offset);
        }else{
            assert(0);
        }
    }
};
}

#endif