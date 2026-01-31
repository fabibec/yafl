#!/bin/bash

# Simple script to check all tests quickly

# Configuration
COMPILER="../src/yaflc"
PASSED_COUNT=0
FAILED_COUNT=0
EXPECTED_FAIL_COUNT=0
UNEXPECTED_PASS_COUNT=0

# Tests that are expected to compile successfully
SHOULD_PASS=(
    arith array basic builtins cast copy defaults
    elif escapes exotic for_range input loop_control
    loops match mutate_str optim polymorphic_arith
    range_print scope slice string
)

# Tests that are expected to fail compilation
SHOULD_FAIL=(
    bad_default empty nested_func syntax_error
    type_checks unary
)

echo "--- Running YAFL Test Suite ---"
# Check tests that should pass
echo "Verifying tests that should compile..."
for test in "${SHOULD_PASS[@]}"; do
    if $COMPILER -o "$test.yaflb" "$test.yafl" > /dev/null 2>&1; then
        ((PASSED_COUNT++))
    else
        echo "  [FAIL] $test.yafl (Should have compiled)"
        ((FAILED_COUNT++))
    fi
done

# Check tests that should fail
echo "Verifying tests that should fail..."
for test in "${SHOULD_FAIL[@]}"; do
    if ! $COMPILER -o "$test.yaflb" "$test.yafl" > /dev/null 2>&1; then
        ((EXPECTED_FAIL_COUNT++))
    else
        echo "  [FAIL] $test.yafl (Should NOT have compiled)"
        ((UNEXPECTED_PASS_COUNT++))
    fi
done

rm -rf *.yaflb

echo "-------------------------------"
echo "Summary:"
echo "  Successes (Pass): $PASSED_COUNT / ${#SHOULD_PASS[@]}"
echo "  Successes (Fail): $EXPECTED_FAIL_COUNT / ${#SHOULD_FAIL[@]}"
if [ $FAILED_COUNT -eq 0 ] && [ $UNEXPECTED_PASS_COUNT -eq 0 ]; then
    echo "Result: ALL TESTS PASSED"
    exit 0
else
    echo "Result: SOME TESTS FAILED"
    exit 1
fi
