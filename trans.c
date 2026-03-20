#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE "credit.dat"
#define REPORT_FILE "accounts.txt"

#define MAX_ACCOUNTS 100U
#define LAST_NAME_SIZE 15
#define FIRST_NAME_SIZE 10
#define INPUT_BUFFER_SIZE 256
#define MIN_BALANCE_LIMIT -1000.0

typedef struct {
    unsigned int acct_num;
    char last_name[LAST_NAME_SIZE];
    char first_name[FIRST_NAME_SIZE];
    double balance;
    unsigned int transaction_count;
} client_data_t;

enum menu_option {
    MENU_CREATE_TEXT = 1,
    MENU_UPDATE = 2,
    MENU_ADD = 3,
    MENU_DELETE = 4,
    MENU_EXIT = 5,
    MENU_LIST_ALL = 6,
    MENU_SEARCH = 7,
    MENU_SORT_BY_NAME = 8,
    MENU_SORT_BY_BALANCE = 9,
    MENU_VIEW_TRANSACTIONS = 10
};

typedef int (*sort_comparator)(const void *, const void *);

static unsigned int enter_choice(void);
static void initialize_file(FILE *fptr);
static void ensure_data_file_layout(FILE *fptr);
static void sanitize_file(FILE *fptr);
static void text_file(FILE *read_ptr);
static void update_record_impl(client_data_t *client, double amount);
static void update_record(FILE *fptr);
static void new_record(FILE *fptr);
static void delete_record(FILE *fptr);
static void list_all(FILE *fptr);
static void search_account(FILE *fptr);
static void sort_and_display(FILE *fptr, sort_comparator cmp, const char *title);

static int read_line(char *buffer, size_t size);
static int prompt_unsigned_in_range(const char *prompt, unsigned int min, unsigned int max, unsigned int *value);
static int prompt_double_in_range(const char *prompt, double min, double max, double *value);
static int prompt_word(const char *prompt, char *dest, size_t dest_size);

static long account_offset(unsigned int account_number);
static int read_account(FILE *fptr, unsigned int account_number, client_data_t *client);
static int write_account(FILE *fptr, unsigned int account_number, const client_data_t *client);
static int is_record_valid_for_slot(const client_data_t *client, unsigned int account_number);
static int is_balance_valid(double balance);

static void print_header(FILE *out);
static void print_client(FILE *out, const client_data_t *client);

static int compare_by_last_name(const void *a, const void *b);
static int compare_by_balance(const void *a, const void *b);

int main(void) {
    FILE *cfptr = fopen(DATA_FILE, "rb+");

    puts("\n==== BANK TRANSACTION SYSTEM (v2.0) ====");
    puts("Professional Edition with Sorting & Balance Protection");

    if (cfptr == NULL) {
        cfptr = fopen(DATA_FILE, "wb+");
        if (cfptr == NULL) {
            fputs("Error: unable to create data file.\n", stderr);
            return EXIT_FAILURE;
        }
        initialize_file(cfptr);
    }

    ensure_data_file_layout(cfptr);
    sanitize_file(cfptr);

    for (;;) {
        unsigned int choice = enter_choice();

        if (choice == MENU_EXIT) {
            break;
        }

        switch (choice) {
            case MENU_CREATE_TEXT:
                text_file(cfptr);
                break;
            case MENU_UPDATE:
                update_record(cfptr);
                break;
            case MENU_ADD:
                new_record(cfptr);
                break;
            case MENU_DELETE:
                delete_record(cfptr);
                break;
            case MENU_LIST_ALL:
                list_all(cfptr);
                break;
            case MENU_SEARCH:
                search_account(cfptr);
                break;
            case MENU_SORT_BY_NAME:
                sort_and_display(cfptr, compare_by_last_name, "Sorted by Last Name");
                break;
            case MENU_SORT_BY_BALANCE:
                sort_and_display(cfptr, compare_by_balance, "Sorted by Balance (Descending)");
                break;
            case MENU_VIEW_TRANSACTIONS:
                puts("\n--- Transaction Statistics ---");
                puts("(Tracked via transaction_count field in records)");
                break;
            default:
                puts("Invalid choice. Please select a valid menu option.");
                break;
        }
    }

    if (fclose(cfptr) != 0) {
        fputs("Warning: failed to close data file cleanly.\n", stderr);
    }

    puts("\nExiting application. Thank you for using Bank Transaction System!");
    return EXIT_SUCCESS;
}

static void initialize_file(FILE *fptr) {
    client_data_t blank = {0, "", "", 0.0, 0};

    rewind(fptr);
    for (unsigned int i = 0; i < MAX_ACCOUNTS; ++i) {
        if (fwrite(&blank, sizeof(client_data_t), 1, fptr) != 1) {
            fputs("Error: failed to initialize data file.\n", stderr);
            break;
        }
    }
    fflush(fptr);
}

static void ensure_data_file_layout(FILE *fptr) {
    const long expected_size = (long)MAX_ACCOUNTS * (long)sizeof(client_data_t);

    if (fseek(fptr, 0L, SEEK_END) != 0) {
        fputs("Warning: unable to verify data file size.\n", stderr);
        rewind(fptr);
        return;
    }

    const long actual_size = ftell(fptr);
    if (actual_size < 0L) {
        fputs("Warning: unable to read data file size.\n", stderr);
        rewind(fptr);
        return;
    }

    if (actual_size != expected_size) {
        printf("Info: rebuilding %s (unexpected file size detected).\n", DATA_FILE);
        initialize_file(fptr);
    }

    rewind(fptr);
}

static void sanitize_file(FILE *fptr) {
    client_data_t client;
    const client_data_t blank = {0, "", "", 0.0, 0};
    unsigned int repaired_count = 0;

    for (unsigned int account = 1; account <= MAX_ACCOUNTS; ++account) {
        if (!read_account(fptr, account, &client)) {
            fputs("Warning: failed to read some records during data sanitization.\n", stderr);
            return;
        }

        if (!is_record_valid_for_slot(&client, account)) {
            if (!write_account(fptr, account, &blank)) {
                fputs("Warning: failed to repair corrupted records.\n", stderr);
                return;
            }
            ++repaired_count;
        }
    }

    if (repaired_count > 0) {
        printf("Info: repaired %u invalid record(s) in %s.\n", repaired_count, DATA_FILE);
    }
}

static void text_file(FILE *read_ptr) {
    client_data_t client;
    FILE *write_ptr = fopen(REPORT_FILE, "w");

    if (write_ptr == NULL) {
        fputs("Error: could not open accounts.txt for writing.\n", stderr);
        return;
    }

    rewind(read_ptr);
    print_header(write_ptr);

    while (fread(&client, sizeof(client_data_t), 1, read_ptr) == 1) {
        if (client.acct_num != 0) {
            print_client(write_ptr, &client);
        }
    }

    if (ferror(read_ptr)) {
        fputs("Warning: read error occurred while generating accounts.txt.\n", stderr);
        clearerr(read_ptr);
    }

    if (fclose(write_ptr) != 0) {
        fputs("Warning: failed to close accounts.txt cleanly.\n", stderr);
    }

    puts("accounts.txt generated successfully.");
}

/* Business Logic: Update account balance (separated from input handling) */
static void update_record_impl(client_data_t *client, double amount) {
    if (!is_balance_valid(client->balance + amount)) {
        printf("Error: Transaction would violate minimum balance limit (%.2f).\n", MIN_BALANCE_LIMIT);
        printf("Current Balance: %.2f | Attempted Transaction: %.2f\n", client->balance, amount);
        printf("Resulting Balance: %.2f (REJECTED)\n", client->balance + amount);
        return;
    }

    client->balance += amount;
    client->transaction_count++;
    printf("Updated Balance: %.2f\n", client->balance);
    printf("Transaction Count: %u\n", client->transaction_count);
}

/* User Input + Business Logic: Update account */
static void update_record(FILE *fptr) {
    unsigned int account = 0;
    double transaction = 0.0;
    client_data_t client;

    if (!prompt_unsigned_in_range("Enter account number to update (1-100): ", 1, MAX_ACCOUNTS, &account)) {
        return;
    }

    if (!read_account(fptr, account, &client)) {
        fputs("Error: failed to read account data.\n", stderr);
        return;
    }

    if (client.acct_num == 0) {
        puts("Account not found.");
        return;
    }

    printf("Current Balance: %.2f\n", client.balance);
    if (!prompt_double_in_range("Enter amount (+deposit / -withdrawal): ", MIN_BALANCE_LIMIT - client.balance, 1e10, &transaction)) {
        return;
    }

    update_record_impl(&client, transaction);

    if (!write_account(fptr, account, &client)) {
        fputs("Error: failed to update account data.\n", stderr);
        return;
    }
}

static void new_record(FILE *fptr) {
    unsigned int account = 0;
    client_data_t client;

    if (!prompt_unsigned_in_range("Enter new account number (1-100): ", 1, MAX_ACCOUNTS, &account)) {
        return;
    }

    if (!read_account(fptr, account, &client)) {
        fputs("Error: failed to read account data.\n", stderr);
        return;
    }

    if (client.acct_num != 0) {
        puts("Account already exists.");
        return;
    }

    memset(&client, 0, sizeof(client));
    client.acct_num = account;

    if (!prompt_word("Enter last name: ", client.last_name, sizeof(client.last_name))) {
        return;
    }
    if (!prompt_word("Enter first name: ", client.first_name, sizeof(client.first_name))) {
        return;
    }
    if (!prompt_double_in_range("Enter opening balance: ", MIN_BALANCE_LIMIT, 1e10, &client.balance)) {
        return;
    }

    client.transaction_count = 0;

    if (!write_account(fptr, account, &client)) {
        fputs("Error: failed to create account.\n", stderr);
        return;
    }

    puts("Account created successfully.");
}

static void delete_record(FILE *fptr) {
    unsigned int account = 0;
    client_data_t client;
    const client_data_t blank = {0, "", "", 0.0, 0};

    if (!prompt_unsigned_in_range("Enter account number to delete (1-100): ", 1, MAX_ACCOUNTS, &account)) {
        return;
    }

    if (!read_account(fptr, account, &client)) {
        fputs("Error: failed to read account data.\n", stderr);
        return;
    }

    if (client.acct_num == 0) {
        puts("Account does not exist.");
        return;
    }

    if (!write_account(fptr, account, &blank)) {
        fputs("Error: failed to delete account.\n", stderr);
        return;
    }

    puts("Account deleted successfully.");
}

static void list_all(FILE *fptr) {
    client_data_t client;
    unsigned int count = 0;

    rewind(fptr);
    puts("\n--- Account List ---");
    print_header(stdout);

    while (fread(&client, sizeof(client_data_t), 1, fptr) == 1) {
        if (client.acct_num != 0) {
            print_client(stdout, &client);
            ++count;
        }
    }

    if (ferror(fptr)) {
        fputs("Warning: read error occurred while listing accounts.\n", stderr);
        clearerr(fptr);
    }

    if (count == 0) {
        puts("No active accounts found.");
    } else {
        printf("Total active accounts: %u\n", count);
    }
}

static void search_account(FILE *fptr) {
    unsigned int account = 0;
    client_data_t client;

    if (!prompt_unsigned_in_range("Enter account number to search (1-100): ", 1, MAX_ACCOUNTS, &account)) {
        return;
    }

    if (!read_account(fptr, account, &client)) {
        fputs("Error: failed to read account data.\n", stderr);
        return;
    }

    if (client.acct_num == 0) {
        puts("Account not found.");
        return;
    }

    puts("Account found:");
    print_header(stdout);
    print_client(stdout, &client);
}

static void sort_and_display(FILE *fptr, sort_comparator cmp, const char *title) {
    client_data_t accounts[MAX_ACCOUNTS];
    unsigned int count = 0;

    rewind(fptr);

    /* Load active accounts into memory (efficient for sorting) */
    for (unsigned int i = 0; i < MAX_ACCOUNTS; ++i) {
        if (fread(&accounts[i], sizeof(client_data_t), 1, fptr) == 1 && accounts[i].acct_num != 0) {
            count++;
        }
    }

    if (count == 0) {
        puts("No active accounts to sort.");
        return;
    }

    /* Compact array (remove blanks) */
    client_data_t *active = malloc(count * sizeof(client_data_t));
    if (active == NULL) {
        fputs("Error: memory allocation failed for sorting.\n", stderr);
        return;
    }

    unsigned int idx = 0;
    rewind(fptr);
    for (unsigned int i = 0; i < MAX_ACCOUNTS; ++i) {
        client_data_t temp;
        if (fread(&temp, sizeof(client_data_t), 1, fptr) == 1 && temp.acct_num != 0) {
            active[idx++] = temp;
        }
    }

    /* Sort using qsort */
    qsort(active, count, sizeof(client_data_t), cmp);

    /* Display sorted results */
    printf("\n--- %s ---\n", title);
    print_header(stdout);
    for (unsigned int i = 0; i < count; ++i) {
        print_client(stdout, &active[i]);
    }

    free(active);
}

static unsigned int enter_choice(void) {
    unsigned int choice = 0;

    puts("\n========== MENU ==========");
    puts("1  - Create text report (accounts.txt)");
    puts("2  - Update account balance");
    puts("3  - Add new account");
    puts("4  - Delete account");
    puts("5  - Exit");
    puts("6  - List all accounts");
    puts("7  - Search account");
    puts("8  - Sort by Last Name");
    puts("9  - Sort by Balance (Highest First)");
    puts("10 - View Transaction Statistics");
    puts("==========================");

    if (!prompt_unsigned_in_range("Enter choice: ", 1, 10, &choice)) {
        return 0;
    }

    return choice;
}

static int read_line(char *buffer, size_t size) {
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    buffer[strcspn(buffer, "\n")] = '\0';
    return 1;
}

static int prompt_unsigned_in_range(const char *prompt, unsigned int min, unsigned int max, unsigned int *value) {
    char buffer[INPUT_BUFFER_SIZE];
    char *end_ptr = NULL;
    unsigned long parsed = 0;

    for (;;) {
        printf("%s", prompt);
        if (!read_line(buffer, sizeof(buffer))) {
            fputs("Input error.\n", stderr);
            return 0;
        }

        errno = 0;
        parsed = strtoul(buffer, &end_ptr, 10);

        if (end_ptr == buffer || *end_ptr != '\0' || errno != 0 || parsed > UINT_MAX) {
            puts("Invalid input. Please enter a number.");
            continue;
        }

        if (parsed < min || parsed > max) {
            printf("Please enter a number between %u and %u.\n", min, max);
            continue;
        }

        *value = (unsigned int)parsed;
        return 1;
    }
}

static int prompt_double_in_range(const char *prompt, double min, double max, double *value) {
    char buffer[INPUT_BUFFER_SIZE];
    char *end_ptr = NULL;
    double parsed = 0.0;

    for (;;) {
        printf("%s", prompt);
        if (!read_line(buffer, sizeof(buffer))) {
            fputs("Input error.\n", stderr);
            return 0;
        }

        errno = 0;
        parsed = strtod(buffer, &end_ptr);

        if (end_ptr == buffer || *end_ptr != '\0' || errno != 0) {
            puts("Invalid input. Please enter a valid amount.");
            continue;
        }

        if (parsed < min || parsed > max) {
            printf("Please enter an amount between %.2f and %.2f.\n", min, max);
            continue;
        }

        *value = parsed;
        return 1;
    }
}

static int prompt_word(const char *prompt, char *dest, size_t dest_size) {
    char buffer[INPUT_BUFFER_SIZE];

    for (;;) {
        printf("%s", prompt);
        if (!read_line(buffer, sizeof(buffer))) {
            fputs("Input error.\n", stderr);
            return 0;
        }

        if (buffer[0] == '\0') {
            puts("Input cannot be empty.");
            continue;
        }

        if (strchr(buffer, ' ') != NULL) {
            puts("Please enter a single word (no spaces).");
            continue;
        }

        if (strlen(buffer) >= dest_size) {
            printf("Input too long. Maximum allowed length is %zu characters.\n", dest_size - 1U);
            continue;
        }

        strcpy(dest, buffer);
        return 1;
    }
}

static long account_offset(unsigned int account_number) {
    return (long)(account_number - 1U) * (long)sizeof(client_data_t);
}

static int read_account(FILE *fptr, unsigned int account_number, client_data_t *client) {
    if (fseek(fptr, account_offset(account_number), SEEK_SET) != 0) {
        return 0;
    }

    return fread(client, sizeof(client_data_t), 1, fptr) == 1;
}

static int write_account(FILE *fptr, unsigned int account_number, const client_data_t *client) {
    if (fseek(fptr, account_offset(account_number), SEEK_SET) != 0) {
        return 0;
    }

    if (fwrite(client, sizeof(client_data_t), 1, fptr) != 1) {
        return 0;
    }

    return fflush(fptr) == 0;
}

static int is_record_valid_for_slot(const client_data_t *client, unsigned int account_number) {
    if (client->acct_num == 0U) {
        return 1;
    }

    return client->acct_num == account_number;
}

static int is_balance_valid(double balance) {
    return balance >= MIN_BALANCE_LIMIT;
}

static void print_header(FILE *out) {
    fprintf(out, "%-8s%-16s%-12s%12s%15s\n", "Acct", "Last Name", "First Name", "Balance", "Txns");
}

static void print_client(FILE *out, const client_data_t *client) {
    fprintf(out, "%-8u%-16s%-12s%12.2f%15u\n",
            client->acct_num,
            client->last_name,
            client->first_name,
            client->balance,
            client->transaction_count);
}

/* Comparator for sorting by last name (ascending) */
static int compare_by_last_name(const void *a, const void *b) {
    const client_data_t *client_a = (const client_data_t *)a;
    const client_data_t *client_b = (const client_data_t *)b;
    return strcmp(client_a->last_name, client_b->last_name);
}

/* Comparator for sorting by balance (descending - highest first) */
static int compare_by_balance(const void *a, const void *b) {
    const client_data_t *client_a = (const client_data_t *)a;
    const client_data_t *client_b = (const client_data_t *)b;

    if (client_b->balance > client_a->balance) return 1;
    if (client_b->balance < client_a->balance) return -1;
    return 0;
}