# trans.c v2.0 - Code Structure & Quick Reference

---

## 📦 File Organization

```
trans.c (v2.0)
├── Headers & Definitions
│   ├── #include (5: errno, limits, stdio, stdlib, string)
│   └── #define (constants: FILES, LIMITS, MIN_BALANCE)
│
├── Data Types
│   ├── typedef struct client_data_t
│   ├── enum menu_option
│   └── typedef sort_comparator
│
├── Function Prototypes (Static)
│   ├── Main menu & entry: enter_choice()
│   ├── File initialization: initialize_file(), ensure_data_file_layout(), sanitize_file()
│   ├── Core operations: text_file(), update_record(), new_record(), delete_record()
│   ├── Queries: list_all(), search_account(), sort_and_display()
│   ├── Input handling: read_line(), prompt_unsigned_in_range(), prompt_double_in_range(), prompt_word()
│   ├── File I/O: read_account(), write_account(), account_offset()
│   ├── Validation: is_record_valid_for_slot(), is_balance_valid()
│   ├── Display: print_header(), print_client()
│   └── Comparators: compare_by_last_name(), compare_by_balance()
│
└── Implementation
    └── Functions implemented in order below
```

---

## 🔍 Function Breakdown

### Main Entry Point

```c
int main(void)
├── File open/create
├── Data validation (ensure_data_file_layout, sanitize_file)
├── Menu loop (for (;;))
│   ├── Get user choice
│   ├── Dispatch to operation
│   └── Repeat until EXIT
└── Cleanup & exit
```

---

### Tier 1: File Organization & Initialization

```c
void initialize_file(FILE *fptr)
    → Writes 100 blank client_data_t records to file
    → Called only when file created new

void ensure_data_file_layout(FILE *fptr)
    → Check: file size == MAX_ACCOUNTS * sizeof(client_data_t)
    → If wrong: reinitialize file
    → Purpose: Recover from corruption

void sanitize_file(FILE *fptr)
    → For each account 1-100:
    │   └── Read record
    │       └── If acct_num != 0 and acct_num != account:
    │           └── Blank the record (invalid entry)
    → Repair corrupted data
```

---

### Tier 2: Core Banking Operations

```c
void new_record(FILE *fptr)
    ├── Get account number (1-100)
    ├── Check: account not already exists
    ├── Get: last_name, first_name, opening_balance
    ├── Validate: balance >= MIN_BALANCE_LIMIT
    ├── Initialize: transaction_count = 0
    └── Write to file

void update_record(FILE *fptr)
    ├── Get account number
    ├── Read current record
    ├── Display current balance
    ├── Get transaction amount
    └── Call: update_record_impl(&client, amount)

void update_record_impl(client_data_t *client, double amount) [BUSINESS LOGIC]
    ├── Calculate: new_balance = current + amount
    ├── Validate: new_balance >= MIN_BALANCE_LIMIT
    ├── If valid:
    │   ├── client->balance += amount
    │   ├── client->transaction_count++
    │   └── Success message
    └── Else: Error message with details

void delete_record(FILE *fptr)
    ├── Get account number
    ├── Read record (verify exists)
    ├── Write blank record to this account
    └── Confirm deletion

void text_file(FILE *read_ptr)
    ├── Open accounts.txt for write
    ├── Write header row
    ├── For each record in memory file:
    │   └── If acct_num != 0: write to text file
    └── Close and confirm
```

---

### Tier 3: Query Operations

```c
void list_all(FILE *fptr)
    ├── Rewind file
    ├── Print header
    ├── Read all records
    ├── Print non-blank records
    └── Show count

void search_account(FILE *fptr)
    ├── Get account number
    ├── Read specific record
    ├── If found: print details
    └── Else: not found message

void sort_and_display(FILE *fptr, sort_comparator cmp, const char *title)
    ├── Allocate memory for active records
    │   └── Load all non-blank records
    ├── qsort() using comparator function
    ├── Print header and sorted records
    └── free() memory
```

---

### Tier 4: Input Handling (User Interaction)

```c
int read_line(char *buffer, size_t size)
    └── Safe line reading from stdin (no buffer overflow)

int prompt_unsigned_in_range(prompt, min, max, value_out)
    ├── Loop until valid input:
    │   ├── Print prompt
    │   ├── Read line
    │   ├── Parse strtoul()
    │   ├── Check range [min, max]
    │   └── Retry on error
    └── Return 1 if success, 0 if EOF

int prompt_double_in_range(prompt, min, max, value_out)
    ├── Same as above but for doubles
    └── Uses strtod()

int prompt_word(prompt, dest, dest_size)
    ├── Get single word (no spaces)
    ├── Length check
    ├── Validate non-empty
    └── Return 1/0 for success/fail
```

---

### Tier 5: File I/O Abstraction

```c
long account_offset(account_number)
    └── Calculate byte offset for account in file
        = (account_number - 1) * sizeof(client_data_t)

int read_account(file, account_number, client_out)
    ├── fseek() to account_offset()
    └── fread() record

int write_account(file, account_number, client_in)
    ├── fseek() to account_offset()
    ├── fwrite() record
    └── fflush() immediately
```

---

### Tier 6: Validation & Utility

```c
int is_record_valid_for_slot(client, account_number)
    └── Return: (client->acct_num == 0) OR (client->acct_num == account_number)

int is_balance_valid(balance)
    └── Return: balance >= MIN_BALANCE_LIMIT

void print_header(FILE *out)
    └── fprintf() column headers with widths

void print_client(FILE *out, client)
    └── fprintf() formatted account record

int compare_by_last_name(a, b)
    └── strcmp() last names (ascending)

int compare_by_balance(a, b)
    └── Compare balances (descending - highest first)
```

---

## 🎯 Key Data Flows

### Adding an Account

```
new_record()
  ├─→ prompt_unsigned_in_range() [get account #]
  ├─→ read_account() [check not exists]
  ├─→ prompt_word() [get last name]
  ├─→ prompt_word() [get first name]
  ├─→ prompt_double_in_range() [get balance with MIN limit]
  ├─→ is_balance_valid() [validate]
  └─→ write_account() [save to file]
```

### Updating Balance

```
update_record()
  ├─→ prompt_unsigned_in_range() [get account #]
  ├─→ read_account() [read current]
  ├─→ prompt_double_in_range() [get amount]
  └─→ update_record_impl()
      ├─→ is_balance_valid(new_balance)
      ├─→ Update balance + transaction_count
      └─→ write_account() [save]
```

### Sorting Display

```
sort_and_display()
  ├─→ malloc(active_count * sizeof)
  ├─→ read_account() × N [populate array]
  ├─→ qsort(array, count, size, comparator)
  ├─→ print_header()
  ├─→ print_client() × N
  └─→ free()
```

---

## 🔐 Error Handling Flow

```
User Input
  ↓
prompt_*() with validation
  ├─→ Valid → proceed
  └─→ Invalid → retry loop
                ├─→ User tries again
                └─→ Repeat OR EOF → return 0

File Operation
  ↓
read_account() / write_account()
  ├─→ fseek() failed? → return 0
  ├─→ fread/fwrite() failed? → return 0
  └─→ Success → return 1

Recovery
  ↓
Operation calls:
  ├─→ if (!operation()) → error message
  └─→ return to menu (user can retry)
```

---

## 📐 Memory Layout

```
Network of 100 account slots (fixed)
└── Slot 1: [acct_num=0/1,44 bytes per record]
│   └── If acct_num=0: blank (deleted/empty)
│   └── If acct_num=N: active account
├── Slot 2: [acct_num=0/2,44 bytes]
├── ...
└── Slot 100: [acct_num=0/100,44 bytes]

On Sort Operation:
└── Allocate: malloc(actual_count * 44)
    ├── Load only non-blank records
    ├── qsort() them
    ├── Display
    └── free()

Stack/Buffer Usage:
├── INPUT_BUFFER_SIZE = 256 bytes (readLine)
├── client_data_t = 44 bytes (single record)
└── Local function variables (fixed sizes)

No dynamic array in normal operations (only for sorting)
```

---

## 🔧 Compilation & Warnings

```bash
gcc trans.c -Wall -Wextra -Wpedantic -std=c11 -o trans.exe

-Wall   : All common warnings
-Wextra : Extra warnings
-Wpedantic : Strict standard compliance
-std=c11 : Use C11 standard
```

**Result:** 0 warnings ✓

---

## 📊 Comparison: Before vs. After

| Aspect               | BEFORE       | AFTER                            |
| -------------------- | ------------ | -------------------------------- |
| Functions            | 14           | 23                               |
| Type checking        | struct       | typedef                          |
| Naming               | camelCase    | snake_case                       |
| Sort support         | No           | Yes, 2 types                     |
| Neg balance limit    | No           | Yes, -1000.0                     |
| Transaction track    | No           | Yes, per account                 |
| Input validation     | Unsafe scanf | Safe prompt\_\* with retry       |
| File I/O abstraction | Scattered    | Centralized (read/write_account) |
| Error handling       | Basic        | Comprehensive                    |
| Memory efficiency    | Adequate     | Optimized (compact sort)         |

---

## 🎓 Interview Talking Points by Function

### You should be able to explain:

1. **initialize_file()** → "Prepares empty 100-slot database structure"
2. **ensure_data_file_layout()** → "Self-healing file corruption detection"
3. **sanitize_file()** → "Data integrity validation on startup"
4. **update_record_impl()** → "Business logic separated from I/O"
5. **sort_and_display()** → "Memory-efficient sorting with qsort()"
6. **prompt_unsigned_in_range()** → "Safe input validation with retry"
7. **is_balance_valid()** → "Enforce business rules (negative limit)"
8. **read_account()** / **write_account()** → "File I/O abstraction layer"
9. **compare_by_last_name()** / **compare_by_balance()** → "Extensible comparators for sorting"

---

## ✨ Project Readiness Checklist

- [x] Code compiles with -Wall -Wextra -Wpedantic
- [x] All 10 menu options working
- [x] Sorting implemented with qsort()
- [x] Balance limits enforced
- [x] Transaction tracking working
- [x] Error handling comprehensive
- [x] snake_case naming throughout
- [x] Functions properly decomposed
- [x] Memory efficient (no leaks)
- [x] Demo script prepared

**Ready for evaluation!**
