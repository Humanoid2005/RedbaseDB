#include "ql_manager.h"
#include "indexing_handler/ndx.h"
#include "system_management/sm.h"
#include "../record_logger.h"
#include "ql_node.h"

TableColumn check_column(const std::vector<Column_MetaData>&columns,TableColumn target){
    if(target.table_name.empty()==true){
        //Table name is not specified we will infer the table name from column name
        std::string table_name;
        for(auto &col:columns){
            if(col.column_name==target.column_name){
                if(table_name.empty()==true){
                    throw AmbiguousColumnError(target.column_name);
                }
                table_name = col.table_name;
            }
        }
        if(table_name.empty()==true){
            throw ColumnNotFoundError(target.column_name);
        }
        target.table_name = table_name;
    }
    else{
        //Check if target column exists
        if(SM_Manager::db_metadata.is_table(target.table_name)==false && SM_Manager::db_metadata.get_table(target.table_name).is_column(target.column_name)){
            throw ColumnNotFoundError(target.table_name+"."+target.column_name);
        }
    }
    return target;
}

static std::vector<Column_MetaData> get_all_columns(const std::vector<std::string>&table_names){
    std::vector<Column_MetaData>all_columns;
    for(auto &select_table_name:table_names){
        const auto &select_table_columns = SM_Manager::db_metadata.get_table(select_table_name).columns;
        all_columns.insert(all_columns.end(),select_table_columns.begin(),select_table_columns.end());
    }
    return all_columns;
}

static std::vector<Condition> check_where_clauses(const std::vector<std::string>&table_names,const std::vector<Condition>&conditions){
    auto all_columns = get_all_columns(table_names);
    std::vector<Condition> res_conditions = conditions;
    for(auto &condition:res_conditions){
        condition.lhs_col = check_column(all_columns,condition.lhs_col);
        if(condition.is_rhs_val==false){
            condition.rhs_col = check_column(all_columns,condition.rhs_col);
        }
        Table_MetaData lhs_table = SM_Manager::db_metadata.get_table(condition.lhs_col.table_name);
        auto lhs_col = lhs_table.get_column(condition.lhs_col.column_name);
        ColumnType lhs_type = lhs_col->type;
        ColumnType rhs_type;
        if(condition.is_rhs_val==true){
            condition.rhs_val.init_raw(lhs_col->length);
            rhs_type = condition.rhs_val.type;
        }
        else{
            Table_MetaData &rhs_table = SM_Manager::db_metadata.get_table(condition.rhs_col.table_name);
            auto rhs_col = rhs_table.get_column(condition.rhs_col.column_name);
            rhs_type = rhs_col->type;
        }
        if(lhs_type!=rhs_type){
            throw IncompatibleTypeError(convertColTypeToString(lhs_type),convertColTypeToString(rhs_type));
        }
    }
    return res_conditions;
}

void QL_Manager::insert_into(const std::string &table_name,std::vector<Value> values){
    Table_MetaData table = SM_Manager::db_metadata.get_table(table_name);
    if(values.size()!=table.columns.size()){
        throw InvalidValueCountError();
    }

    auto file_handle = SM_Manager::file_handle_map.at(table_name).get();
    RM_Record record(file_handle->file_header.record_size);

    for(size_t i=0;i<values.size();i++){
        auto &column = table.columns[i];
        auto &value = values[i];
        if(column.type!=value.type){
            throw IncompatibleTypeError(convertColTypeToString(column.type),convertColTypeToString(value.type));
        }
        value.init_raw(column.length);
        memcpy(record.data+column.offset,value.raw_record_buffer->data,column.length);
    }
    //Insert into record file
    RecordID rid = file_handle->insert_record(record.data);
    //Insert into index
    for(size_t i=0;i<table.columns.size();i++){
        auto &column = table.columns[i];
        if(column.index==true){
            auto index_handle = SM_Manager::index_handle_map.at(IndexManager::get_index_name(table_name,i)).get();
            index_handle->insert_entry(record.data+column.offset,rid);
        }
    }
}

void QL_Manager::delete_from(const std::string &table_name,std::vector<Condition>conditions){
    Table_MetaData table = SM_Manager::db_metadata.get_table(table_name);
    conditions = check_where_clauses({table_name},conditions);//parse where clauses
    std::vector<RecordID>rids;//record ids to delete
    QL_TableNode table_scan(table_name,conditions);
    for(table_scan.begin();table_scan.is_end()==false;table_scan.next()){
        rids.push_back(table_scan.get_rid());
    }
    //get the record file
    auto file_handle = SM_Manager::file_handle_map.at(table_name).get();
    //get all index files
    std::vector<IndexHandle*> index_handle_scan(table.columns.size(),nullptr);
    for(size_t col_i=0;col_i<table.columns.size();col_i++){
        if(table.columns[col_i].index==true){
            index_handle_scan[col_i] = SM_Manager::index_handle_map.at(IndexManager::get_index_name(table_name,col_i)).get();
        }
    }

    //delete each record id from record and index file
    for(auto &rid:rids){
        auto rec = file_handle->get_record(rid);
        for(size_t col_i=0;col_i<table.columns.size();col_i++){
            if(index_handle_scan[col_i]!=nullptr){
                //delete from index file
                index_handle_scan[col_i]->delete_entry(rec->data+table.columns[col_i].offset,rid);
            }
        }
        //delete from record file
        file_handle->delete_record(rid);
    }

}

void QL_Manager::update_set(const std::string &table_name, std::vector<SetClause> set_clauses, std::vector<Condition> conditions)
{
    Table_MetaData &table = SM_Manager::db_metadata.get_table(table_name);
    conditions = check_where_clauses({table_name},conditions);//Parsing the where clause

    for(auto &set_clause:set_clauses){
        auto lhs_col = table.get_column(set_clause.lhs.column_name);
        if(lhs_col->type!=set_clause.rhs.type){
            throw IncompatibleTypeError(convertColTypeToString(lhs_col->type),convertColTypeToString(set_clause.rhs.type));
        }
        set_clause.rhs.init_raw(lhs_col->length);
    }

    //Get all the ids of records to be updated
    std::vector<RecordID>rids;
    QL_TableNode table_scan(table_name,conditions);
    for(table_scan.begin();table_scan.is_end()==false;table_scan.next()){
        rids.push_back(table_scan.get_rid());
    }

    //get the record file
    auto file_handle = SM_Manager::file_handle_map.at(table_name).get();
    std::vector<IndexHandle*>index_handle_scan(table.columns.size(),nullptr);//get necessary index files
    for(auto &set_clause:set_clauses){
        auto lhs_col = table.get_column(set_clause.lhs.column_name);
        if(lhs_col->index==true){
            size_t lhs_col_idx = lhs_col - table.columns.begin();
            if(index_handle_scan[lhs_col_idx]==nullptr){
                index_handle_scan[lhs_col_idx] = SM_Manager::index_handle_map.at(IndexManager::get_index_name(table_name,lhs_col_idx)).get();
            }
        }
    }

    //update each record from record file and index file
    for(auto &rid:rids){
        auto rec = file_handle->get_record(rid);
        //remove old entry
        for(size_t i=0;i<table.columns.size();i++){
            if(index_handle_scan[i]!=nullptr){
                index_handle_scan[i]->delete_entry(rec->data+table.columns[i].offset,rid);
            }
        }

        //update record in record file
        for(auto &set_clause:set_clauses){
            auto lhs_col = table.get_column(set_clause.lhs.column_name);
            memcpy(rec->data+lhs_col->offset,set_clause.rhs.raw_record_buffer->data,lhs_col->length);
        }
        file_handle->update_record(rid,rec->data);

        //insert new entry into index
        for(size_t i=0;i<table.columns.size();i++){
            if(index_handle_scan[i]!=nullptr){
                index_handle_scan[i]->insert_entry(rec->data+table.columns[i].offset,rid);
            }
        }
    }
}

//Returns a vector of resolved conditions that can now be evaluated using the current tab_names.
static std::vector<Condition> pop_conds(std::vector<Condition> &conds, const std::vector<std::string> &tab_names) {
    auto has_tab = [&](const std::string &tab_name) {
        return std::find(tab_names.begin(), tab_names.end(), tab_name) != tab_names.end();
    };
    std::vector<Condition> solved_conds;
    auto it = conds.begin();
    while (it != conds.end()) {
        if (has_tab(it->lhs_col.table_name) && (it->is_rhs_val || has_tab(it->rhs_col.table_name))) {
            solved_conds.emplace_back(std::move(*it));
            it = conds.erase(it);
        } else {
            it++;
        }
    }
    return solved_conds;
}

void QL_Manager::select_from(std::vector<TableColumn> selected_columns, const std::vector<std::string> &table_names, std::vector<Condition> conditions)
{
    //parse the selector i.e. from
    auto all_columns = get_all_columns(table_names);
    if(selected_columns.empty()){
        //select all columns
        for(auto &col:all_columns){
            TableColumn selected_column(col.table_name,col.column_name);
            selected_columns.push_back(selected_column);
        }
    }
    else{
        for(auto &selected_column:selected_columns){
            selected_column = check_column(all_columns,selected_column);
        }
    }

    //parse where clauses
    conditions = check_where_clauses(table_names,conditions);
    //scan the table
    std::vector<std::unique_ptr<QL_TableNode>>table_nodes(table_names.size());
    for(size_t i=0;i<table_names.size();i++){
        auto current_conditions = pop_conds(conditions,{table_names.begin(),table_names.begin()+i+1});
        table_nodes[i] = std::make_unique<QL_TableNode>(table_names[i],current_conditions);
    }
    assert(conditions.empty());
    std::unique_ptr<QL_Node> query_plan = std::move(table_nodes.back());
    for(size_t i=table_names.size()-2;i!=(size_t)(-1);i--){
        query_plan = std::make_unique<QL_JoinNode>(std::move(table_nodes[i]),std::move(query_plan));
    }
    query_plan = std::make_unique<QL_ProjectionNode>(std::move(query_plan),selected_columns);
    //Column titles
    std::vector<std::string>captions;
    captions.reserve(selected_columns.size());

    for(auto &selected_column:selected_columns){
        captions.push_back(selected_column.column_name);
    }

    //LOGGER
    RecordLogger logger(selected_columns.size());
    logger.print_separator();
    logger.print_record(captions);
    logger.print_separator();

    size_t num_records = 0;

    for(query_plan->begin();query_plan->is_end()==false;query_plan->next()){
        auto record = query_plan->get_record();
        std::vector<std::string>columns;
        for(auto &col:query_plan->get_columns()){
            std::string col_str;
            uint8_t * record_buffer = record->data + col.offset;
            if(col.type==TYPE_INT){
                int integer = *((int*)record_buffer);
                col_str = std::to_string(integer);
            }
            else if(col.type==TYPE_FLOAT){
                float floating_number = *((float*)record_buffer);
                col_str = std::to_string(floating_number);
            }
            else if(col.type==TYPE_STRING){
                col_str = std::string((char*)record_buffer,col.length);
                col_str.resize(strlen(col_str.c_str()));
                std::string varchar = col_str;
            }
            else if(col.type==TYPE_DATETIME){
                DateTime dt = *((DateTime*)record_buffer);
                col_str = DateTime::DTtoString(dt);
            }
            columns.push_back(col_str);
        }
        logger.print_record(columns);
        num_records++;
    }
    logger.print_separator();
    RecordLogger::print_record_count(num_records);
}
