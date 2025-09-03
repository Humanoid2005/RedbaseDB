#include "rm.h"
#include <gtest/gtest.h>

void rand_buf(int size, uint8_t *out_buf) {
    for (int i = 0; i < size; i++) {
        out_buf[i] = rand() & 0xff;
    }
}

struct RecordID_hash_t {
    size_t operator()(const RecordID &RecordID) const { return (RecordID.page_no << 16) | RecordID.slot_no; }
};

struct RecordID_equal_t {
    bool operator()(const RecordID &x, const RecordID &y) const { return x.page_no == y.page_no && x.slot_no == y.slot_no; }
};

void check_equal(const RM_FileHandle *fh, const std::unordered_map<RecordID, std::string, RecordID_hash_t, RecordID_equal_t> &mock) {
    // Test all records
    for (auto &entry : mock) {
        RecordID RecordID = entry.first;
        auto mock_buf = (uint8_t *)entry.second.c_str();
        auto rec = fh->get_record(RecordID);
        EXPECT_EQ(memcmp(mock_buf, rec->data, fh->hdr.record_size), 0);
    }
    // Randomly get record
    for (int i = 0; i < 10; i++) {
        RecordID RecordID(1 + rand() % (fh->hdr.num_pages - 1), rand() % fh->hdr.num_records_per_page);
        bool mock_exist = mock.count(RecordID) > 0;
        bool rm_exist = fh->is_record(RecordID);
        EXPECT_EQ(rm_exist, mock_exist);
    }
    // Test RM scan
    size_t num_records = 0;
    for (RM_Iterator scan(fh); !scan.is_end(); scan.next()) {
        EXPECT_GT(mock.count(scan.get_RecordID()), 0);
        auto rec = fh->get_record(scan.get_RecordID());
        EXPECT_EQ(memcmp(rec->data, mock.at(scan.get_RecordID()).c_str(), fh->hdr.record_size), 0);
        num_records++;
    }
    EXPECT_EQ(num_records, mock.size());
}

std::ostream &operator<<(std::ostream &os, const RecordID &RecordID) {
    return os << '(' << RecordID.page_no << ", " << RecordID.slot_no << ')';
}

TEST(rm, basic) {
    srand((unsigned)time(nullptr));

    std::unordered_map<RecordID, std::string, RecordID_hash_t, RecordID_equal_t> mock;

    std::string filename = "abc.txt";

    int record_size = 4 + rand() % 256;
    // test files
    {
        if (PF_Manager::is_file(filename)) {
            PF_Manager::destroy_file(filename);
        }
        RM_Manager::create_file(filename, record_size);
        auto fh = RM_Manager::open_file(filename);
        EXPECT_EQ(fh->hdr.record_size, record_size);
        EXPECT_EQ(fh->hdr.first_free, RM_NO_PAGE);
        EXPECT_EQ(fh->hdr.num_pages, 1);
        int max_bytes =
            fh->hdr.record_size * fh->hdr.num_records_per_page + fh->hdr.bitmap_size + (int)sizeof(RM_PageHeader);
        EXPECT_LE(max_bytes, PAGE_SIZE);
        int rand_val = rand();
        fh->hdr.num_pages = rand_val;
        RM_Manager::close_file(fh.get());
        // reopen file
        fh = RM_Manager::open_file(filename);
        EXPECT_EQ(fh->hdr.num_pages, rand_val);
        RM_Manager::close_file(fh.get());
        RM_Manager::destroy_file(filename);
    }
    // test pages
    RM_Manager::create_file(filename, record_size);
    auto fh = RM_Manager::open_file(filename);

    uint8_t write_buf[PAGE_SIZE];
    size_t add_cnt = 0;
    size_t upd_cnt = 0;
    size_t del_cnt = 0;
    for (int round = 0; round < 10000; round++) {
        double insert_prob = 1. - mock.size() / 2500.;
        double dice = rand() * 1. / RAND_MAX;
        if (mock.empty() || dice < insert_prob) {
            rand_buf(fh->hdr.record_size, write_buf);
            RecordID RecordID = fh->insert_record(write_buf);
            mock[RecordID] = std::string((char *)write_buf, fh->hdr.record_size);
            add_cnt++;
        } else {
            // update or erase random RecordID
            int RecordID_idx = rand() % mock.size();
            auto it = mock.begin();
            for (int i = 0; i < RecordID_idx; i++) {
                it++;
            }
            auto RecordID = it->first;
            if (rand() % 2 == 0) {
                // update
                rand_buf(fh->hdr.record_size, write_buf);
                fh->update_record(RecordID, write_buf);
                mock[RecordID] = std::string((char *)write_buf, fh->hdr.record_size);
                upd_cnt++;
            } else {
                // erase
                fh->delete_record(RecordID);
                mock.erase(RecordID);
                del_cnt++;
            }
        }
        // Randomly re-open file
        if (round % 500 == 0) {
            RM_Manager::close_file(fh.get());
            fh = RM_Manager::open_file(filename);
        }
        check_equal(fh.get(), mock);
    }
    EXPECT_EQ(mock.size(), add_cnt - del_cnt);
    std::cout << "insert " << add_cnt << '\n' << "delete " << del_cnt << '\n' << "update " << upd_cnt << '\n';
    // clean up
    RM_Manager::close_file(fh.get());
    RM_Manager::destroy_file(filename);
}