#ifndef QL_MANAGER_H
#define QL_MANAGER_H

#include "ql_structs.h"
#include "../record_manager/rm.h"
#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

class TableColumn{
public:
    std::string table_name;
    std::string column_name;

    TableColumn() = default;
    TableColumn(std::string table_name,std::string column_name){
        this->table_name = table_name;
        this->column_name = column_name;
    }

    friend bool operator<(const TableColumn &a,const TableColumn &b){
        return std::make_pair(a.table_name,a.column_name) < std::make_pair(b.table_name,b.column_name);
    }
};

class Value{
public:
    ColumnType type;
    int int_val;
    float float_val;
    std::string str_val;
    DateTime datetime_val;

    std::shared_ptr<RM_Record> raw_record_buffer;

    void set_int(int int_val){
        this->type = TYPE_INT;
        this->int_val = int_val;
    }

    void set_float(float float_val){
        this->type = TYPE_FLOAT;
        this->float_val = float_val;
    }

    void set_str(std::string string_val){
        this->type = TYPE_STRING;
        this->str_val = string_val;
    }

    void set_datetime(DateTime dt){
        this->type = TYPE_DATETIME;
        this->datetime_val = dt;
    }

    void init_raw(int len){
        assert(this->raw_record_buffer==nullptr);
        this->raw_record_buffer = std::make_shared<RM_Record>(len);
        if(this->type==TYPE_INT){
            assert(len==sizeof(int));
            *(int*)(this->raw_record_buffer->data) = this->int_val;
        }
        else if(this->type==TYPE_FLOAT){
            assert(len==sizeof(float));
            *(float*)(this->raw_record_buffer->data) = this->float_val;
        }
        else if(this->type==TYPE_STRING){
            if(len< (int)this->str_val.size()){
                throw StringOverflowError();
            }
            memset(this->raw_record_buffer->data,0,len);
            memcpy(this->raw_record_buffer->data,str_val.c_str(),str_val.size());
        }
        else if(this->type==TYPE_DATETIME){
            assert(len==sizeof(DateTime));
            memset(this->raw_record_buffer->data,0,len);
            memcpy(this->raw_record_buffer->data,&(this->datetime_val),len);
        }
    }
};

typedef enum{
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE
} ComparatorOps;

class Condition{
public:
    TableColumn lhs_col;  // left-hand side column
    ComparatorOps op;       // comparison operator
    bool is_rhs_val; // true if right-hand side is a value (not a column)
    TableColumn rhs_col;  // right-hand side column
    Value rhs_val;   // right-hand side value
};

class SetClause{
public:
    TableColumn lhs;
    Value rhs;

    SetClause() = default;
    SetClause(TableColumn lhs,Value rhs){
        this->lhs = lhs;
        this->rhs = rhs;
    }
};

class QL_Manager{
public:
    static void insert_into(const std::string &table_name,std::vector<Value>values);

    static void delete_from(const std::string &table_name,std::vector<Condition>conditions);

    static void update_set(const std::string &table_name,std::vector<SetClause>set_clauses,std::vector<Condition>conditions);

    static void select_from(std::vector<TableColumn> select_columns,const std::vector<std::string>&table_names,std::vector<Condition>conditions);
};

#endif