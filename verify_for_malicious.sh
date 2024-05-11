#!/bin/bash

# Script pentru verificarea fișierelor malitioase sau corupte

file_path=$1

# Verificăm dacă fișierul există
if [ ! -f "$file_path" ]; then
    echo "File $file_path not found!"
    exit 1
fi

# Verificăm numărul de linii
num_lines=$(wc -l < "$file_path")
echo "Number of lines: $num_lines"

# Verificăm numărul de cuvinte
num_words=$(wc -w < "$file_path")
echo "Number of words: $num_words"

# Verificăm numărul de caractere
num_chars=$(wc -c < "$file_path")
echo "Number of characters: $num_chars"

# Cautăm cuvinte cheie asociate cu fișiere corupte sau malitioase
keywords=("corrupted" "dangerous" "risk" "attack" "malware" "malicious")

echo "Checking for keywords:"
for keyword in "${keywords[@]}"; do
    grep -q "$keyword" "$file_path"
    if [ $? -eq 0 ]; then
        echo "File $file_path contains keyword '$keyword'"
        exit 2
    fi
done

# Verificăm existența de caractere non-ASCII
echo "Checking for non-ASCII characters:"
grep -q -P "[\x80-\xFF]" "$file_path"
if [ $? -eq 0 ]; then
    echo "File $file_path contains non-ASCII characters"
    exit 3
fi

# Dacă nu s-a găsit nicio problemă, fișierul este considerat sigur
echo "File $file_path is safe"

exit 0
