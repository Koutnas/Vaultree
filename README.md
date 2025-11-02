# Vaultree
Vaultree is a lightweight, integrity-aware backup tool written in C++.
It recursively scans directories, builds a Merkle tree to detect changes, and stores file metadata in SQLite.
Future goals include compression and encryption for fully secure offline backups.

## Features
- Recursive filesystem scanner (BFS-based)
- SQLite-based metadata storage
- Skip lists for excluded directories
- Modular design for future encryption/compression

## Planned Features
- File compression (Zstd or LZ4)
- Encryption (AES-256)
- Incremental backups with deduplication
