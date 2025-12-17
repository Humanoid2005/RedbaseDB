#ifndef RM_ITERATOR_H
#define RM_ITERATOR_H

#include "rm_defs.h"

class RM_FileHandle;

class RM_Iterator : public RecordIterator {
  private:
    const RM_FileHandle *_fh;
    RecordID _rid;

  public:
    RM_Iterator(const RM_FileHandle *fh);

    void next() override;

    bool is_end() const override { return _rid.page_no == RM_NO_PAGE; }

    RecordID get_RecordID() const override { return _rid; }
};

#endif