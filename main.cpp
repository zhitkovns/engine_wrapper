#include "tests/test.h"

int main() {
    try {
        run_all_tests();
        return 0;
    } catch (...) {
        return 1;
    }
}