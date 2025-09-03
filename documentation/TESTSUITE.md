# How to write a test suite for unit testing in C++

### 1. Create a <feature>_test.cpp

### 2. Include the headers

```bash
    #include "feature.h"
    #include <gtest/gtest.h> // Google test framework
```

### 3. One test suite per class/module

```bash
    TEST(<TestSuiteName>,<TestName>)
```

```bash
    EXPECT_EQ(<function call>,actual output)
    EXPECT_THROW(<function call>,actual error)
    EXPECT_TRUE(<function call>)
```

### 4. Some rules:
- Tests should be independent
- Have descriptive assertions


### 5. Run the tests

Refer to the CMakeLists.txt in root and src folders
