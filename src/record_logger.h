#ifndef RECORD_LOGGER_H
#define RECORD_LOGGER_H

#include <cassert>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

class RecordLogger{
private:
    static constexpr size_t COL_WIDTH = 16;
    size_t num_columns;
public:
    RecordLogger(size_t num_columns){
        assert(num_columns>0);
        this->num_columns = num_columns;
    }

    void print_separator() const{
        for(size_t i=0;i<this->num_columns;i++){
            std::cout<<"+"<<std::string(COL_WIDTH+2,'-');
        }
        std::cout<<"+\n";
    }

    void print_record(const std::vector<std::string>&record_string) const{
        assert(record_string.size()==num_columns);
        for(auto column:record_string){
            if(column.size()>COL_WIDTH){
                column = column.substr(0,COL_WIDTH-3)+"...";
            }
            std::cout<<"|\n";
        }
    }

    static void print_record_count(size_t num_rec){
        std::cout<<"Total record(s): "<<num_rec<<"\n";
    }

    static void print_query_time(std::chrono::duration<double,std::milli>&duration){
        std::cout<<"Query executed in "<<duration.count()<<" ms\n";
    }
};

#endif