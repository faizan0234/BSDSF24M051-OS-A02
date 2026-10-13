# Operating Systems Programming Assignment 02

## Feature 7: Recursive Listing (-R)

### Q1. What is the base case that prevents infinite recursion?

The program skips the special directory entries `.` and `..`. The `.` entry refers to the current directory, while `..` refers to the parent directory. Skipping these entries prevents the program from repeatedly visiting the same directories. The program recursively processes entries identified as directories.

### Q2. Why do we construct the full path `parent_dir/subdir` instead of using only `subdir`?

A subdirectory name alone may not identify the correct directory when it is located inside another directory. Constructing the full path `parent_dir/subdir` allows the program to access the correct location, inspect its contents, and recursively list nested directories.
