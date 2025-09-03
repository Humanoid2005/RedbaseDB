#ifndef QL_NODE_H
#define QL_NODE_H

#include "ql_manager.h"
#include "system_management/sm.h"


/*
Abstract virtual base class for query node ops: sigma (selection), pi(projection), join, tables
*/
class QL_Node{
public:
    virtual ~QL_Node() = default;

    virtual size_t len() const = 0;

    virtual const std::vector<Column_MetaData> &get_columns() const  = 0;

    virtual void begin() = 0;
    virtual void next() = 0;
    virtual bool is_end() const  = 0;

    virtual std::unique_ptr<RM_Record> get_record() const = 0;
    virtual void feed(const std::map<TableColumn,Value>&feed_dict) = 0;

    static std::vector<Column_MetaData>::const_iterator get_column(const std::vector<Column_MetaData>&record_columns,const TableColumn &target);
    static std::map<TableColumn,Value> record2dict(const std::vector<Column_MetaData>&columns,const RM_Record *record);
};

class QL_ProjectionNode: public QL_Node{
private:
    std::unique_ptr<QL_Node> prev;
    std::vector<Column_MetaData> columns;
    size_t length;
    std::vector<size_t> selected_indices;
public:
    QL_ProjectionNode(std::unique_ptr<QL_Node>prev,const std::vector<TableColumn>&select_columns);
    void next() override;
    const std::vector<Column_MetaData> &get_columns() const override;

    size_t len() const override{
        return this->length;
    }

    void begin() override{
        return this->prev->begin();
    }

    bool is_end() const override { 
        return this->prev->is_end(); 
    }

    std::unique_ptr<RM_Record> get_record() const override;

    void feed(const std::map<TableColumn, Value> &feed_dict) override {
        throw InternalError("Cannot feed a projection node");
    }
};

class QL_TableNode: public QL_Node{
private:
    std::string table_name;
    std::vector<Condition> conditions;
    RM_FileHandle *file_handle;
    std::vector<Column_MetaData> columns;
    size_t length;
    std::vector<Condition> fed_conditions;
    RecordID rid;
    std::unique_ptr<RecordScanner> scan;

public:
    QL_TableNode(std::string tab_name, std::vector<Condition> conds);

    void begin() override;
    void next() override;
    bool is_end() const override;

    size_t len() const override { 
        return this->length; 
    }

    const std::vector<Column_MetaData> &get_columns() const override { 
        return this->columns; 
    }

    std::unique_ptr<RM_Record> get_record() const override {
        assert(!is_end());
        return file_handle->get_record(rid);
    }

    void feed(const std::map<TableColumn, Value> &feed_dict) override;

    const RecordID &get_rid() const { 
        return this->rid; 
    }

    void check_runtime_conds();

    static bool eval_cond(const std::vector<Column_MetaData> &rec_cols, const Condition &cond, const RM_Record *rec);

    static bool eval_conds(const std::vector<Column_MetaData> &rec_cols, const std::vector<Condition> &conds,const RM_Record *rec);
};

class QL_JoinNode : public QL_Node{
private:
    std::unique_ptr<QL_Node> left;
    std::unique_ptr<QL_Node> right;
    size_t length;
    std::vector<Column_MetaData> columns;
    std::map<TableColumn, Value> prev_feed_dict;

public:
    QL_JoinNode(std::unique_ptr<QL_Node> left, std::unique_ptr<QL_Node> right);

    size_t len() const override { 
        return this->length; 
    }

    const std::vector<Column_MetaData> &get_columns() const override { 
        return this->columns; 
    }

    void begin() override;

    void next() override;

    bool is_end() const override { 
        return this->left->is_end(); 
    }

    std::unique_ptr<RM_Record> get_record() const override;

    void feed(const std::map<TableColumn, Value> &feed_dict) override;

    void feed_right();
};

#endif