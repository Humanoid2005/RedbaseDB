#ifndef QL_NODE_H
#define QL_NODE_H

#include "database_engine/query_language/ql_manager.h"
#include "database_engine/system_management/sm.h"
#include "database_engine/index_handler/ndx_iterator.h"

class QlNode {
  public:
    virtual ~QlNode() = default;

    virtual size_t len() const = 0;
    virtual const std::vector<Column_Metadata> &cols() const = 0;

    virtual void begin() = 0;
    virtual void next() = 0;
    virtual bool is_end() const = 0;

    virtual std::unique_ptr<RM_Record> rec() const = 0;
    virtual void feed(const std::map<TabCol, Value> &feed_dict) = 0;

    static std::vector<Column_Metadata>::const_iterator get_col(const std::vector<Column_Metadata> &rec_cols, const TabCol &target);

    static std::map<TabCol, Value> rec2dict(const std::vector<Column_Metadata> &cols, const RM_Record *rec);
};

class QlNodeProj : public QlNode {
  public:
    QlNodeProj(std::unique_ptr<QlNode> prev, const std::vector<TabCol> &sel_cols);

    size_t len() const override { return _len; }
    const std::vector<Column_Metadata> &cols() const override { return _cols; }

    void begin() override { _prev->begin(); }
    void next() override {
        assert(!_prev->is_end());
        _prev->next();
    }
    bool is_end() const override { return _prev->is_end(); }

    std::unique_ptr<RM_Record> rec() const override;

    void feed(const std::map<TabCol, Value> &feed_dict) override {
        throw InternalError("Cannot feed a projection node");
    }

  private:
    std::unique_ptr<QlNode> _prev;
    std::vector<Column_Metadata> _cols;
    size_t _len;
    std::vector<size_t> _sel_idxs;
};

class QlNodeTable : public QlNode {
  public:
    QlNodeTable(std::string tab_name, std::vector<Condition> conds);

    void begin() override;
    void next() override;
    bool is_end() const override;

    size_t len() const override { return _len; }
    const std::vector<Column_Metadata> &cols() const override { return _cols; }

    std::unique_ptr<RM_Record> rec() const override {
        assert(!is_end());
        return _fh->get_record(_rid);
    }

    void feed(const std::map<TabCol, Value> &feed_dict) override;

    const RecordID &rid() const { return _rid; }

    void check_runtime_conds();

    static bool eval_cond(const std::vector<Column_Metadata> &rec_cols, const Condition &cond, const RM_Record *rec);

    static bool eval_conds(const std::vector<Column_Metadata> &rec_cols, const std::vector<Condition> &conds,
                           const RM_Record *rec);

  private:
    std::string _tab_name;
    std::vector<Condition> _conds;
    RM_FileHandle *_fh;
    std::vector<Column_Metadata> _cols;
    size_t _len;
    std::vector<Condition> _fed_conds;

    RecordID _rid;
    std::unique_ptr<RecordIterator> _scan;
};

class QlNodeJoin : public QlNode {
  public:
    QlNodeJoin(std::unique_ptr<QlNode> left, std::unique_ptr<QlNode> right);

    size_t len() const override { return _len; }

    const std::vector<Column_Metadata> &cols() const override { return _cols; }

    void begin() override;

    void next() override;

    bool is_end() const override { return _left->is_end(); }

    std::unique_ptr<RM_Record> rec() const override;

    void feed(const std::map<TabCol, Value> &feed_dict) override;

    void feed_right();

  private:
    std::unique_ptr<QlNode> _left;
    std::unique_ptr<QlNode> _right;
    size_t _len;
    std::vector<Column_Metadata> _cols;

    std::map<TabCol, Value> _prev_feed_dict;
};

#endif