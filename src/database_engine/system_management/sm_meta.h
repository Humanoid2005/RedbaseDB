#ifndef SM_META_H
#define SM_META_H

#include "database_engine/error.h"
#include "database_engine/db_defs.h"
#include "database_engine/system_management/sm_defs.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Column_Metadata {
    std::string tab_name;
    std::string name;
    ColumnType type;
    int len;
    int offset;
    bool index;

    Column_Metadata() = default;
    Column_Metadata(std::string tab_name_, std::string name_, ColumnType type_, int len_, int offset_, bool index_)
        : tab_name(std::move(tab_name_)), name(std::move(name_)), type(type_), len(len_), offset(offset_),
          index(index_) {}

    friend std::ostream &operator<<(std::ostream &os, const Column_Metadata &col) {
        return os << col.tab_name << ' ' << col.name << ' ' << col.type << ' ' << col.len << ' ' << col.offset << ' '
                  << col.index;
    }

    friend std::istream &operator>>(std::istream &is, Column_Metadata &col) {
        return is >> col.tab_name >> col.name >> col.type >> col.len >> col.offset >> col.index;
    }
};

struct Table_Metadata {
    std::string name;
    std::vector<Column_Metadata> cols;

    bool is_col(const std::string &col_name) const {
        auto pos = std::find_if(cols.begin(), cols.end(), [&](const Column_Metadata &col) { return col.name == col_name; });
        return pos != cols.end();
    }

    std::vector<Column_Metadata>::iterator get_col(const std::string &col_name) {
        auto pos = std::find_if(cols.begin(), cols.end(), [&](const Column_Metadata &col) { return col.name == col_name; });
        if (pos == cols.end()) {
            throw ColumnNotFoundError(col_name);
        }
        return pos;
    }

    friend std::ostream &operator<<(std::ostream &os, const Table_Metadata &tab) {
        os << tab.name << '\n' << tab.cols.size() << '\n';
        for (auto &col : tab.cols) {
            os << col << '\n';
        }
        return os;
    }

    friend std::istream &operator>>(std::istream &is, Table_Metadata &tab) {
        size_t n;
        is >> tab.name >> n;
        for (size_t i = 0; i < n; i++) {
            Column_Metadata col;
            is >> col;
            tab.cols.push_back(col);
        }
        return is;
    }
};

struct DB_Metadata {
    std::string name;
    std::map<std::string, Table_Metadata> tabs;

    bool is_table(const std::string &tab_name) const { return tabs.find(tab_name) != tabs.end(); }

    Table_Metadata &get_table(const std::string &tab_name) {
        auto pos = tabs.find(tab_name);
        if (pos == tabs.end()) {
            throw TableNotFoundError(tab_name);
        }
        return pos->second;
    }

    friend std::ostream &operator<<(std::ostream &os, const DB_Metadata &db_meta) {
        os << db_meta.name << '\n' << db_meta.tabs.size() << '\n';
        for (auto &entry : db_meta.tabs) {
            os << entry.second << '\n';
        }
        return os;
    }

    friend std::istream &operator>>(std::istream &is, DB_Metadata &db_meta) {
        size_t n;
        is >> db_meta.name >> n;
        for (size_t i = 0; i < n; i++) {
            Table_Metadata tab;
            is >> tab;
            db_meta.tabs[tab.name] = tab;
        }
        return is;
    }
};

#endif
