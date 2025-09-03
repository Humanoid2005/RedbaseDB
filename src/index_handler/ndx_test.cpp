#include "ndx.h"
#include <gtest/gtest.h>
#include <iostream>
#include <map>
#include <cstdlib>
#include <ctime>

class IndexTest : public ::testing::Test {
public:
    void check_tree(IndexHandle *ih, int root_page) {
        IndexNodeHandle node = ih->get_node(root_page);
        if (node.header->is_leaf) return;

        for (int i = 0; i < node.header->num_child; i++) {
            IndexNodeHandle child = ih->get_node(node.get_record_id(i)->page_no);
            EXPECT_EQ(child.header->parent, root_page);
            EXPECT_EQ(memcmp(node.get_key(i), child.get_key(child.header->num_key - 1), ih->index_header.col_len), 0);
            check_tree(ih, node.get_record_id(i)->page_no);
        }
    }

    void check_leaf(const IndexHandle *ih) {
        int leaf_no = ih->index_header.first_leaf;
        while (leaf_no != NDX_LEAF_HEADER_PAGE) {
            IndexNodeHandle curr = ih->get_node(leaf_no);
            IndexNodeHandle prev = ih->get_node(curr.header->prev_leaf);
            IndexNodeHandle next = ih->get_node(curr.header->next_leaf);
            EXPECT_EQ(prev.header->next_leaf, leaf_no);
            EXPECT_EQ(next.header->prev_leaf, leaf_no);
            leaf_no = curr.header->next_leaf;
        }
    }

    void check_equal(IndexHandle *ih, const std::multimap<int, RecordID> &mock) {
        check_tree(ih, ih->index_header.root_page);
        check_leaf(ih);

        for (auto &entry : mock) {
            int mock_key = entry.first;
            {
                auto mock_lower = mock.lower_bound(mock_key);
                IndexID iid = ih->lower_bound((const uint8_t *)&mock_key);
                RecordID rid = ih->get_record_id(iid);
                EXPECT_EQ(rid, mock_lower->second);
            }
            {
                auto mock_upper = mock.upper_bound(mock_key);
                IndexID iid = ih->upper_bound((const uint8_t *)&mock_key);
                if (mock_upper == mock.end()) {
                    EXPECT_EQ(iid, ih->leaf_end());
                } else {
                    RecordID rid = ih->get_record_id(iid);
                    EXPECT_EQ(rid, mock_upper->second);
                }
            }
        }

        IndexIterator scan(ih, ih->leaf_begin(), ih->leaf_end());
        auto it = mock.begin();
        while (!scan.is_end() && it != mock.end()) {
            RecordID mock_rid = it->second;
            RecordID rid = scan.get_RecordID();
            EXPECT_EQ(rid, mock_rid);
            it++;
            scan.next();
        }
        EXPECT_TRUE(scan.is_end());
        EXPECT_EQ(it, mock.end());
    }

    void print_btree(IndexHandle &ih, int root_page, int offset) {
        IndexNodeHandle node = ih.get_node(root_page);
        for (int i = node.header->num_child - 1; i >= 0; i--) {
            std::cout << std::string(offset, ' ') << *(int *)node.get_key(i) << std::endl;
            if (!node.header->is_leaf) {
                print_btree(ih, node.get_record_id(i)->page_no, offset + 4);
            }
        }
    }

    void test_ndx_insert_delete(int order, int round) {
        std::string filename = "abc";
        int index_no = 0;

        if (IndexManager::exists(filename, index_no)) {
            IndexManager::destroy_index(filename, index_no);
        }

        IndexManager::create_index(filename, index_no, TYPE_INT, sizeof(int));
        auto ih = IndexManager::open_index(filename, index_no);

        if (order > 2 && order <= ih->index_header.btree_order) {
            ih->index_header.btree_order = order;
        }

        std::multimap<int, RecordID> mock;
        for (int i = 0; i < round; i++) {
            int rand_key = rand() % round;
            RecordID rand_val(rand(), rand());
            ih->insert_entry((const uint8_t *)&rand_key, rand_val);
            mock.insert(std::make_pair(rand_key, rand_val));

            if (i % 500 == 0) {
                IndexManager::close_index(ih.get());
                ih = IndexManager::open_index(filename, index_no);
            }
        }

        std::cout << "Insert " << round << std::endl;
        check_equal(ih.get(), mock);

        for (int i = 0; i < round; i++) {
            auto it = mock.begin();
            int key = it->first;
            RecordID rid = it->second;
            ih->delete_entry((const uint8_t *)&key, rid);
            mock.erase(it);

            if (i % 500 == 0) {
                IndexManager::close_index(ih.get());
                ih = IndexManager::open_index(filename, index_no);
            }
        }

        std::cout << "Delete " << round << std::endl;
        check_equal(ih.get(), mock);

        IndexManager::close_index(ih.get());
        IndexManager::destroy_index(filename, index_no);
    }

    void test_ndx(int order, int round) {
        std::string filename = "abc";
        int index_no = 0;

        if (IndexManager::exists(filename, index_no)) {
            IndexManager::destroy_index(filename, index_no);
        }

        IndexManager::create_index(filename, index_no, TYPE_INT, sizeof(int));
        auto ih = IndexManager::open_index(filename, index_no);

        if (order >= 2 && order <= ih->index_header.btree_order) {
            ih->index_header.btree_order = order;
        }

        int add_cnt = 0, del_cnt = 0;
        std::multimap<int, RecordID> mock;

        for (int i = 0; i < round; i++) {
            double dice = rand() * 1. / RAND_MAX;
            double insert_prob = 1. - mock.size() / (0.5 * round);

            if (mock.empty() || dice < insert_prob) {
                int rand_key = rand() % round;
                RecordID rand_val(rand(), rand());
                ih->insert_entry((const uint8_t *)&rand_key, rand_val);
                mock.insert(std::make_pair(rand_key, rand_val));
                add_cnt++;
            } else {
                int rand_idx = rand() % mock.size();
                auto it = mock.begin();
                std::advance(it, rand_idx);
                int key = it->first;
                RecordID rid = it->second;
                ih->delete_entry((const uint8_t *)&key, rid);
                mock.erase(it);
                del_cnt++;
            }

            if (i % 500 == 0) {
                IndexManager::close_index(ih.get());
                ih = IndexManager::open_index(filename, index_no);
            }
        }

        std::cout << "Insert " << add_cnt << '\n' << "Delete " << del_cnt << '\n';

        while (!mock.empty()) {
            int rand_idx = rand() % mock.size();
            auto it = mock.begin();
            std::advance(it, rand_idx);
            int key = it->first;
            RecordID rid = it->second;
            ih->delete_entry((const uint8_t *)&key, rid);
            mock.erase(it);

            if (mock.size() % 500 == 0) {
                IndexManager::close_index(ih.get());
                ih = IndexManager::open_index(filename, index_no);
            }
        }

        check_equal(ih.get(), mock);
        IndexManager::close_index(ih.get());
        IndexManager::destroy_index(filename, index_no);
    }
};

TEST_F(IndexTest, basic) {
    srand(static_cast<unsigned>(time(nullptr)));
    test_ndx_insert_delete(3, 1000);
    test_ndx(4, 1000);
    test_ndx(3, 100000);
}
