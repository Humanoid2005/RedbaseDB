#ifndef SM_META_H
#define SM_META_H

#include "../db_error.h"
#include "../db_structs.h"
#include "sm_structs.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

class Column_MetaData{
public:
    std::string table_name;
    std::string column_name;
    ColumnType type;
    int length;
    int offset;
    bool index;

    Column_MetaData() = default;

    Column_MetaData(std::string table_name,std::string column_name,ColumnType type,int length,int offset,bool index){
        this->table_name = table_name;
        this->column_name = column_name;
        this->type = type;
        this->offset = offset;
        this->length = length;
        this->index = index;
    }

    friend std::ostream &operator<<(std::ostream &out,const Column_MetaData &column){
        return out<<column.table_name<<" "<<column.column_name<<" "<<column.type<<" "<<column.length<<" "<<column.offset<<" "<<column.index;
    }

    friend std::istream &operator>>(std::istream &in,Column_MetaData &column){
        return in>>column.table_name>>column.column_name>>column.type>>column.length>>column.offset>>column.index;
    }
};

class Table_MetaData{
public:
    std::string table_name;
    std::vector<Column_MetaData> columns;

    bool is_column(const std::string &column_name) const{
        auto position = std::find_if(this->columns.begin(),this->columns.end(),[&](const Column_MetaData &column){
            return column_name == column.column_name; 
        });
        return position!=this->columns.end();
    }

    std::vector<Column_MetaData>::iterator get_column(const std::string &column_name){
        auto position = std::find_if(this->columns.begin(),this->columns.end(),[&](const Column_MetaData &column){
            return column_name == column.column_name; 
        });
        if(position==this->columns.end()){
            throw ColumnNotFoundError(column_name);
        }
        return position;
    }

    friend std::ostream &operator<<(std::ostream &out,const Table_MetaData &table){
        out<<table.table_name<<"\n"<<table.columns.size()<<"\n";
        for(auto &column:table.columns){
            out<<column<<"\n";
        }
        return out;
    }

    friend std::istream &operator>>(std::istream &in,Table_MetaData &table){
        size_t n;
        in>>table.table_name>>n;
        for(size_t i=0;i<n;i++){
            Column_MetaData column;
            in>>column;
            table.columns.push_back(column);
        }
        return in;
    }
};

class DB_MetaData{
public:
    std::string db_name;
    std::map<std::string,Table_MetaData>tables;

    bool is_table(const std::string &table_name){
        return this->tables.find(table_name)!=this->tables.end();
    }

    Table_MetaData &get_table(const std::string &table_name){
        auto position = tables.find(table_name);
        if(position==tables.end()){
            throw TableNotFoundError(table_name);
        }
        return position->second;
    }

    friend std::ostream &operator<<(std::ostream &out,const DB_MetaData &db){
        out<<db.db_name<<"\n"<<db.tables.size()<<"\n";
        for(auto &entry: db.tables){
            out<<entry.second<<"\n";
        }
        return out;
    }

    friend std::istream &operator>>(std::istream &in,DB_MetaData &db){
        size_t n;
        in>>db>>n;
        for(size_t i=0;i<n;i++){
            Table_MetaData table;
            in>>table;
            db.tables[table.table_name] = table;
        }
        return in;
    }
};

#endif