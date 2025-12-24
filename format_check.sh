# This script verifies the formatting in the target directories.
find src include -iname '*.cpp' -o -iname '*.hpp' | xargs clang-format --dry-run -Werror