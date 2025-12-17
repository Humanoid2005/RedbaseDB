# Query Language Module

## Overview

The Query Language module implements SQL-like query processing for the database system. It provides a complete query execution engine that supports data manipulation (INSERT, UPDATE, DELETE) and retrieval (SELECT) operations. The module uses an **iterator-based execution model** with a **query plan tree** for efficient query processing.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      QL_Manager                              │
│  (Query Interface - parses and executes SQL operations)      │
└────────────┬───────────────────────────────────┬────────────┘
             │                                   │
             │ Creates Query Plans               │ Uses
             ↓                                   ↓
┌────────────────────────────┐      ┌──────────────────────────┐
│   QlNode (Abstract Base)   │      │    SM_Manager            │
│  - Iterator Protocol       │      │  (Metadata & File Access)│
└────────────┬───────────────┘      └──────────────────────────┘
             │
      ┌──────┴────────┬─────────────┬──────────────┐
      ↓               ↓             ↓              ↓
┌──────────┐    ┌──────────┐  ┌──────────┐  ┌──────────┐
│QlNodeTable│   │QlNodeProj│  │QlNodeJoin│  │  Value   │
│ (Scan)   │    │(Project) │  │ (Join)   │  │TabCol/   │
└──────────┘    └──────────┘  └──────────┘  │Condition │
                                             └──────────┘
```

## Components

### 1. QL_Manager (ql_manager.h/cpp)

**Purpose:** High-level interface for executing SQL operations. Handles query parsing, validation, and execution.

**Key Methods:**
- `InsertResult insert_into(table_name, values)` - Insert a row into a table
- `DeleteResult delete_from(table_name, conditions)` - Delete rows matching conditions
- `UpdateResult update_set(table_name, set_clauses, conditions)` - Update rows
- `SelectResult select_from(columns, tables, conditions)` - Query data

**Features:**
- **Column name inference:** Automatically resolves table names for unqualified columns
- **Type checking:** Validates data types in WHERE clauses and SET clauses
- **Index utilization:** Automatically uses indexes when available for better performance
- **Multi-database support:** Works seamlessly with SM_Manager's database switching

---

### 2. QlNode Abstract Base Class (ql_node.h/cpp)

**Purpose:** Defines the iterator protocol for query execution operators.

**Iterator Protocol:**
```cpp
void begin()                    // Initialize iterator
void next()                     // Move to next tuple
bool is_end()                   // Check if iteration complete
unique_ptr<RM_Record> rec()     // Get current record
```

**Why Iterator Model?**
- **Memory efficient:** Processes one tuple at a time (pipeline execution)
- **Composable:** Operators can be chained to build complex query plans
- **Lazy evaluation:** Only computes results as needed

---

### 3. QlNodeTable (ql_node.h/cpp)

**Purpose:** Leaf node in query plan - scans a single table with optional filtering.

**Key Features:**

**a) Scan Strategy Selection:**
```
IF index available on condition column:
    Use IndexIterator (fast range scan)
ELSE:
    Use RM_Iterator (full table scan)
```

**b) Index Range Scan Optimization:**
```cpp
// Translates WHERE conditions to index bounds
WHERE age = 25     → [lower_bound(25), upper_bound(25)]
WHERE age < 25     → [leaf_begin(), lower_bound(25)]
WHERE age > 25     → [upper_bound(25), leaf_end()]
WHERE age <= 25    → [leaf_begin(), upper_bound(25)]
WHERE age >= 25    → [lower_bound(25), leaf_end()]
```

**c) Condition Evaluation:**
- **Index conditions:** Applied via index bounds (pre-filtering)
- **Non-index conditions:** Applied via `eval_conds()` (post-filtering)
- **Join conditions:** Fed from outer tables via `feed()` method

**Algorithm:**
```
PROCEDURE QlNodeTable.begin():
    1. Select scan strategy (index vs. full scan)
    2. If index available:
        a. Convert WHERE conditions to index bounds
        b. Create IndexIterator with [lower, upper]
    3. Else:
        a. Create RM_Iterator for full table scan
    4. Find first matching record:
        WHILE NOT scan.is_end():
            rid = scan.get_RecordID()
            record = fh.get_record(rid)
            IF eval_conds(record):
                BREAK
            scan.next()
```

---

### 4. QlNodeProj (ql_node.h/cpp)

**Purpose:** Projection operator - selects specific columns from input tuples.

**Operation:**
```
Input:  [col1, col2, col3, col4, col5]
Select: [col2, col4]
Output: [col2, col4]
```

**Implementation:**
- Stores column index mappings (`_sel_idxs`)
- Copies selected column data to new record
- Recomputes column offsets for output schema

**Example:**
```sql
SELECT name, age FROM students
         ↓
    QlNodeProj(columns=[name, age])
         ↓
    QlNodeTable(students)
```

---

### 5. QlNodeJoin (ql_node.h/cpp)

**Purpose:** Join operator - combines tuples from two input streams.

**Algorithm:** Nested Loop Join
```
PROCEDURE QlNodeJoin.begin():
    left.begin()
    FOR EACH left_tuple IN left:
        feed_right(left_tuple)      // Pass left values to right
        right.begin()
        FOR EACH right_tuple IN right:
            YIELD concat(left_tuple, right_tuple)
```

**Condition Feeding:**
- Evaluates join conditions by "feeding" values from left table to right
- Enables index usage on right table even for join conditions
- Example:
  ```sql
  SELECT * FROM orders o, customers c 
  WHERE o.customer_id = c.id
  ```
  - Left (orders): Scans all orders
  - Right (customers): For each order, feeds `customer_id` to right scan
  - If index on `customers.id`, uses index lookup!

**Record Construction:**
```cpp
concat(left_record, right_record)
// Creates new record: [left_cols..., right_cols...]
```

---

### 6. Data Structures (ql_manager.h, ql_defs.h)

#### Value
```cpp
struct Value {
    ColumnType type;           // INT, FLOAT, STRING, DATETIME
    union { int, float };      // Typed value
    string str_val;
    shared_ptr<RM_Record> raw; // Serialized binary form
}
```
**Purpose:** Represents a constant value in queries (literals in WHERE/SET clauses).

#### TabCol
```cpp
struct TabCol {
    string tab_name;  // Table name (can be empty, inferred later)
    string col_name;  // Column name
}
```
**Purpose:** References a column, potentially from a specific table.

#### Condition
```cpp
struct Condition {
    TabCol lhs_col;           // Left column
    CompOp op;                // =, !=, <, >, <=, >=
    bool is_rhs_val;          // Is RHS a value or column?
    TabCol rhs_col;           // Right column (if not value)
    Value rhs_val;            // Right value (if not column)
}
```
**Purpose:** Represents a WHERE clause predicate.

#### SetClause
```cpp
struct SetClause {
    TabCol lhs;  // Column to update
    Value rhs;   // New value
}
```
**Purpose:** Represents an UPDATE SET assignment.

---

### 7. Result Structures (ql_defs.h)

All query operations return structured results instead of printing to terminal.

#### SelectResult
```cpp
struct SelectResult {
    vector<string> column_names;  // Column headers
    vector<SelectRow> rows;       // Data rows
}
```
**Usage:**
```cpp
auto result = QL_Manager::select_from({}, {"students"}, {});
for (auto& row : result.rows) {
    for (auto& value : row.values) {
        cout << value.value << " ";
    }
}
```

#### InsertResult, UpdateResult, DeleteResult
```cpp
struct InsertResult {
    string table_name;
    bool success;
    string message;
}

struct UpdateResult / DeleteResult {
    string table_name;
    size_t affected_rows;
    bool success;
    string message;
}
```

**Benefits:**
- ✅ No terminal dependency (suitable for embedded/server use)
- ✅ Easy serialization to JSON/XML for APIs
- ✅ Better error handling with success flags
- ✅ Structured data for external services

---

## Query Execution Flow

### SELECT Query
```sql
SELECT name, age FROM students WHERE age > 18
```

**Execution Steps:**

1. **Parse & Validate:**
   ```cpp
   check_column({"students"}, "name")  // Verify column exists
   check_column({"students"}, "age")
   check_where_clause({"students"}, conditions)  // Type check
   ```

2. **Build Query Plan:**
   ```
   QlNodeProj(columns=[name, age])
       ↓
   QlNodeTable(students, conditions=[age > 18])
   ```

3. **Execute (Iterator Protocol):**
   ```cpp
   query_plan.begin()
   WHILE NOT query_plan.is_end():
       record = query_plan.rec()
       result.add_row(record)
       query_plan.next()
   ```

4. **Return SelectResult:**
   ```cpp
   SelectResult {
       column_names: ["name", "age"]
       rows: [
           ["Alice", "20"],
           ["Bob", "22"],
           ...
       ]
   }
   ```

---

### JOIN Query
```sql
SELECT o.id, c.name 
FROM orders o, customers c 
WHERE o.customer_id = c.id AND o.amount > 100
```

**Query Plan:**
```
QlNodeProj(columns=[o.id, c.name])
    ↓
QlNodeJoin
    ↓                           ↓
QlNodeTable(orders)      QlNodeTable(customers)
conditions=[amount>100]  conditions=[id=?(fed)]
```

**Execution:**
```
FOR EACH order IN orders WHERE amount > 100:
    feed(customer_id = order.customer_id)
    FOR EACH customer IN customers WHERE id = order.customer_id:
        YIELD [order.id, customer.name]
```

---

### INSERT Query
```sql
INSERT INTO students VALUES (1, 'Alice', 20)
```

**Algorithm:**
```
PROCEDURE insert_into(table_name, values):
    1. Validate value count matches column count
    2. Type check each value against column type
    3. Serialize values to record format
    4. Insert record → record_file (get RID)
    5. FOR EACH indexed column:
        Insert (key, RID) → index_file
    6. RETURN InsertResult(success, message)
```

---

### UPDATE Query
```sql
UPDATE students SET age = 21 WHERE name = 'Alice'
```

**Algorithm:**
```
PROCEDURE update_set(table_name, set_clauses, conditions):
    1. Validate conditions and set clauses (types)
    2. Scan table to collect all matching RIDs
    3. FOR EACH rid IN matching_rids:
        a. Get old record
        b. Delete old index entries (for updated columns)
        c. Apply SET clauses to record
        d. Update record in record_file
        e. Insert new index entries
    4. RETURN UpdateResult(affected_rows, success)
```

**Note:** Two-phase approach (collect RIDs, then update) avoids iterator invalidation.

---

### DELETE Query
```sql
DELETE FROM students WHERE age < 18
```

**Algorithm:**
```
PROCEDURE delete_from(table_name, conditions):
    1. Validate conditions
    2. Scan table to collect all matching RIDs
    3. FOR EACH rid IN matching_rids:
        a. Get record
        b. Delete from all index files
        c. Delete from record file
    4. RETURN DeleteResult(affected_rows, success)
```

---

## Index Utilization Strategy

**Decision Process:**
```
FOR EACH condition IN where_clause:
    IF condition.op != NE AND condition.rhs IS constant:
        IF column HAS index:
            USE IndexIterator with range [lower, upper]
            BREAK
    
IF no_index_found:
    USE RM_Iterator (full table scan)
```

**Supported Index Operations:**
- `=` (equality): Exact range `[lower_bound, upper_bound]`
- `<` (less than): Range `[begin, lower_bound)`
- `>` (greater than): Range `(upper_bound, end]`
- `<=`: Range `[begin, upper_bound]`
- `>=`: Range `[lower_bound, end]`
- `!=` (not equal): **Cannot use index** (requires full scan)

**Limitations:**
- Only one index per table scan (picks first available)
- No index intersection/union
- Future enhancement: maintain interval for multiple conditions

---

## Integration with Other Modules

### 1. System Management (SM_Manager)
```cpp
// Access metadata
Table_Metadata& tab = SM_Manager::db.get_table(table_name);

// Access file handles
RM_FileHandle* fh = SM_Manager::fhs.at(table_name).get();

// Access index handles
IndexHandle* ih = SM_Manager::ihs.at(index_name).get();
```

**Multi-Database Support:**
- Query language automatically works with currently open database
- SM_Manager::db changes when switching databases
- No database name needed in queries (implicit context)

### 2. Record Manager (RM)
```cpp
// Insert
RecordID rid = fh->insert_record(data);

// Delete
fh->delete_record(rid);

// Update
fh->update_record(rid, new_data);

// Scan
RM_Iterator iter(fh);
```

### 3. Index Handler (IndexHandle)
```cpp
// Insert
ih->insert_entry(key, rid);

// Delete
ih->delete_entry(key, rid);

// Range scan
IndexID lower = ih->lower_bound(key);
IndexID upper = ih->upper_bound(key);
IndexIterator iter(ih, lower, upper);
```

---

## Error Handling

### Common Exceptions
- `ColumnNotFoundError` - Referenced column doesn't exist
- `AmbiguousColumnError` - Column name ambiguous (multiple tables have it)
- `IncompatibleTypeError` - Type mismatch in comparisons/assignments
- `InvalidValueCountError` - Wrong number of values in INSERT
- `TableNotFoundError` - Table doesn't exist
- `InternalError` - Unexpected internal state

### Result-Based Error Reporting
All operations return result structures with:
- `success` flag (true/false)
- `message` field (error description or success message)

Example:
```cpp
auto result = QL_Manager::insert_into("students", values);
if (!result.success) {
    cout << "Error: " << result.message << endl;
}
```

---

## Design Patterns

### 1. Iterator Pattern
**Purpose:** Uniform interface for scanning different data sources.
- All nodes implement `begin()`, `next()`, `is_end()`, `rec()`
- Enables pipeline execution (operator chaining)

### 2. Composite Pattern
**Purpose:** Build complex query plans from simple operators.
- QlNode is abstract component
- QlNodeTable (leaf), QlNodeProj/QlNodeJoin (composite)
- Recursive execution via tree traversal

### 3. Strategy Pattern
**Purpose:** Select scan strategy at runtime.
- IndexIterator vs RM_Iterator
- Decision based on available indexes

### 4. Template Method Pattern
**Purpose:** Common validation logic with customizable execution.
- `check_where_clause()` - common validation
- Specific execution in each operation

---

## Performance Considerations

### Optimization Techniques

1. **Index Usage:**
   - Automatically selects indexed columns in WHERE clauses
   - Converts conditions to efficient range scans
   - Example: `WHERE id = 5` → O(log N) instead of O(N)

2. **Two-Phase Updates/Deletes:**
   - Phase 1: Collect RIDs (read-only scan)
   - Phase 2: Modify records
   - Prevents iterator invalidation during modification

3. **Pipeline Execution:**
   - Iterator model processes one tuple at a time
   - Low memory footprint (no materialization)
   - Results streamed to output

4. **Join Order:**
   - Left-deep join trees
   - More selective table should be on left (fewer outer loop iterations)
   - Future: cost-based optimization

### Complexity Analysis

| Operation | Without Index | With Index |
|-----------|---------------|------------|
| SELECT (equality) | O(N) | O(log N + M) |
| SELECT (range) | O(N) | O(log N + M) |
| INSERT | O(1) + O(I·log N)* | - |
| UPDATE | O(N) + O(I·log N)* | O(log N + M) + O(I·log N)* |
| DELETE | O(N) + O(I·log N)* | O(log N + M) + O(I·log N)* |
| JOIN | O(N·M) | O(N·log M)** |

*I = number of indexes, N = table size  
**If right table has index on join column  
M = result size

---

## Future Enhancements

### Planned Features
1. **Query Optimization:**
   - Cost-based query planning
   - Join reordering
   - Predicate pushdown

2. **Advanced Operators:**
   - Aggregation (COUNT, SUM, AVG, MIN, MAX)
   - GROUP BY / HAVING
   - ORDER BY / LIMIT
   - Subqueries

3. **Index Improvements:**
   - Multiple index usage (index intersection)
   - Interval maintenance for compound conditions
   - Index-only scans (covering indexes)

4. **Join Algorithms:**
   - Hash join
   - Sort-merge join
   - Index nested loop join

5. **Parallel Execution:**
   - Multi-threaded scanning
   - Parallel joins

---

## Example Usage

### Basic Operations

```cpp
#include "query_language/ql.h"

// INSERT
Value id, name, age;
id.set_int(1);
name.set_str("Alice");
age.set_int(20);
auto ins_result = QL_Manager::insert_into("students", {id, name, age});

// SELECT
auto sel_result = QL_Manager::select_from(
    {TabCol("", "name"), TabCol("", "age")},  // Columns
    {"students"},                              // Tables
    {}                                         // Conditions
);

// Display results
for (size_t i = 0; i < sel_result.column_names.size(); i++) {
    cout << sel_result.column_names[i] << "\t";
}
cout << endl;
for (auto& row : sel_result.rows) {
    for (auto& val : row.values) {
        cout << val.value << "\t";
    }
    cout << endl;
}

// UPDATE
Condition cond;
cond.lhs_col = TabCol("", "name");
cond.op = OP_EQ;
cond.is_rhs_val = true;
cond.rhs_val.set_str("Alice");

SetClause set;
set.lhs = TabCol("", "age");
set.rhs.set_int(21);

auto upd_result = QL_Manager::update_set("students", {set}, {cond});
cout << upd_result.message << endl;  // "1 row(s) updated"

// DELETE
auto del_result = QL_Manager::delete_from("students", {cond});
cout << del_result.message << endl;  // "1 row(s) deleted"
```

---

## Testing

Test file: `ql_test.cpp`

**Test Categories:**
1. Basic operations (INSERT, SELECT, UPDATE, DELETE)
2. WHERE clause filtering
3. Join queries
4. Index utilization
5. Multi-table queries
6. Error conditions

---

## Summary

The Query Language module provides a complete SQL-like query execution engine with:
- ✅ **Iterator-based execution** for memory efficiency
- ✅ **Automatic index utilization** for performance
- ✅ **Composable query operators** for complex queries
- ✅ **Structured output** for external service integration
- ✅ **Multi-database support** via SM_Manager integration
- ✅ **Type safety** with compile-time and runtime checks

The modular design allows easy extension with new operators and optimizations while maintaining clean separation of concerns with other database modules.
