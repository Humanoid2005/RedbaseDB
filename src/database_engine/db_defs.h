#ifndef DB_DEFS_H
#define DB_DEFS_H

#include <iostream>
#include <map>

template <typename T, typename = typename std::enable_if<std::is_enum<T>::value, T>::type>
static std::ostream &operator<<(std::ostream &os, const T &enum_val) {
    return os << (int)enum_val;
}

template <typename T, typename = typename std::enable_if<std::is_enum<T>::value, T>::type>
static std::istream &operator>>(std::istream &is, T &enum_val) {
    int int_val;
    is >> int_val;
    enum_val = static_cast<T>(int_val);
    return is;
}

struct RecordID {
    int page_no;
    int slot_no;

    RecordID() = default;
    RecordID(int page_no_, int slot_no_) : page_no(page_no_), slot_no(slot_no_) {}

    friend bool operator==(const RecordID &x, const RecordID &y) { return x.page_no == y.page_no && x.slot_no == y.slot_no; }
    friend bool operator!=(const RecordID &x, const RecordID &y) { return !(x == y); }
};

enum ColumnType { TYPE_INT, TYPE_FLOAT, TYPE_STRING,TYPE_DATETIME };

static inline std::string ColumnType2str(ColumnType type) {
    static std::map<ColumnType, std::string> m = {{TYPE_INT, "INT"}, {TYPE_FLOAT, "FLOAT"}, {TYPE_STRING, "STRING"},{TYPE_DATETIME,"DATETIME"}};
    return m.at(type);
}

class RecordIterator {
  public:
    virtual ~RecordIterator() = default;

    virtual void next() = 0;

    virtual bool is_end() const = 0;

    virtual RecordID get_RecordID() const = 0;
};

#endif
