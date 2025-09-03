#include "ql_node.h"

std::vector<Column_MetaData>::const_iterator QL_Node::get_column(const std::vector<Column_MetaData>&record_columns,const TableColumn &target){
    auto pos = std::find_if(record_columns.begin(),record_columns.end(),[&](const Column_MetaData &column){
        return column.table_name==target.table_name && column.column_name==target.column_name;
    });
    if(pos==record_columns.end()){
        throw ColumnNotFoundError(target.table_name+"."+target.column_name);
    }
    return pos;
}

std::map<TableColumn,Value> QL_Node::record2dict(const std::vector<Column_MetaData>&cols,const RM_Record *record){
    std::map<TableColumn,Value>rec_dict;
    for(auto &col:cols){
        TableColumn key(col.table_name,col.column_name);
        Value val;
        u_int8_t * value_buffer = record->data + col.offset;
        if(col.type== TYPE_INT){
            val.set_int(*(int*)value_buffer);
        }
        else if(col.type==TYPE_FLOAT){
            val.set_float(*(float*)value_buffer);

        }
        else if(col.type==TYPE_STRING){
            std::string str_val ((char*)value_buffer,col.length);
            str_val.resize(strlen(str_val.c_str()));
            val.set_str(str_val);
        }
        else if(col.type==TYPE_DATETIME){
            DateTime dt;
            memcpy(&dt,(DateTime*)value_buffer,sizeof(DateTime));
            val.set_datetime(dt);
        }
        assert(rec_dict.count(key)==0);
        val.init_raw(col.length);
        rec_dict[key] = val;
    }
    return rec_dict;
}

QL_ProjectionNode::QL_ProjectionNode(std::unique_ptr<QL_Node>prev,const std::vector<TableColumn>&sel_cols){
    this->prev = move(prev);
    size_t curr_offset = 0;
    auto &prev_columns = this->prev->get_columns();
    for(auto &sel_col:sel_cols){
        auto pos = get_column(prev_columns,sel_col);
        this->selected_indices.push_back(pos-prev_columns.begin());
        auto col = *pos;
        col.offset = curr_offset;
        curr_offset += col.length;
        this->columns.push_back(col);
    }
    this->length = curr_offset;
}

std::unique_ptr<RM_Record> QL_ProjectionNode::get_record() const{
    assert(!is_end());
    auto &prev_columns = this->prev->get_columns();
    auto prev_record = this->prev->get_record();
    auto &projected_columns = this->columns;
    auto projected_record = std::make_unique<RM_Record>(this->length);
    for(size_t projected_index = 0;projected_index<projected_columns.size();projected_index++){
        size_t prev_idx = this->selected_indices[projected_index];
        auto &prev_col = prev_columns[projected_index];
        auto &projected_col = projected_columns[projected_index];
        memcpy(projected_record->data+projected_col.offset,prev_record->data + prev_col.offset,projected_col.length);
    }
    return projected_record;
}

void QL_ProjectionNode::next(){
    assert(!is_end());
    this->prev->next();
}

const std::vector<Column_MetaData>& QL_ProjectionNode::get_columns() const{
    return columns;
}

QL_TableNode::QL_TableNode(std::string table_name,std::vector<Condition>conditions){
    this->table_name = std::move(table_name);
    this->conditions = conditions;
    Table_MetaData &table = SM_Manager::db_metadata.get_table(table_name);
    this->file_handle = SM_Manager::file_handle_map.at(table_name).get();
    this->columns = table.columns;
    this->length = columns.back().offset + columns.back().length;
    static std::map<ComparatorOps,ComparatorOps> swap_operators = {
        {OP_EQ,OP_EQ},{OP_NE,OP_NE},{OP_LT,OP_GT},{OP_GT,OP_LT},{OP_LE,OP_GE},{OP_GE,OP_LE}
    };

    for(auto &condition:this->conditions){
        if(condition.lhs_col.table_name != table_name){
            //lhs is on other table, rhs must be in this table
            assert(!condition.is_rhs_val && condition.rhs_col.table_name==table_name);
            std::swap(condition.lhs_col,condition.rhs_col);
            condition.op = swap_operators.at(condition.op);
        }
    }
    this->fed_conditions = conditions;
}

void QL_TableNode::begin(){
    check_runtime_conds();

    int index_number = -1;

    Table_MetaData &table = SM_Manager::db_metadata.get_table(table_name);
    for(auto &condition:this->fed_conditions){
        if(condition.is_rhs_val && condition.op != OP_NE){
            //If rhs is a value and op is not != then check if lhs has index
            auto lhs_col = table.get_column(condition.lhs_col.column_name);
            if(lhs_col->index==true){
                //This column has index so use it
                index_number = lhs_col - table.columns.begin();
                break;
            }
        }
    }

    if(index_number==-1){
        //no index is available, scan record file
        this->scan = std::make_unique<RM_Scanner>(this->file_handle);
    }
    else{
        auto index_handle = SM_Manager::index_handle_map.at(IndexManager::get_index_name(table_name,index_number)).get();
        IndexID lower = index_handle->leaf_begin();
        IndexID upper = index_handle->leaf_end();
        auto &index_col = this->columns[index_number];

        for(auto &condition: this->fed_conditions){
            if(condition.is_rhs_val && condition.op!=OP_NE && condition.lhs_col.column_name==index_col.column_name){
                uint8_t *rhs_key = condition.rhs_val.raw_record_buffer->data;
                if(condition.op==OP_EQ){
                    lower = index_handle->lower_bound(rhs_key);
                    upper = index_handle->upper_bound(rhs_key);
                }
                else if(condition.op==OP_LT){
                    upper = index_handle->lower_bound(rhs_key);
                }
                else if(condition.op==OP_GT){
                    lower = index_handle->upper_bound(rhs_key);
                }
                else if(condition.op==OP_LE){
                    upper = index_handle->upper_bound(rhs_key);
                }
                else if(condition.op==OP_GE){
                    lower = index_handle->lower_bound(rhs_key);
                }
                else{
                    throw InternalError("Unexpected operator type");
                }
                break;
            }
        }
        this->scan = std::make_unique<IndexScanner>(index_handle,lower,upper);
    }

    while(this->scan->is_end()==false){
        this->rid = this->scan->current_record_id();
        auto record = this->file_handle->get_record(this->rid);
        if(eval_conds(this->columns,this->fed_conditions,record.get())==true){
            break;
        }
        this->scan->next();
    }
}

void QL_TableNode::next(){
    this->check_runtime_conds();
    assert(!this->is_end());
    for(this->scan->next();this->scan->is_end()==false;this->scan->next()){
        this->rid = this->scan->current_record_id();
        auto record = this->file_handle->get_record(this->rid);
        if(eval_conds(this->columns,this->fed_conditions,record.get())==true){
            break;
        }
    }
}

bool QL_TableNode::is_end() const {
    return this->scan->is_end();
}

void QL_TableNode::feed(const std::map<TableColumn,Value>&feed_dict){
    this->fed_conditions = this->conditions;
    for(auto &condition:this->conditions){
        if(condition.is_rhs_val==false && condition.rhs_col.table_name != this->table_name){
            condition.is_rhs_val = true;
            condition.rhs_val = feed_dict.at(condition.rhs_col);
        }
    }
    check_runtime_conds();
}

void QL_TableNode::check_runtime_conds(){
    for(auto &condition:this->fed_conditions){
        assert(condition.lhs_col.table_name == this->table_name);
        if(condition.is_rhs_val==false){
            assert(condition.rhs_col.table_name == this->table_name);
        }
    }
}

bool QL_TableNode::eval_cond(const std::vector<Column_MetaData>&rec_cols,const Condition &condition,const RM_Record *record){
    auto lhs_col = get_column(rec_cols,condition.lhs_col);
    uint8_t *lhs = record->data + lhs_col->offset;
    uint8_t* rhs;
    ColumnType rhs_type;
    if(condition.is_rhs_val==true){
        rhs_type = condition.rhs_val.type;
        rhs = condition.rhs_val.raw_record_buffer->data;
    }
    else{
        auto rhs_col = get_column(rec_cols,condition.rhs_col);
        rhs_type = rhs_col->type;
        rhs = record->data + rhs_col->offset;
    }
    assert(rhs_type==lhs_col->type);
    int compare = compare_keys(lhs,rhs,rhs_type,lhs_col->length);
    if(condition.op==OP_EQ){
        return compare==0;
    }
    else if(condition.op==OP_NE){
        return compare!=0;
    }
    else if(condition.op== OP_LT){
        return compare<0;
    }
    else if(condition.op==OP_GT){
        return compare>0;
    }
    else if(condition.op==OP_LE){
        return compare<=0;
    }
    else if(condition.op==OP_GE){
        return compare>=0;
    }
    else{
        throw InternalError("Unexpected operation type");
    }
}

bool QL_TableNode::eval_conds(const std::vector<Column_MetaData>&rec_cols,const std::vector<Condition>&conditions,const RM_Record *record){
    return std::all_of(conditions.begin(),conditions.end(),[&](const Condition &condition){
        return eval_cond(rec_cols,condition,record);
    });
}

QL_JoinNode::QL_JoinNode(std::unique_ptr<QL_Node> left, std::unique_ptr<QL_Node> right)
{
    this->left = std::move(left);
    this->right = std::move(right);
    this->length = this->left->len() + this->right->len();
    this->columns = this->left->get_columns();
    auto right_cols = this->right->get_columns();
    for(auto &col : right_cols){
        col.offset += this->left->len();
    }
    this->columns.insert(this->columns.end(),right_cols.begin(),right_cols.end());
}

void QL_JoinNode::begin(){
    this->left->begin();
    if(this->left->is_end()==true){
        return ;
    }
    feed_right();
    this->right->begin();
    while(this->right->is_end()==true){
        this->left->next();
        if(this->left->is_end()==true){
            break;
        }
        feed_right();
        this->right->begin();
    }
}

void QL_JoinNode::next(){
    assert(this->is_end()==false);
    this->right->next();
    while(this->right->is_end()==true){
        this->left->next();
        if(this->left->is_end()==true){
            break;
        }
        feed_right();
        this->right->begin();
    }
}

std::unique_ptr<RM_Record> QL_JoinNode::get_record() const{
    assert(!is_end());
    auto record = std::make_unique<RM_Record>(this->length);
    memcpy(record->data,this->left->get_record()->data,this->left->len());
    memcpy(record->data + this->left->len(),this->right->get_record()->data,this->right->len());
}

void QL_JoinNode::feed(const std::map<TableColumn,Value>&feed_dict){
    this->prev_feed_dict = feed_dict;
    this->left->feed(feed_dict);
}

void QL_JoinNode::feed_right(){
    auto left_dict = record2dict(this->left->get_columns(),this->left->get_record().get());
    auto feed_dict = this->prev_feed_dict;
    feed_dict.insert(left_dict.begin(),left_dict.end());
    this->right->feed(feed_dict);
}