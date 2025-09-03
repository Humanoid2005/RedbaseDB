#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

//sv--> semantic value
#include <memory>
#include <string>
#include <vector>
#include "../datetime.h"

namespace ast{

    enum SV_Type{SV_TYPE_INT,SV_TYPE_FLOAT,SV_TYPE_STRING,SV_TYPE_DATETIME};

    enum SV_ComparatorOps{SV_OP_EQ,SV_OP_NE,SV_OP_LT,SV_OP_GT,SV_OP_LE,SV_OP_GE};

    class TreeNode{
    public:
        virtual ~TreeNode() = default; //virtual destructor for polymorphism
    };

    class Help: public TreeNode{};

    class ShowTables: public TreeNode{};

    class TypeLen: public TreeNode{
    public:
        SV_Type type;
        int len;

        TypeLen(SV_Type type,int len){
            this->type = type;
            this->len = len;
        }
    };

    class Field: public TreeNode {};

    class ColumnDefination : public Field{
    public:
        std::string column_name;
        std::shared_ptr<TypeLen> type_len;

        ColumnDefination(std::string column_name,std::shared_ptr<TypeLen>type_len){
            this->column_name;
            this->type_len;
        }
    };

    class CreateDB : public TreeNode{
    public:
        std::string db_name;
        
        CreateDB(std::string db_name){
            this->db_name = db_name;
        }
    };

    class DropDB : public TreeNode{
    public:
        std::string db_name;
            
        DropDB(std::string db_name){
            this->db_name = db_name;
        }
    };

    class UseDB : public TreeNode{
    public:
        std::string db_name;
            
        UseDB(std::string db_name){
            this->db_name = db_name;
        }
    };

    class ExitDB : public TreeNode{
    public:
        std::string db_name;
            
        ExitDB(std::string db_name){
            this->db_name = db_name;
        }
    };

    class CreateTable : public TreeNode{
    public:
        std::string table_name;
        std::vector<std::shared_ptr<Field>>fields;

        CreateTable(std::string table_name,std::vector<std::shared_ptr<Field>>fields){
            this->table_name = table_name;
            this->fields = fields;
        }
    };

    class DropTable : public TreeNode{
    public:
        std::string table_name;

        DropTable(std::string table_name){
            this->table_name = table_name;
        }
    };

    class DescTable : public TreeNode{
    public:
        std::string table_name;

        DescTable(std::string table_name){
            this->table_name = table_name;
        }
    };

    class CreateIndex : public TreeNode{
    public:
        std::string table_name;
        std::string column_name;

        CreateIndex(std::string table_name,std::string column_name){
            this->table_name = table_name;
            this->column_name = column_name;
        }
    };

    class DropIndex : public TreeNode{
    public:
        std::string table_name;
        std::string column_name;
    
        DropIndex(std::string table_name,std::string column_name){
            this->table_name = table_name;
            this->column_name = column_name;
        }
    };

    class Expression : public TreeNode {};

    class Value: public Expression {};

    class IntLiteral: public Value{
    public:
        int val;

        IntLiteral(int val){
            this->val = val;
        }
    };

    class FloatLiteral: public Value{
    public:
        float val;

        FloatLiteral(float val){
            this->val = val;
        }
    };

    class StringLiteral: public Value{
    public:
        std::string val;

        StringLiteral(std::string val){
            this->val = val;
        }
    };

    class DateTimeLiteral: public Value{
    public:
        DateTime val;

        DateTimeLiteral(DateTime val){
            this->val = val;
        }
    };

    class Column: public Expression{
    public:
        std::string table_name;
        std::string column_name;

        Column(std::string table_name,std::string column_name){
            this->table_name = table_name;
            this->column_name = column_name;
        }
    };

    class SetClause : public TreeNode{
    public:
        std::string column_name;
        std::shared_ptr<Value> val;
        
        SetClause(std::string column_name,std::shared_ptr<Value>val){
            this->column_name = column_name;
            this->val = val;
        }
    };

    class BinaryExpression: public TreeNode{
    public:
        std::shared_ptr<Column> lhs;
        SV_ComparatorOps op;
        std::shared_ptr<Expression>rhs;

        BinaryExpression(std::shared_ptr<Column> lhs, SV_ComparatorOps op, std::shared_ptr<Expression> rhs) {
            this->lhs = lhs;
            this->op = op;
            this->rhs = rhs;
        }
    };

    class InsertStatement : public TreeNode {
    public:
        std::string table_name;
        std::vector<std::shared_ptr<Value>> values;

        InsertStatement(std::string table_name, std::vector<std::shared_ptr<Value>> values) {
            this->table_name = table_name;
            this->values = values;
        }
    };

    class DeleteStatement : public TreeNode {
    public:
        std::string table_name;
        std::vector<std::shared_ptr<BinaryExpression>> conditions;

        DeleteStatement(std::string table_name, std::vector<std::shared_ptr<BinaryExpression>> conditions) {
            this->table_name = table_name;
            this->conditions = conditions;
        }
    };

    class UpdateStatement : public TreeNode {
    public:
        std::string table_name;
        std::vector<std::shared_ptr<SetClause>> set_clauses;
        std::vector<std::shared_ptr<BinaryExpression>> conditions;

        UpdateStatement(std::string table_name, std::vector<std::shared_ptr<SetClause>> set_clauses, std::vector<std::shared_ptr<BinaryExpression>> conditions) {
            this->table_name = table_name;
            this->set_clauses = set_clauses;
            this->conditions = conditions;
        }
    };

    class SelectStatement : public TreeNode {
    public:
        std::vector<std::shared_ptr<Column>> columns;
        std::vector<std::string> tables;
        std::vector<std::shared_ptr<BinaryExpression>> conditions;

        SelectStatement(std::vector<std::shared_ptr<Column>> columns, std::vector<std::string> tables, std::vector<std::shared_ptr<BinaryExpression>> conditions) {
            this->columns = columns;
            this->tables = tables;
            this->conditions = conditions;
        }
    };

    class SemanticValue {
    public:
        int sv_int;
        float sv_float;
        std::string sv_string;
        DateTime sv_datetime;
        std::vector<std::string>sv_strs;
        std::shared_ptr<TreeNode> sv_tree_node;
        SV_ComparatorOps sv_comparator_op;
        std::shared_ptr<TypeLen> sv_type_len;
        std::shared_ptr<Field> sv_field;
        std::vector<std::shared_ptr<Field>> sv_fields;
        std::shared_ptr<Expression> sv_expression;
        std::shared_ptr<Value> sv_value;
        std::vector<std::shared_ptr<Value>> sv_values;
        std::shared_ptr<Column> sv_column;
        std::vector<std::shared_ptr<Column>> sv_columns;
        std::shared_ptr<SetClause> sv_set_clause;
        std::vector<std::shared_ptr<SetClause>> sv_set_clauses;
        std::shared_ptr<BinaryExpression> sv_condition;
        std::vector<std::shared_ptr<BinaryExpression>> sv_conditions;
    };

    extern std::shared_ptr<ast::TreeNode> parse_tree;
};

#define YYSTYPE ast::SemanticValue

#endif