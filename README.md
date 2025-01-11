# **Optimized IP Route Lookup Using Compressed Binary Tries**



[![Status](https://img.shields.io/badge/Status-Complete-brightgreen)](#)

## **📖 Overview**

The project introduces an efficient solution for IP route lookup by leveraging compressed binary tries (Patricia Trie). It aims to optimize the handling of large routing tables, ensuring fast lookup times and minimal memory consumption. The implementation simulates routing behavior by processing input packets, providing detailed performance metrics such as node accesses and lookup times. This makes it a valuable tool for high-performance networking systems and academic research. The optimization of the Forwarding Information Base (FIB) enhances lookup speeds while minimizing memory usage, addressing critical needs in modern networking infrastructures.

## **📋 Table of Contents**
- 🚀 [Introduction](#-introduction)
- 🛠  [Implementation Details & Structure](#-implementation-details--structure)
- 📌 [How to Use](#-how-to-use)
- 📊 [Visual Results](#-visual-results)

## 🚀 **Introduction** 

Efficient IP route lookup is critical for modern networking devices like routers and switches. This program uses a compressed binary trie to optimize the lookup process. By reducing redundancy and optimizing memory usage, the algorithm achieves a balance between speed and resource consumption.

### 📌 Key Features
- **Optimized Binary Trie**: Implements a compressed Patricia Trie for efficient route searches by reducing redundant nodes.
- **Fast & Scalable**: Optimized for large routing tables with low memory and computational overhead, performing efficient IP-to-interface mapping.
- **Metrics Tracking**: Provides detailed statistics on search performance by measuring the time complexity and node accesses for each search.
- **Flexible Input/Output**: Handles customizable routing table files and IP packet lists.
- **Cross-Platform Compatibility**: Designed for Unix/Linux environments with GCC support.

## 🛠 **Implementation Details**

### Components and Functionality

#### Input and Output (IO)
The program uses the `io.c` module for managing input and output files:
- **Input Files**: Includes the routing table (`FIB`) with network prefixes and output interfaces, and a file of input IP addresses.
- **Output File**: Records results such as the discovered output interface, processing time, and nodes accessed.
- **Functions**:
  - `initializeIO`: Initializes file handling for routing tables and input packets.
  - `readFIBLine`: Reads and parses a single routing table entry.
  - `readInputPacketFileLine`: Processes a single input IP address.
  - `printOutputLine` and `printSummary`: Output results and summarize performance metrics.

#### Binary Tree Structure
The Patricia trie is implemented with a `Node` structure:
- **Node Attributes**:
  - `prefix`: Network prefix as an integer.
  - `prefix_bin`: Binary representation of the prefix.
  - `prefixLength`: Length of the prefix.
  - `outInterface`: Associated output interface.
  - Pointers to left and right children.
  - Flags (`bitID`, `isOut`) for efficient traversal and compression.
  
- **Tree Construction**:
  - `insertNode`: Inserts routing entries by parsing the prefix bit-by-bit.
  - `compressTree`: Optimizes the trie by merging redundant nodes.

#### Route Lookup
The core of the project is the route lookup functionality:
- **Search Functionality**:
  - `searchInterface`: Finds the output interface for an input IP address by traversing the trie.
  - Tracks metrics such as node accesses and traversal time.

#### Packet Processing
The `main` function handles processing:
1. Reads IP addresses from the input file.
2. Uses `searchInterface` to find the matching output interface.
3. Logs metrics (e.g., processing time, node accesses) in the output file.

#### Performance Metrics
The program includes tools to evaluate its efficiency:
- Average nodes accessed and processing time per packet.
- Memory and CPU usage statistics using `getrusage`.

---

### Program Flow

#### Initialization
1. **File Setup**: Opens the routing table (`FIB`) and input packet file.
2. **Tree Construction**:
   - Reads routing table entries and constructs the Patricia trie using `insertNode`.
   - Compresses the trie to remove redundant nodes with `compressTree`.

#### Packet Processing
1. For each IP address in the input file:
   - Traverses the trie using `searchInterface` to find the most specific matching prefix.
   - Measures processing time using `clock_gettime`.
   - Logs results (output interface, nodes accessed, processing time).

#### Finalization
1. Outputs a summary with:
   - Average nodes accessed.
   - Average processing time per packet.
2. Frees allocated memory (`freeTree`) and closes files (`freeIO`).

---

### Visual Summary of Program Flow

```mermaid
graph TD;
    A[Start] --> B[Initialize IO and Load FIB];
    B --> C[Construct Patricia Trie];
    C --> D[Compress Trie];
    D --> E[Process Input Packets];
    E --> F[Search for Longest Prefix Match];
    F --> G[Log Results];
    G --> H[Generate Performance Summary];
    H --> I[Clean Up Resources];
    I --> J[End];


