# This script formats all files in the format-checked directories.
find src include -iname '*.cpp' -o -iname '*.hpp' | xargs clang-format -i