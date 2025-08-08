/****************************************************************************
*                                                                           *
*  sh2_lazy_lord -  An assistant for the lazy lord                          *
*  Control gold, taxes, and resident satisfaction                           *
*                                                                           *
*  Break free from limits and be the lord you are destined to be.           *
*                                                                           *
* made with <3 in germany                                                   *
*****************************************************************************/

#define VERSION "1.0.0"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <conio.h>
#include <string.h>

#define MODULE_NAME "Stronghold2.exe"
//#define SP_GOLD_BASE_OFFSET 0x6E8BDC
#define GOLD_BASE_OFFSET 0x6E8C60
#define GOLD_POINTER_OFFSET 0x1010
//#define SP_HONOR_BASE_OFFSET 0x6E8BDC
#define HONOR_BASE_OFFSET 0x6E8C60
#define HONOR_POINTER_OFFSET 0x1C
//#define SP_HAPPINESS_BASE_OFFSET 0x6E8BDC
#define HAPPINESS_BASE_OFFSET 0x6E8C60
#define HAPPINESS_POINTER_OFFSET 0x1028

// Resource types
#define RESOURCE_GOLD 1
#define RESOURCE_HONOR 2
#define RESOURCE_HAPPINESS 3

// Colors for console output
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD    "\033[1m"

// Function prototypes
void print_ascii_art(void);
void print_welcome(void);
long find_stronghold_pid(const char* module_name);
long get_base_address(long shpid);
void write_gold_value(long shpid, long address, float new_value);
float read_gold_value(long shpid, long address);
void write_honor_value(long shpid, long address, int new_value);
int read_honor_value(long shpid, long address);
void write_happiness_value(long shpid, long address, float new_value);
float read_happiness_value(long shpid, long address);
void clear_screen(void);
void set_console_colors(void);
float get_user_gold_input(void);
int get_user_honor_input(void);
void print_loading_animation(const char* message);
void print_separator(void);
int show_resource_menu(void);
void handle_gold_modification(long shpid);
void handle_honor_modification(long shpid);
void handle_happiness_modification(long shpid);

int main(void)
{
    long shpid = 0;
    int user_choice = 0;
    int continue_choice = 0;

    // Enable console colors on Windows
    set_console_colors();

    clear_screen();
    print_ascii_art();
    print_welcome();

    while (1) {
        print_separator();
        printf(CYAN BOLD "[*] Searching for Stronghold 2 process...\n" RESET);

        // Wait for Stronghold 2 process
        while (!(shpid = find_stronghold_pid(MODULE_NAME))) {
            printf(YELLOW "[!] Stronghold 2 not found. Start the game and press any key to search again..." RESET);
            _getch();
            printf("\r" CYAN "[*] Search again..." RESET);
            print_loading_animation("Looking for process");
        }

        printf(GREEN "[+] Stronghold 2 found! (PID: %ld)\n" RESET, shpid);
        Sleep(500);

        // Show resource selection menu
        user_choice = show_resource_menu();

        switch (user_choice) {
            case RESOURCE_GOLD:
                handle_gold_modification(shpid);
                break;
            case RESOURCE_HONOR:
                handle_honor_modification(shpid);
                break;
            case RESOURCE_HAPPINESS:
                handle_happiness_modification(shpid);
                break;
            default:
                printf(RED "[-] Invalid selection!\n" RESET);
                continue;
        }

        print_separator();
        printf(MAGENTA BOLD "Would you like to make another modification? (y/n): " RESET);
        continue_choice = _getch();
        printf("%c\n", (char)continue_choice);

        if (continue_choice != 'y' && continue_choice != 'Y') {
            break;
        }

        clear_screen();
        printf(CYAN BOLD "[~] Ready for the next modification!\n\n" RESET);
    }

    printf(GREEN BOLD "\n[+] Thank you for using Stronghold 2 Lazy Lord!\n" RESET);
    printf("Press any key to exit...");
    _getch();
    return EXIT_SUCCESS;
}

int show_resource_menu(void)
{
    int choice = 0;
    
    clear_screen();
    printf(YELLOW BOLD "================================================================================\n");
    printf("||                          RESOURCE SELECTION MENU                           ||\n");
    printf("================================================================================\n");
    printf("||                                                                            ||\n");
    printf("||  [1] GOLD OPTIONEN          - Manage your gold reserves                    ||\n");
    printf("||                                                                            ||\n");
    printf("||  [2] HONOR OPTIONEN         - Manage your honor                            ||\n");
    printf("||                                                                            ||\n");
    printf("||  [3] CITIZEN SATISFACTION   - Restore satisfaction                         ||\n");
    printf("||                                                                            ||\n");
    printf("================================================================================\n" RESET);
    
    printf(CYAN BOLD "\n[>] Select your option (1-3): " RESET);
    
    while (1) {
        choice = _getch();
        printf("%c\n", (char)choice);
        
        if (choice == '1') {
            return RESOURCE_GOLD;
        } else if (choice == '2') {
            return RESOURCE_HONOR;
        } else if (choice == '3') {
            return RESOURCE_HAPPINESS;
        } else {
            printf(RED "[-] Invalid entry! Please select 1, 2, or 3: " RESET);
        }
    }
}

void handle_happiness_modification(long shpid)
{
    long happiness_ptr_addr = 0;
    long happiness_addr = 0;
    float current_happiness = 0.0f;
    float new_happiness = 100.0f;

    printf(CYAN "[*] Determine citizen satisfaction memory address...\n" RESET);
    happiness_ptr_addr = get_base_address(shpid) + HAPPINESS_BASE_OFFSET;

    // Read the pointer to get the actual happiness address
    HANDLE proc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
        FALSE, shpid);
    if (!proc) {
        printf(RED "[-] Error opening the process handle\n" RESET);
        return;
    }

    SIZE_T bytes_read = 0;
    long happiness_base = 0;
    if (!ReadProcessMemory(proc, (void*)(uintptr_t)happiness_ptr_addr,
        &happiness_base, sizeof(happiness_base), &bytes_read) ||
        bytes_read != sizeof(happiness_base)) {
        printf(RED "[-] Error reading the satisfaction pointer\n" RESET);
        CloseHandle(proc);
        return;
    }

    happiness_addr = happiness_base + HAPPINESS_POINTER_OFFSET;
    CloseHandle(proc);

    // Read current happiness value
    current_happiness = read_happiness_value(shpid, happiness_addr);

    printf(GREEN "[+] Satisfaction address found!\n" RESET);
    printf(YELLOW "[!] Current citizen satisfaction: %.2f\n" RESET, current_happiness);

    printf(CYAN "[*] Set citizen satisfaction to %.2f...\n" RESET, new_happiness);
    print_loading_animation("Citizen satisfaction is restored");

    write_happiness_value(shpid, happiness_addr, new_happiness);

    // Verify the change
    current_happiness = read_happiness_value(shpid, happiness_addr);
    printf(GREEN "[+] Citizen satisfaction successfully restored!\n" RESET);
    printf(YELLOW "[!] New citizen satisfaction: %.2f\n" RESET, current_happiness);
}

void handle_gold_modification(long shpid)
{
    long gold_ptr_addr = 0;
    long gold_addr = 0;
    float current_gold = 0.0f;
    float new_gold = 0.0f;

    printf(CYAN "[*] Determine gold memory address...\n" RESET);
    gold_ptr_addr = get_base_address(shpid) + GOLD_BASE_OFFSET;

    // Read the pointer to get the actual gold address
    HANDLE proc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
        FALSE, shpid);
    if (!proc) {
        printf(RED "[-] Error opening the process handle\n" RESET);
        return;
    }

    SIZE_T bytes_read = 0;
    long gold_base = 0;
    if (!ReadProcessMemory(proc, (void*)(uintptr_t)gold_ptr_addr,
        &gold_base, sizeof(gold_base), &bytes_read) ||
        bytes_read != sizeof(gold_base)) {
        printf(RED "[-] Error reading the gold pointer\n" RESET);
        CloseHandle(proc);
        return;
    }

    gold_addr = gold_base + GOLD_POINTER_OFFSET;
    CloseHandle(proc);

    // Read current gold value
    current_gold = read_gold_value(shpid, gold_addr);

    printf(GREEN "[+] Gold address found!\n" RESET);
    printf(YELLOW "[$] Current gold amount: %.2f\n" RESET, current_gold);

    // Get user input for new gold amount
    new_gold = get_user_gold_input();

    if (new_gold < 0) {
        printf(YELLOW "[~] Gold modification canceled.\n" RESET);
    }
    else {
        printf(CYAN "[*] Set gold value to %.2f...\n" RESET, new_gold);
        print_loading_animation("Gold value is updating");

        write_gold_value(shpid, gold_addr, new_gold);

        // Verify the change
        current_gold = read_gold_value(shpid, gold_addr);
        printf(GREEN "[+] Gold successfully changed!\n" RESET);
        printf(YELLOW "[$] New gold amount: %.2f\n" RESET, current_gold);
    }
}

void handle_honor_modification(long shpid)
{
    long honor_ptr_addr = 0;
    long honor_addr = 0;
    int current_honor = 0;
    int new_honor = 0;

    printf(CYAN "[*] Determine honor memory address...\n" RESET);
    honor_ptr_addr = get_base_address(shpid) + HONOR_BASE_OFFSET;

    // Read the pointer to get the actual honor address
    HANDLE proc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
        FALSE, shpid);
    if (!proc) {
        printf(RED "[-] Error opening the process handle\n" RESET);
        return;
    }

    SIZE_T bytes_read = 0;
    long honor_base = 0;
    if (!ReadProcessMemory(proc, (void*)(uintptr_t)honor_ptr_addr,
        &honor_base, sizeof(honor_base), &bytes_read) ||
        bytes_read != sizeof(honor_base)) {
        printf(RED "[-] Error reading the honor pointer\n" RESET);
        CloseHandle(proc);
        return;
    }

    honor_addr = honor_base + HONOR_POINTER_OFFSET;
    CloseHandle(proc);

    // Read current honor value
    current_honor = read_honor_value(shpid, honor_addr);

    printf(GREEN "[+] Honor address found!\n" RESET);
    printf(YELLOW "[#] Current honor: %d\n" RESET, current_honor);

    // Get user input for new honor amount
    new_honor = get_user_honor_input();

    if (new_honor < 0) {
        printf(YELLOW "[~] Honor modification canceled.\n" RESET);
    }
    else {
        printf(CYAN "[*] Set honor to %d...\n" RESET, new_honor);
        print_loading_animation("Honor gets changed up");

        write_honor_value(shpid, honor_addr, new_honor);

        // Verify the change
        current_honor = read_honor_value(shpid, honor_addr);
        printf(GREEN "[+] Honor successfully changed!\n" RESET);
        printf(YELLOW "[#] New honor: %d\n" RESET, current_honor);
    }
}

int get_user_honor_input(void)
{
    char input[32];
    int honor_amount = 0;

    while (1) {
        print_separator();
        printf(MAGENTA BOLD "[#] How much honor do you want to have?\n" RESET);
        printf(WHITE "   [!] Popular values: 100, 500, 1000, 5000, 10000\n");
        printf(WHITE "   [!] Enter '0' to cancel\n");
        printf(CYAN "   [>] Requested honor: " RESET);

        if (fgets(input, sizeof(input), stdin) != NULL) {
            // Remove newline character
            input[strcspn(input, "\n")] = 0;

            if (strlen(input) == 0) {
                printf(RED "[-] Please enter a valid amount.!\n" RESET);
                continue;
            }

            honor_amount = atoi(input);

            if (honor_amount == 0 && strcmp(input, "0") == 0) {
                return -1; // User wants to cancel
            }

            if (honor_amount < 0) {
                printf(RED "[-] Negative values are not allowed.!\n" RESET);
                continue;
            }

            if (honor_amount > 999999) {
                printf(YELLOW "[!]  Very high value! Are you sure?? (y/n): " RESET);
                int confirm = _getch();
                printf("%c\n", (char)confirm);
                if (confirm != 'y' && confirm != 'Y') {
                    continue;
                }
            }

            printf(GREEN "[+] Honor amount confirmed: %d\n" RESET, honor_amount);
            return honor_amount;
        }
    }
}

void write_honor_value(long shpid, long address, int new_value)
{
    DWORD access = PROCESS_VM_READ | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_WRITE | PROCESS_VM_OPERATION;
    HANDLE proc;
    SIZE_T bytes_written = 0;

    if (!(proc = OpenProcess(access, FALSE, shpid))) {
        printf(RED "[-] Error opening handle for PID: %ld\n" RESET, shpid);
        exit(EXIT_FAILURE);
    }

    if (!WriteProcessMemory(proc, (void*)(uintptr_t)address,
        &new_value, sizeof(int), &bytes_written) ||
        bytes_written != sizeof(int)) {
        CloseHandle(proc);
        printf(RED "[-] Error writing the honor value for PID: %ld\n" RESET, shpid);
        exit(EXIT_FAILURE);
    }

    CloseHandle(proc);
}

int read_honor_value(long shpid, long address)
{
    DWORD access = PROCESS_VM_READ | PROCESS_QUERY_INFORMATION;
    HANDLE proc;
    SIZE_T bytes_read = 0;
    int value = 0;

    if (!(proc = OpenProcess(access, FALSE, shpid))) {
        printf(RED "[-] Error opening handle for PID: %ld\n" RESET, shpid);
        return 0;
    }

    if (!ReadProcessMemory(proc, (void*)(uintptr_t)address,
        &value, sizeof(int), &bytes_read) ||
        bytes_read != sizeof(int)) {
        CloseHandle(proc);
        printf(RED "[-] Error reading the honor value of PID: %ld\n" RESET, shpid);
        return 0;
    }

    CloseHandle(proc);
    return value;
}

void write_happiness_value(long shpid, long address, float new_value)
{
    DWORD access = PROCESS_VM_READ | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_WRITE | PROCESS_VM_OPERATION;
    HANDLE proc;
    SIZE_T bytes_written = 0;

    if (!(proc = OpenProcess(access, FALSE, shpid))) {
        printf(RED "[-] Error opening handle for PID: %ld\n" RESET, shpid);
        exit(EXIT_FAILURE);
    }

    if (!WriteProcessMemory(proc, (void*)(uintptr_t)address,
        &new_value, sizeof(float), &bytes_written) ||
        bytes_written != sizeof(float)) {
        CloseHandle(proc);
        printf(RED "[-] Error writing satisfaction value for PID: %ld\n" RESET, shpid);
        exit(EXIT_FAILURE);
    }

    CloseHandle(proc);
}

float read_happiness_value(long shpid, long address)
{
    DWORD access = PROCESS_VM_READ | PROCESS_QUERY_INFORMATION;
    HANDLE proc;
    SIZE_T bytes_read = 0;
    float value = 0.0f;

    if (!(proc = OpenProcess(access, FALSE, shpid))) {
        printf(RED "[-] Error opening handle for PID: %ld\n" RESET, shpid);
        return 0.0f;
    }

    if (!ReadProcessMemory(proc, (void*)(uintptr_t)address,
        &value, sizeof(float), &bytes_read) ||
        bytes_read != sizeof(float)) {
        CloseHandle(proc);
        printf(RED "[-] Error reading PID satisfaction value: %ld\n" RESET, shpid);
        return 0.0f;
    }

    CloseHandle(proc);
    return value;
}

void print_ascii_art(void)
{
    printf(YELLOW BOLD);
    printf("    ##           ###      ########  ###    ###           \n");
    printf("    ##          #####          ##     ##  ##             \n");
    printf("    ##         ##   ##       ##        ####              \n");
    printf("    ##        ## ### ##    ##           ##               \n");
    printf("    #######  ##       ##  ########      ##               \n");
    printf("                                                         \n");
    printf("                ##       ######  ########   ########     \n");
    printf("                ##      ##    ##  ##    ##  ##     ##    \n");
    printf("                ##      ##    ##  ## ####   ##     ##    \n");
    printf("                ##      ##    ##  ##   ##   ##     ##    \n");
    printf("                #######  ######  ####   ### ########     \n");
    printf("                                                         \n");
    printf(" *made with <3 in germany*                               \n");
    printf(RESET);
}

void print_welcome(void)
{
    printf(CYAN BOLD "\n    ===============================================================================\n");
    printf("    ||                      [*] STRONGHOLD 2 LAZY LORD [*]                       ||\n");
    printf("    ||                              Version %s                                ||\n", VERSION);
    printf("    |=============================================================================|\n");
    printf("    ||  Welcome, my lord! This tool will help you become a                       ||\n");
    printf("    ||  successful and, above all, lazy lord in Stronghold 2.                    ||\n");
    printf("    ||                                                                           ||\n");
    printf("    ||  Need Gold?             - We fill the treasure chambers.                  ||\n");
    printf("    ||  No Honor?              - All the glory in the blink of an eye.           ||\n");
    printf("    ||  Dissatisfied citizens? - We will make them happy again.                  ||\n");
    printf("    ||                                                                           ||\n");
    printf("    ||  [!] Note: Make sure Stronghold 2 is running!                             ||\n");
    printf("    ===============================================================================\n" RESET);

    printf(GREEN "\n[+] Press any key to start..." RESET);
    _getch();
    clear_screen();
}

void print_separator(void)
{
    printf(BLUE "--------------------------------------------------------------------------------\n" RESET);
}

void print_loading_animation(const char* message)
{
    const char* spinner = "|/-\\";
    printf(YELLOW "%s ", message);
    for (int i = 0; i < 20; i++) {
        printf("\r%s %c", message, spinner[i % 4]);
        fflush(stdout);
        Sleep(100);
    }
    printf("\r%s [OK]\n", message);
}

float get_user_gold_input(void)
{
    char input[32];
    float gold_amount = 0.0f;

    while (1) {
        print_separator();
        printf(MAGENTA BOLD "[$] How much gold would you like to have?\n" RESET);
        printf(WHITE "   [!] Popular values: 100, 500, 1000, 10000\n");
        printf(WHITE "   [!] Enter '0' to cancel\n");
        printf(CYAN "   [>] Requested amount of gold: " RESET);

        if (fgets(input, sizeof(input), stdin) != NULL) {
            // Remove newline character
            input[strcspn(input, "\n")] = 0;

            if (strlen(input) == 0) {
                printf(RED "[-] Please enter a valid amount.!\n" RESET);
                continue;
            }

            gold_amount = (float)atof(input);

            if (gold_amount == 0.0f && strcmp(input, "0") == 0) {
                return -1; // User wants to cancel
            }

            if (gold_amount < 0) {
                printf(RED "[-] Negative values are not allowed.!\n" RESET);
                continue;
            }

            if (gold_amount > 999999.0f) {
                printf(YELLOW "[!] Very high value! Are you sure?? (y/n): " RESET);
                int confirm = _getch();
                printf("%c\n", (char)confirm);
                if (confirm != 'y' && confirm != 'Y') {
                    continue;
                }
            }

            printf(GREEN "[+] Gold amount confirmed: %.2f\n" RESET, gold_amount);
            return gold_amount;
        }
    }
}

void clear_screen(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void set_console_colors(void)
{
#ifdef _WIN32
    // Enable ANSI color codes on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

long find_stronghold_pid(const char* module_name)
{
    HANDLE hsnap;
    PROCESSENTRY32 pe32;
    long pid = 0;

    pe32.dwSize = sizeof(PROCESSENTRY32);
    hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hsnap == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (Process32First(hsnap, &pe32)) {
        do {
            if (strcmp(pe32.szExeFile, module_name) == 0) {
                pid = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(hsnap, &pe32));
    }

    CloseHandle(hsnap);
    return pid;
}

long get_base_address(long shpid)
{
    void* base_addr = NULL;
    HANDLE hsnap;
    MODULEENTRY32 modentry;
    char mname[] = MODULE_NAME;
    int i;

    modentry.dwSize = sizeof(modentry);
    hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, shpid);

    if (hsnap != INVALID_HANDLE_VALUE) {
        if (Module32First(hsnap, &modentry)) {
            do {
                i = 0;
                while ((modentry.szModule[i] != '\0' && mname[i] != '\0') &&
                    modentry.szModule[i] == mname[i])
                    i++;
                if (i && (mname[i] == '\0' && modentry.szModule[i] == '\0')) {
                    base_addr = modentry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hsnap, &modentry));
        }
    }
    else {
        printf(RED "[-] Error determining the base module address.\n" RESET);
        exit(EXIT_FAILURE);
    }

    CloseHandle(hsnap);
    return (long)(uintptr_t)base_addr;
}

void write_gold_value(long shpid, long address, float new_value)
{
    DWORD access = PROCESS_VM_READ | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_WRITE | PROCESS_VM_OPERATION;
    HANDLE proc;
    SIZE_T bytes_written = 0;

    if (!(proc = OpenProcess(access, FALSE, shpid))) {
        printf(RED "[-] Error opening handle for PID: %ld\n" RESET, shpid);
        exit(EXIT_FAILURE);
    }

    if (!WriteProcessMemory(proc, (void*)(uintptr_t)address,
        &new_value, sizeof(float), &bytes_written) ||
        bytes_written != sizeof(float)) {
        CloseHandle(proc);
        printf(RED "[-] Error writing the gold value for PID: %ld\n" RESET, shpid);
        exit(EXIT_FAILURE);
    }

    CloseHandle(proc);
}

float read_gold_value(long shpid, long address)
{
    DWORD access = PROCESS_VM_READ | PROCESS_QUERY_INFORMATION;
    HANDLE proc;
    SIZE_T bytes_read = 0;
    float value = 0.0f;

    if (!(proc = OpenProcess(access, FALSE, shpid))) {
        printf(RED "[-] Error opening handle for PID: %ld\n" RESET, shpid);
        return 0.0f;
    }

    if (!ReadProcessMemory(proc, (void*)(uintptr_t)address,
        &value, sizeof(float), &bytes_read) ||
        bytes_read != sizeof(float)) {
        CloseHandle(proc);
        printf(RED "[-] Error reading the gold value of PID: %ld\n" RESET, shpid);
        return 0.0f;
    }

    CloseHandle(proc);
    return value;
}