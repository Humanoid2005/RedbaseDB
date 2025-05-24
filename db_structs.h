#ifndef DB_STRUCTS_H
#define DB_STRUCTS_H

#include <iostream>

/*
Output: ENUM --> Integer
Serializing ENUM for logging onto necessary interface
*/
template <typename T>
typename std::enable_if<std::is_enum<T>::value, std::ostream&>::type
operator<<(std::ostream &out, T val) {
    return out << static_cast<int>(val);
}

/*
Input: Integer --> ENUM
Deserializing int to ENUM to be usable in program
*/
template <typename T>
typename std::enable_if<std::is_enum<T>::value, std::istream&>::type
operator>>(std::istream &in, T &val) {
    int temp;
    in >> temp;
    val = static_cast<T>(temp);
    return in;
}

class RecordID{
public:
    int page_number;
    int slot_number;

    RecordID(int page_number=0,int slot_number=0){
        this->page_number = page_number;
        this->slot_number = slot_number;
    }

    bool operator==(const RecordID &other) const{
        return this->page_number==other.page_number && this->slot_number==other.slot_number;
    }

    bool operator!=(const RecordID &other) const{
        return this->page_number!=other.page_number || this->slot_number!=other.slot_number;

    }
};

/*
Column Types : Currently the types supported in db are int,float,string and datetime
Function to deserialise the ENUMS
*/
typedef enum {
TYPE_INT,
TYPE_FLOAT,
TYPE_STRING,
TYPE_DATETIME
} ColumnType;

std::string convertColTypeToString(ColumnType c){
    switch(c){
        case TYPE_INT:
            return "int";
        case TYPE_FLOAT:
            return "float";
        case TYPE_STRING:
            return "string";
        case TYPE_DATETIME:
            return "datetime";
        default:
            return "none";
    }

    return "none";
}

/*
A virtual class which will be implemented by the scanning algorithms used in index_helper.
*/
class RecordScanner{
public:
    virtual ~RecordScanner() = default;
    virtual void next() = 0;
    virtual bool is_end() const = 0;
    virtual RecordID current_record_id() const = 0;
};

#endif