
# Buffered File Version Indexing System


---

## Introduction

This project presents a C++ based file indexing system designed to handle large text files efficiently. Instead of storing the entire file in memory, the program processes data in small fixed-size chunks, making it suitable for files of any size.

The system creates separate indexes for different file versions and provides useful statistics about the stored data.

---

## Main Features

* Incremental file processing using a fixed-size buffer
* Creation of word-frequency indexes
* Support for maintaining multiple versions
* Search for the frequency of a specific word
* Compare word frequencies across versions
* Retrieve the most common words
* Simple command-line interface
* Low and predictable memory consumption

---

## Project Files

```text
240210_Ashish.cpp   Source code
240210_Ashish.pdf   Project report
240210_Ashish.md    Documentation
240210_Ashish.jpg   Sample output
```

---

## Compilation

```bash
g++ 240210_Ashish.cpp -o analyzer
```

---

## Sample Usage

### Count Occurrences of a Word

```bash
./analyzer --file test_logs.txt --version v1 --buffer 512 --query word --word error
```

### Display Top Frequent Words

```bash
./analyzer --file test_logs.txt --version v1 --buffer 512 --query top --top 10
```

### Compare Two Versions

```bash
./analyzer --file1 v1.txt --version1 v1 --file2 v2.txt --version2 v2 --buffer 512 --query diff --word error
```

---

## Processing Workflow

1. Read input files one buffer at a time.
2. Extract valid words from the incoming data.
3. Normalize words by converting them to lowercase.
4. Update the frequency index for the selected version.
5. Execute user-specified queries on the generated index.

---

## Handling Buffer Boundaries

Since files are processed in chunks, a word may sometimes be split between two consecutive buffers. The program temporarily stores the incomplete portion and combines it with data from the next read operation before tokenization. This guarantees accurate indexing without increasing memory requirements.

