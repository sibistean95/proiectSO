#!/bin/bash

file_path=$1

if [ ! -r "$file_path" ] || [ ! -x "$file_path" ]; then
    echo "Insufficient permissions to read or execute file: $file_path"
    exit 1
fi

echo "Checking file: $file_path"

num_lines=$(wc -l < "$file_path")
echo "Number of lines: $num_lines"

num_words=$(wc -w < "$file_path")
echo "Number of words: $num_words"

num_chars=$(wc -c < "$file_path")
echo "Number of characters: $num_chars"

echo "Checking for keywords:"
keywords=("virus" "trojan" "malware" "spyware")

for keyword in "${keywords[@]}"; do
    if grep -q "$keyword" "$file_path"; then
        echo "Keyword \"$keyword\" found!"
    fi
done

echo "Checking for non-ASCII characters:"
if grep -q -P '[^\x00-\x7F]' "$file_path"; then
    echo "Non-ASCII characters found!"
fi

safe_dir="DirectorSafe"

if [ $num_lines -gt 100 ] || [ $num_words -gt 1000 ] || [ $num_chars -gt 10000 ]; then
    echo "File $file_path is too large, moving to $safe_dir"
    mv "$file_path" "$safe_dir/"
    echo "File $file_path moved to $safe_dir"
else
    echo "File $file_path is safe"
fi
