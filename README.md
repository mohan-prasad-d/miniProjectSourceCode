# 🏦 Core-Banking Transaction System (v2.0)

![C](https://img.shields.io/badge/Language-C-blue.svg) ![Environment](https://img.shields.io/badge/Environment-CLI-black.svg) ![Memory](https://img.shields.io/badge/Memory-Dynamic-brightgreen.svg)

A robust, terminal-based Bank Transaction Management System written in standard C. This project demonstrates core low-level programming concepts including **Binary File Handling, Dynamic Memory Management, and Function Pointers**, providing a highly realistic simulation of backend banking operations.

---

## ✨ Outstanding Features Added

I upgraded the basic transaction model into a fully-fledged banking system by engineering three major capabilities:

- **💸 Secure Peer-to-Peer Fund Transfers:** Users can transfer money directly between two accounts. The system handles balance deduction, validation, and real-time database updating simultaneously to prevent data mismatches.
- **📜 Real-time Audit Trails (Mini Statement):** Every financial alteration (deposits, tracking, transfers) triggers an automated append-only log to `history.txt`. No transaction is ever lost.
- **📊 Live Account Statistics:** The system automatically calculates and parses through the binary data to display the exact volume of transactions performed per active account.

---

## 🛠️ Technical Highlights & Array Utilization

To ensure the application is memory-safe and lightning-fast, I engineered the code using advanced data structures:

- **Struct Object Arrays:** In the `sort_and_display()` functionality, I used `client_data_t accounts[100]` and memory-allocated dynamic arrays (`malloc`) to load only active accounts into RAM. This makes sorting blazing fast.
- **Multi-dimensional Character Arrays:** `char last_name[15]` and `char first_name[10]` arrays rigidly structure the user string data inside the memory blocks without fragmentation.
- **Buffer Arrays (Security):** `char buffer[256]` combined with `fgets()` prevents classic C Buffer Overflow vulnerabilities during user inputs.
- **Function Pointers & Callbacks:** Leveraged `qsort()` passing custom comparator functions to sort millions of potential data blocks by Name or Balance instantly.

---

## 🔄 Evolution (V1.0 vs V2.0)

| Core Component   | Old Legacy Code (V1.0)          | Upgraded Professional Code (V2.0)                  |
| :--------------- | :------------------------------ | :------------------------------------------------- |
| **Logic flow**   | Basic CRUD operations only      | Added Inter-account Fund Transfers                 |
| **Data Logging** | None (Data overwritten forever) | Persistent append-only logging (`history.txt`)     |
| **Calculations** | Printed dummy placeholder lines | Calculates real-time active transaction metrics    |
| **Compilation**  | Included strict syntax errors   | Refactored, memory-safe, and 0-warning compilation |

---

## 🚀 Getting Started

```bash
# Compile the robust C code using GCC
gcc trans.c -o trans

# Execute the application
./trans
```

_(Built utilizing POSIX C standards without any heavy external dependencies)._
