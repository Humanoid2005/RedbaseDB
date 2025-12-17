#include "ql_manager.h"
#include "database_engine/index_handler/ndx.h"
#include "database_engine/query_language/ql_node.h"
#include "database_engine/system_management/sm.h"

static TabCol check_column(const std::vector<Column_Metadata> &all_cols, TabCol target) {
    if (target.tab_name.empty()) {
        // Table name not specified, infer table name from column name
        std::string tab_name;
        for (auto &col : all_cols) {
            if (col.name == target.col_name) {
                if (!tab_name.empty()) {
                    throw AmbiguousColumnError(target.col_name);
                }
                tab_name = col.tab_name;
            }
        }
        if (tab_name.empty()) {
            throw ColumnNotFoundError(target.col_name);
        }
        target.tab_name = tab_name;
    } else {
        // Make sure target column exists
        if (!(SM_Manager::db.is_table(target.tab_name) &&
              SM_Manager::db.get_table(target.tab_name).is_col(target.col_name))) {
            throw ColumnNotFoundError(target.tab_name + '.' + target.col_name);
        }
    }
    return target;
}

static std::vector<Column_Metadata> get_all_cols(const std::vector<std::string> &tab_names) {
    std::vector<Column_Metadata> all_cols;
    for (auto &sel_tab_name : tab_names) {
        const auto &sel_tab_cols = SM_Manager::db.get_table(sel_tab_name).cols;
        all_cols.insert(all_cols.end(), sel_tab_cols.begin(), sel_tab_cols.end());
    }
    return all_cols;
}

static std::vector<Condition> check_where_clause(const std::vector<std::string> &tab_names,
                                                 const std::vector<Condition> &conds) {
    auto all_cols = get_all_cols(tab_names);
    // Get raw values in where clause
    std::vector<Condition> res_conds = conds;
    for (auto &cond : res_conds) {
        // Infer table name from column name
        cond.lhs_col = check_column(all_cols, cond.lhs_col);
        if (!cond.is_rhs_val) {
            cond.rhs_col = check_column(all_cols, cond.rhs_col);
        }
        Table_Metadata &lhs_tab = SM_Manager::db.get_table(cond.lhs_col.tab_name);
        auto lhs_col = lhs_tab.get_col(cond.lhs_col.col_name);
        ColumnType lhs_type = lhs_col->type;
        ColumnType rhs_type;
        if (cond.is_rhs_val) {
            cond.rhs_val.init_raw(lhs_col->len);
            rhs_type = cond.rhs_val.type;
        } else {
            Table_Metadata &rhs_tab = SM_Manager::db.get_table(cond.rhs_col.tab_name);
            auto rhs_col = rhs_tab.get_col(cond.rhs_col.col_name);
            rhs_type = rhs_col->type;
        }
        if (lhs_type != rhs_type) {
            throw IncompatibleTypeError(ColumnType2str(lhs_type), ColumnType2str(rhs_type));
        }
    }
    return res_conds;
}

InsertResult QL_Manager::insert_into(const std::string &tab_name, std::vector<Value> values) {
    InsertResult result;
    result.table_name = tab_name;
    
    Table_Metadata tab = SM_Manager::db.get_table(tab_name);
    if (values.size() != tab.cols.size()) {
        throw InvalidValueCountError();
    }
    // Get record file handle
    auto fh = SM_Manager::fhs.at(tab_name).get();
    // Make record buffer
    RM_Record rec(fh->hdr.record_size);
    for (size_t i = 0; i < values.size(); i++) {
        auto &col = tab.cols[i];
        auto &val = values[i];
        if (col.type != val.type) {
            throw IncompatibleTypeError(ColumnType2str(col.type), ColumnType2str(val.type));
        }
        val.init_raw(col.len);
        memcpy(rec.data + col.offset, val.raw->data, col.len);
    }
    // Insert into record file
    RecordID rid = fh->insert_record(rec.data);
    // Insert into index
    for (size_t i = 0; i < tab.cols.size(); i++) {
        auto &col = tab.cols[i];
        if (col.index) {
            auto ih = SM_Manager::ihs.at(IndexManager::get_index_name(tab_name, i)).get();
            ih->insert_entry(rec.data + col.offset, rid);
        }
    }
    
    result.success = true;
    result.message = "1 row inserted";
    
    return result;
}

DeleteResult QL_Manager::delete_from(const std::string &tab_name, std::vector<Condition> conds) {
    DeleteResult result;
    result.table_name = tab_name;
    result.affected_rows = 0;
    
    Table_Metadata &tab = SM_Manager::db.get_table(tab_name);
    conds = check_where_clause({tab_name}, conds);
    // Get record file handle
    auto fh = SM_Manager::fhs.at(tab_name).get();
    // Get all RID to delete
    std::vector<RecordID> rids;
    QlNodeTable table_scan(tab_name, conds);
    for (table_scan.begin(); !table_scan.is_end(); table_scan.next()) {
        rids.push_back(table_scan.rid());
    }
    // Delete from index
    for (auto &rid : rids) {
        auto rec = fh->get_record(rid);
        for (size_t i = 0; i < tab.cols.size(); i++) {
            auto &col = tab.cols[i];
            if (col.index) {
                auto ih = SM_Manager::ihs.at(IndexManager::get_index_name(tab_name, i)).get();
                ih->delete_entry(rec->data + col.offset, rid);
            }
        }
    }
    // Delete from record file
    for (auto &rid : rids) {
        fh->delete_record(rid);
        result.affected_rows++;
    }
    
    result.success = true;
    result.message = std::to_string(result.affected_rows) + " row(s) deleted";
    
    return result;
}

UpdateResult QL_Manager::update_set(const std::string &tab_name, std::vector<SetClause> set_clauses,
                           std::vector<Condition> conds) {
    UpdateResult result;
    result.table_name = tab_name;
    result.affected_rows = 0;
    
    Table_Metadata &tab = SM_Manager::db.get_table(tab_name);
    // Parse where clause
    conds = check_where_clause({tab_name}, conds);
    // Get raw values in set clause
    for (auto &set_clause : set_clauses) {
        auto lhs_col = tab.get_col(set_clause.lhs.col_name);
        if (lhs_col->type != set_clause.rhs.type) {
            throw IncompatibleTypeError(ColumnType2str(lhs_col->type), ColumnType2str(set_clause.rhs.type));
        }
        set_clause.rhs.init_raw(lhs_col->len);
    }
    // Get all RID to update
    std::vector<RecordID> rids;
    QlNodeTable table_scan(tab_name, conds);
    for (table_scan.begin(); !table_scan.is_end(); table_scan.next()) {
        rids.push_back(table_scan.rid());
    }
    // Get record file
    auto fh = SM_Manager::fhs.at(tab_name).get();
    // Get all necessary index files
    std::vector<IndexHandle *> ihs(tab.cols.size(), nullptr);
    for (auto &set_clause : set_clauses) {
        auto lhs_col = tab.get_col(set_clause.lhs.col_name);
        if (lhs_col->index) {
            size_t lhs_col_idx = lhs_col - tab.cols.begin();
            if (ihs[lhs_col_idx] == nullptr) {
                ihs[lhs_col_idx] = SM_Manager::ihs.at(IndexManager::get_index_name(tab_name, lhs_col_idx)).get();
            }
        }
    }
    // Update each rid from record file and index file
    for (auto &rid : rids) {
        auto rec = fh->get_record(rid);
        // Remove old entry from index
        for (size_t i = 0; i < tab.cols.size(); i++) {
            if (ihs[i] != nullptr) {
                ihs[i]->delete_entry(rec->data + tab.cols[i].offset, rid);
            }
        }
        // Update record in record file
        for (auto &set_clause : set_clauses) {
            auto lhs_col = tab.get_col(set_clause.lhs.col_name);
            memcpy(rec->data + lhs_col->offset, set_clause.rhs.raw->data, lhs_col->len);
        }
        fh->update_record(rid, rec->data);
        // Insert new entry into index
        for (size_t i = 0; i < tab.cols.size(); i++) {
            if (ihs[i] != nullptr) {
                ihs[i]->insert_entry(rec->data + tab.cols[i].offset, rid);
            }
        }
        result.affected_rows++;
    }
    
    result.success = true;
    result.message = std::to_string(result.affected_rows) + " row(s) updated";
    
    return result;
}

static std::vector<Condition> pop_conds(std::vector<Condition> &conds, const std::vector<std::string> &tab_names) {
    auto has_tab = [&](const std::string &tab_name) {
        return std::find(tab_names.begin(), tab_names.end(), tab_name) != tab_names.end();
    };
    std::vector<Condition> solved_conds;
    auto it = conds.begin();
    while (it != conds.end()) {
        if (has_tab(it->lhs_col.tab_name) && (it->is_rhs_val || has_tab(it->rhs_col.tab_name))) {
            solved_conds.emplace_back(std::move(*it));
            it = conds.erase(it);
        } else {
            it++;
        }
    }
    return solved_conds;
}

SelectResult QL_Manager::select_from(std::vector<TabCol> sel_cols, const std::vector<std::string> &tab_names,
                            std::vector<Condition> conds) {
    SelectResult result;
    
    // Parse selector
    auto all_cols = get_all_cols(tab_names);
    if (sel_cols.empty()) {
        // select all columns
        for (auto &col : all_cols) {
            TabCol sel_col(col.tab_name, col.name);
            sel_cols.push_back(sel_col);
        }
    } else {
        // infer table name from column name
        for (auto &sel_col : sel_cols) {
            sel_col = check_column(all_cols, sel_col);
        }
    }
    // Parse where clause
    conds = check_where_clause(tab_names, conds);
    // Scan table
    std::vector<std::unique_ptr<QlNodeTable>> tab_nodes(tab_names.size());
    for (size_t i = 0; i < tab_names.size(); i++) {
        auto curr_conds = pop_conds(conds, {tab_names.begin(), tab_names.begin() + i + 1});
        tab_nodes[i] = std::make_unique<QlNodeTable>(tab_names[i], curr_conds);
    }
    assert(conds.empty());
    std::unique_ptr<QlNode> query_plan = std::move(tab_nodes.back());
    for (size_t i = tab_names.size() - 2; i != (size_t)-1; i--) {
        query_plan = std::make_unique<QlNodeJoin>(std::move(tab_nodes[i]), std::move(query_plan));
    }
    query_plan = std::make_unique<QlNodeProj>(std::move(query_plan), sel_cols);
    
    // Add column names to result
    for (auto &sel_col : sel_cols) {
        result.add_column(sel_col.col_name);
    }
    
    // Process records and add to result
    for (query_plan->begin(); !query_plan->is_end(); query_plan->next()) {
            auto rec = query_plan->rec();
            SelectRow row;
            for (auto &col : query_plan->cols()) {
                std::string col_str;
                uint8_t *rec_buf = rec->data + col.offset;
                if (col.type == TYPE_INT) {
                    col_str = std::to_string(*(int *)rec_buf);
                } else if (col.type == TYPE_FLOAT) {
                    col_str = std::to_string(*(float *)rec_buf);
                } else if (col.type == TYPE_STRING) {
                    col_str = std::string((char *)rec_buf, col.len);
                    col_str.resize(strlen(col_str.c_str()));
                } else if (col.type == TYPE_DATETIME) {
                    DateTime datetime_val;
                    memcpy(&datetime_val, rec_buf, sizeof(DateTime));
                    col_str = DateTime::DTtoString(datetime_val);
                }
                row.add_value(col_str);
            }
            result.add_row(row);
        }
    
    return result;
}
