# Vaultree (Work in Progress)

**A modern, incremental backup engine prototype written in C++20.**

![Status](https://img.shields.io/badge/Status-Active_Development-yellow) ![Language](https://img.shields.io/badge/Language-C++20-blue)

##  About
Vaultree is a project aimed at building a high-performance backup system from scratch. It utilizes **Content Addressable Storage (CAS)** for file-level deduplication and a **Merkle Tree** architecture for instant change detection.

**Current Status:** The core discovery engine (File Scanning, Merkle Tree generation, Database Schema) is implemented and benchmarked. The data processing pipeline (Compression & Encryption) is currently being architected.

##  Implemented Features

### Hybrid Serial/Parallel Architecture
Vaultree optimizes for different bottlenecks at different stages:
1.  **Discovery (Serial):** Uses an iterative **Breadth-First Search (BFS)** to scan the file system with minimal overhead.
2.  **Processing (Parallel):** Dispatches file processing tasks to a **Thread Pool**.

###  Iterative BFS File Discovery
* **Algorithm:** Uses an efficient, non-recursive **Breadth-First Search (BFS)** queue to traverse the file system.
* **Performance:** Minimizes system overhead by traversing serially, achieving scan speeds of ~240,000 files in **< 2 seconds** on warm cache.
* **Metadata Extraction:** Collects file timestamps (`mtm`) and sizes during traversal to identify `added` or `modified` candidates instantly.

###  SQLite Metadata Store
* **Schema:** Normalized `Many-to-One` relationship between Files and Blobs to support file-level deduplication.

### Content Addressable Storage (CAS)
* **Directory Manager:** Implements a caching mechanism to minimize syscalls when verifying storage structure (e.g., `objects/a1/b2...`).
* **Deduplication:** Ensures identical content is stored only once, regardless of filename or location.

##  Architecture & Design Goals (In Progress)

The following components are currently under development:

* **Encryption Strategy:** **XChaCha20-Poly1305** Selected for its 192-bit nonce, allowing stateless, random nonce generation across threads.
* **Compression:** **Zstandard (zstd)** with adaptive skipping - no sense in compressing jpg or mp4 twice.
* **Hashing:** **BLAKE3** currently one of the fastest hashing cryptographically secure algorithms in the world

## Technology Stack
* **Language:** C++20 (Concepts, `std::filesystem`)
* **Build System:** CMake (FetchContent)
* **Database:** SQLite3
* **Dependencies:**
    * `BLAKE3` (Hashing)
    * `SQLite3` (Metadata)

## Preliminary Benchmarks
*Test Environment: Ryzen 7, NVMe SSD*
* **BFS Scan:** ~240,000 files scanned in < 2 seconds.
* **Merkle Tree Build:** 180ms for ~240k entries.
* **Throughput:** ~115 MB/s sustained pipeline speed (Scan + Hash + DB Write). Avg file size 200 kB

## Roadmap
- [x] Iterative BFS File Scanner
- [x] SQLite Schema & Database Manager
- [x] Merkle Tree Logic for Diffing
- [x] Directory Structure Manager (CAS)
- [x] Parallel Worker Pool Implementation
- [ ] Compression Implementation (Zstd)
- [ ] Encryption Pipeline (XChaCha20-Poly1305)
- [ ] Restore Functionality