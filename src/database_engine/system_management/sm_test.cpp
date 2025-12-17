#include "database_engine/system_management/sm.h"
#include <gtest/gtest.h>

TEST(sm, multi_database_support) {
    std::string db1 = "testdb1";
    std::string db2 = "testdb2";
    std::string db3 = "testdb3";
    
    if (SM_Manager::is_dir(db1)) SM_Manager::drop_db(db1);
    if (SM_Manager::is_dir(db2)) SM_Manager::drop_db(db2);
    if (SM_Manager::is_dir(db3)) SM_Manager::drop_db(db3);
    
    SM_Manager::create_db(db1);
    SM_Manager::create_db(db2);
    SM_Manager::create_db(db3);
    
    auto db_list = SM_Manager::list_databases();
    EXPECT_TRUE(std::find(db_list.begin(), db_list.end(), db1) != db_list.end());
    EXPECT_TRUE(std::find(db_list.begin(), db_list.end(), db2) != db_list.end());
    EXPECT_TRUE(std::find(db_list.begin(), db_list.end(), db3) != db_list.end());
    
    EXPECT_THROW(SM_Manager::create_db(db1), DatabaseExistsError);
    
    SM_Manager::open_db(db1);
    EXPECT_EQ(SM_Manager::get_current_db(), db1);
    
    std::vector<ColumnInfo> col_defs1 = {
        ColumnInfo("id", TYPE_INT, 4),
        ColumnInfo("name", TYPE_STRING, 50),
        ColumnInfo("price", TYPE_FLOAT, 4)
    };
    
    SM_Manager::create_table("products", col_defs1);
    EXPECT_TRUE(SM_Manager::db.is_table("products"));
    
    SM_Manager::open_db(db2);
    EXPECT_EQ(SM_Manager::get_current_db(), db2);
    
    std::vector<ColumnInfo> col_defs2 = {
        ColumnInfo("order_id", TYPE_INT, 4),
        ColumnInfo("customer", TYPE_STRING, 100)
    };
    
    SM_Manager::create_table("orders", col_defs2);
    EXPECT_TRUE(SM_Manager::db.is_table("orders"));
    EXPECT_FALSE(SM_Manager::db.is_table("products"));
    
    SM_Manager::open_db(db3);
    EXPECT_EQ(SM_Manager::get_current_db(), db3);
    EXPECT_FALSE(SM_Manager::db.is_table("products"));
    EXPECT_FALSE(SM_Manager::db.is_table("orders"));
    
    SM_Manager::close_db();
    
    SM_Manager::open_db(db1);
    EXPECT_THROW(SM_Manager::drop_db(db1), InternalError);
    SM_Manager::close_db();
    
    SM_Manager::drop_db(db1);
    SM_Manager::drop_db(db2);
    SM_Manager::drop_db(db3);
    
    db_list = SM_Manager::list_databases();
    EXPECT_TRUE(std::find(db_list.begin(), db_list.end(), db1) == db_list.end());
    EXPECT_TRUE(std::find(db_list.begin(), db_list.end(), db2) == db_list.end());
    EXPECT_TRUE(std::find(db_list.begin(), db_list.end(), db3) == db_list.end());
}

TEST(sm, basic_operations) {
    std::string db = "testdb";
    std::string tab1 = "users";
    std::string tab2 = "products";
    
    if (SM_Manager::is_dir(db)) {
        SM_Manager::drop_db(db);
    }
    
    EXPECT_THROW(SM_Manager::open_db(db), DatabaseNotFoundError);
    
    SM_Manager::create_db(db);
    EXPECT_THROW(SM_Manager::create_db(db), DatabaseExistsError);
    
    SM_Manager::open_db(db);
    
    std::vector<ColumnInfo> col_defs1 = {
        ColumnInfo("id", TYPE_INT, 4),
        ColumnInfo("name", TYPE_STRING, 50),
        ColumnInfo("age", TYPE_INT, 4)
    };
    
    SM_Manager::create_table(tab1, col_defs1);
    EXPECT_THROW(SM_Manager::create_table(tab1, col_defs1), TableExistsError);
    
    std::vector<ColumnInfo> col_defs2 = {
        ColumnInfo("product_id", TYPE_INT, 4),
        ColumnInfo("product_name", TYPE_STRING, 100),
        ColumnInfo("price", TYPE_FLOAT, 4)
    };
    
    SM_Manager::create_table(tab2, col_defs2);
    
    EXPECT_TRUE(SM_Manager::db.is_table(tab1));
    EXPECT_TRUE(SM_Manager::db.is_table(tab2));
    
    const Table_Metadata &users_meta = SM_Manager::db.get_table(tab1);
    EXPECT_EQ(users_meta.name, tab1);
    EXPECT_EQ(users_meta.cols.size(), 3);
    
    SM_Manager::drop_table(tab1);
    EXPECT_FALSE(SM_Manager::db.is_table(tab1));
    
    SM_Manager::close_db();
    SM_Manager::drop_db(db);
    EXPECT_FALSE(SM_Manager::is_dir(db));
}
