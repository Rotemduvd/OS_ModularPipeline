#!/bin/bash

echo "Running tests for Operating Systems Final Project..."

# Build the project first
./build.sh

if [ $? -ne 0 ]; then
    echo "Build failed! Cannot run tests."
    exit 1
fi

# Run the main application
echo "Running main application..."
./build/final_project

# Run specific plugin tests
echo "Testing monitor plugin..."
# Add specific test commands here

echo "Testing sync plugin..."
# Add specific test commands here

echo "All tests completed!" 