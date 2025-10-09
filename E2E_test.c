#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define DUP _dup
#define DUP2 _dup2
#define CLOSE _close
#define FILENO _fileno
#else
#include <unistd.h>
#define DUP dup
#define DUP2 dup2
#define CLOSE close
#define FILENO fileno
#endif

#define E2E_TMP_PREFIX "job_e2e"
#define E2E_LINE_LEN   512

typedef struct Store Store;
void clear_screen(void);
void pause_console(void);
void display_menu(void);
void load_csv(Store *st);
void add_applicant(Store *st);
void search_applicant(Store *st);
void update_applicant(Store *st);
void delete_applicant(Store *st);
void DisplayAll(const Store *st);
void run_unit_tests(Store *st);

Store *unit_test_create_store(void);
void unit_test_destroy_store(Store *st);
void unit_test_clear_store(Store *st);
int unit_test_get_store_count(const Store *st);
void unit_test_extract_applicant(const Store *st,
                                 int index,
                                 char *name,
                                 size_t name_len,
                                 char *position,
                                 size_t position_len,
                                 char *email,
                                 size_t email_len,
                                 char *phone,
                                 size_t phone_len);
const char *unit_test_get_csv_path(void);

typedef struct {
    int fd;
    FILE *stream;
} StreamBackup;



static void print_divider(const char *title) {
    puts("--------------------------------------------------------------------");
    if (title) {
        puts(title);
        puts("--------------------------------------------------------------------");
    }
}

static void print_step(int index, const char *text) {
    printf("Step %d: %s\n", index, text);
}

static bool log_check(bool condition, const char *message) {
    printf("  [%s] %s\n", condition ? "OK " : "FAIL", message);
    return condition;
}

static void init_backup(StreamBackup *backup) {
    if (!backup) return;
    backup->fd = -1;
    backup->stream = NULL;
}

static int redirect_stream(FILE *stream, const char *path, const char *mode, StreamBackup *backup) {
    if (!stream || !path || !mode || !backup) return -1;
    init_backup(backup);
    int fd = DUP(FILENO(stream));
    if (fd == -1) return -1;
    if (!freopen(path, mode, stream)) {
        CLOSE(fd);
        return -1;
    }
    (void)setvbuf(stream, NULL, _IONBF, 0);
    backup->fd = fd;
    backup->stream = stream;
    return 0;
}

static void restore_stream(StreamBackup *backup) {
    if (!backup || backup->fd == -1 || !backup->stream) return;
    if (backup->stream != stdin) fflush(backup->stream);
    DUP2(backup->fd, FILENO(backup->stream));
    CLOSE(backup->fd);
    clearerr(backup->stream);
    init_backup(backup);
}

static bool make_temp_path(char *buffer, size_t len) {
    if (!buffer || len == 0) return false;
#ifdef _WIN32
    char temp_dir[MAX_PATH];
    DWORD dir_len = GetTempPathA((DWORD)sizeof(temp_dir), temp_dir);
    if (dir_len == 0 || dir_len > sizeof(temp_dir)) return false;

    char template_buf[MAX_PATH];
    if (GetTempFileNameA(temp_dir, E2E_TMP_PREFIX, 0, template_buf) == 0) return false;

    size_t needed = strlen(template_buf) + 1;
    if (needed > len) {
        remove(template_buf);
        return false;
    }
    strncpy(buffer, template_buf, len - 1);
    buffer[len - 1] = '\0';
#else
    char template_buf[] = "/tmp/job_e2eXXXXXX";
    int fd = mkstemp(template_buf);
    if (fd == -1) return false;
    close(fd);

    size_t needed = strlen(template_buf) + 1;
    if (needed > len) {
        remove(template_buf);
        return false;
    }
    strncpy(buffer, template_buf, len - 1);
    buffer[len - 1] = '\0';
#endif
    return true;
}

static bool write_text_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) return false;
    if (!content) content = "";
    if (fputs(content, f) == EOF) {
        fclose(f);
        return false;
    }
    fclose(f);
    return true;
}

static bool read_text_file(const char *path, char **out_buf, size_t *out_len) {
    if (out_buf) *out_buf = NULL;
    if (out_len) *out_len = 0;

    FILE *f = fopen(path, "rb");
    if (!f) return false;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return false;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return false;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return false;
    }
    char *buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(f);
        return false;
    }
    size_t read = fread(buffer, 1, (size_t)size, f);
    fclose(f);
    buffer[read] = '\0';
    if (out_len) *out_len = read;
    if (out_buf) *out_buf = buffer;
    else free(buffer);
    return true;
}

static bool copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return false;
    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return false;
    }
    char buf[1024];
    size_t n;
    bool ok = true;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            ok = false;
            break;
        }
    }
    if (ferror(in)) ok = false;
    fclose(in);
    fclose(out);
    return ok;
}

static bool string_contains(const char *haystack, const char *needle) {
    if (!haystack || !needle) return false;
    return strstr(haystack, needle) != NULL;
}

static void remove_file_if_exists(const char *path) {
    if (path) remove(path);
}

static bool file_exists(const char *path) {
    if (!path || !*path) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

static bool restore_from_backup(const char *backup_path) {
    if (!backup_path || backup_path[0] == '\0') return false;
    const char *csv_path = unit_test_get_csv_path();
    return copy_file(backup_path, csv_path);
}

static void trim_line(char *s) {
    if (!s) return;
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
}

static size_t count_lines(const char *text) {
    if (!text) return 0;
    size_t lines = 0;
    for (const char *p = text; *p; ++p) {
        if (*p == '\n') lines++;
    }
    if (*text && text[strlen(text) - 1] != '\n') lines++;
    return lines;
}

static void e2e_cli_driver(Store *st) {
    if (!st) return;

    clear_screen();
    load_csv(st);

    for (int iteration = 0; iteration < 512; ++iteration) {
        display_menu();

        char input[E2E_LINE_LEN];
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) {
            puts("==================");
            puts("Please choose 1-8");
            puts("==================");
            pause_console();
            clear_screen();
            continue;
        }

        trim_line(input);
        if (input[0] == '\0') {
            puts("==================================");
            puts("Please enter a non-empty keyword.");
            puts("==================================");
            pause_console();
            clear_screen();
            continue;
        }

        char *endptr = NULL;
        long value = strtol(input, &endptr, 10);
        if (*endptr != '\0' || endptr == input) {
            puts("==================");
            puts("Please choose 1-8");
            puts("==================");
            pause_console();
            clear_screen();
            continue;
        }

        int choice = (int)value;
        switch (choice) {
            case 1:
                add_applicant(st);
                break;
            case 2:
                search_applicant(st);
                break;
            case 3:
                update_applicant(st);
                break;
            case 4:
                delete_applicant(st);
                break;
            case 5:
                DisplayAll(st);
                break;
            case 6:
                run_unit_tests(st);
                clear_screen();
                break;
            case 7:
                puts("==================================");
                puts("Nested E2E invocation is not supported.");
                puts("==================================");
                pause_console();
                clear_screen();
                break;
            case 8:
                puts("Bye!");
                return;
            default:
                puts("==================");
                puts("Please choose 1-8");
                puts("==================");
                pause_console();
                clear_screen();
                break;
        }
    }

    puts("==================================");
    puts("E2E driver stopped (script exceeded iteration limit).");
    puts("==================================");
}

static bool run_cli_script(Store *st, const char *script, char **captured_output) {
    if (captured_output) *captured_output = NULL;
    if (!st || !script) return false;

    char input_path[260] = {0};
    char output_path[260] = {0};
    if (!make_temp_path(input_path, sizeof(input_path))) return false;
    if (!make_temp_path(output_path, sizeof(output_path))) {
        remove_file_if_exists(input_path);
        return false;
    }

    if (!write_text_file(input_path, script)) {
        remove_file_if_exists(input_path);
        remove_file_if_exists(output_path);
        return false;
    }

    StreamBackup stdin_backup;
    StreamBackup stdout_backup;
    init_backup(&stdin_backup);
    init_backup(&stdout_backup);

    bool ok = true;
    if (redirect_stream(stdin, input_path, "r", &stdin_backup) != 0) ok = false;
    if (ok && redirect_stream(stdout, output_path, "w", &stdout_backup) != 0) ok = false;

    if (ok) e2e_cli_driver(st);

    restore_stream(&stdout_backup);
    restore_stream(&stdin_backup);

    if (ok && captured_output) {
        if (!read_text_file(output_path, captured_output, NULL)) {
            ok = false;
        }
    }

    remove_file_if_exists(input_path);
    remove_file_if_exists(output_path);
    return ok;
}

static bool scenario_add_and_review(const char *backup_path) {
    print_divider("          E2E Scenario 1: Register and Review Applicants        ");
    puts("Goal: simulate a hiring clerk adding multiple applicants, listing them, and verifying search results.");

    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) {
        puts("  [FAIL] Unable to prepare CSV backup.");
        return false;
    }

    if (!write_text_file(csv_path, "ApplicantName,JobPosition,Email,PhoneNumber\n")) {
        puts("  [FAIL] Unable to reset CSV content.");
        return false;
    }

    Store *st = unit_test_create_store();
    if (!st) return false;
    unit_test_clear_store(st);

    puts("\nWorkflow outline:");
    print_step(1, "Add three applicants using realistic input.");
    print_step(2, "Display every applicant to confirm the list.");
    print_step(3, "Search for one applicant to ensure lookups work.");
    print_step(4, "Exit the program gracefully.");

    const char *script =
        "1\n"  "Rina\n"  "\n"  "Watts\n"  "1\n"  "rina@example.com\n"  "0888888888\n"  "\n"
        // ผู้สมัครคนที่ 2
        "1\n"  "Marco\n" "\n"  "Silva\n"  "2\n"  "marco@example.com\n" "0777777777\n" "\n"
        // ผู้สมัครคนที่ 3
        "1\n"  "Ella\n"  "\n"  "Stone\n"  "5\n"  "ella@example.com\n"  "0666666666\n"  "\n"
        // ลำดับการแสดงผลและค้นหา
        "5\n"  "\n"
        "2\n"  "Rina\n" "\n"
        "8\n";

    puts("\nExecuting script...");
    char *session_output = NULL;
    bool run_ok = run_cli_script(st, script, &session_output);
    bool pass = true;
    pass &= log_check(run_ok, "CLI ran without redirection issues.");

    const int expected = 3;
    int actual = unit_test_get_store_count(st);
    pass &= log_check(actual == expected, "Store contains three applicants.");

    const char *names[]     = {"Rina Watts", "Marco Silva", "Ella Stone"};
    const char *positions[] = {"Developer", "Marketing", "HR Specialist"};
    const char *emails[]    = {"rina@example.com", "marco@example.com", "ella@example.com"};
    const char *phones[]    = {"0888888888", "0777777777", "0666666666"};

    if (actual == expected) {
        for (int i = 0; i < expected; ++i) {
            char name[128] = {0}, position[128] = {0}, email[128] = {0}, phone[128] = {0};
            unit_test_extract_applicant(st, i, name, sizeof(name), position, sizeof(position),
                                        email, sizeof(email), phone, sizeof(phone));
            char label[80];
            snprintf(label, sizeof(label), "Applicant %d fields match expected data.", i + 1);
            bool row_ok = strcmp(name, names[i]) == 0 &&
                          strcmp(position, positions[i]) == 0 &&
                          strcmp(email, emails[i]) == 0 &&
                          strcmp(phone, phones[i]) == 0;
            pass &= log_check(row_ok, label);
        }
    }

    char *csv_content = NULL;
    if (!read_text_file(csv_path, &csv_content, NULL)) {
        pass &= log_check(false, "CSV content could not be read.");
    } else {
        pass &= log_check(string_contains(csv_content, "Rina Watts,Developer,rina@example.com,0888888888"),
                          "CSV entry exists for Rina.");
        pass &= log_check(string_contains(csv_content, "Marco Silva,Marketing,marco@example.com,0777777777"),
                          "CSV entry exists for Marco.");
        pass &= log_check(string_contains(csv_content, "Ella Stone,HR Specialist,ella@example.com,0666666666"),
                          "CSV entry exists for Ella.");
        pass &= log_check(count_lines(csv_content) == 4,
                          "CSV line count equals 4 (header + 3 rows).");
        free(csv_content);
    }

    if (session_output) {
        pass &= log_check(string_contains(session_output, "Added and saved."),
                          "Console feedback shows successful additions.");
        pass &= log_check(string_contains(session_output, "=== Applicants (3) ==="),
                          "Display screen lists the three applicants.");
        pass &= log_check(string_contains(session_output, "Rina Watts"),
                          "Search output includes the requested applicant.");
    } else {
        pass &= log_check(false, "Captured console output is available.");
    }

    free(session_output);
    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) {
        pass &= log_check(false, "Original CSV restored after scenario.");
    }

    puts(pass ? "\nScenario result: SUCCESS" : "\nScenario result: FAILURE");
    return pass;
}

static bool scenario_update_and_cleanup(const char *backup_path) {
    print_divider("          E2E Scenario 2: Update Applicant then Cleanup        ");
    puts("Goal: confirm that editing every field and removing a record leaves the system clean.");

    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) {
        puts("  [FAIL] Unable to prepare CSV backup.");
        return false;
    }
    if (!write_text_file(csv_path, "ApplicantName,JobPosition,Email,PhoneNumber\n")) {
        puts("  [FAIL] Unable to reset CSV content.");
        return false;
    }

    Store *st = unit_test_create_store();
    if (!st) return false;
    unit_test_clear_store(st);

    puts("\nWorkflow outline:");
    print_step(1, "Add a new Sales applicant.");
    print_step(2, "Update the applicant with new name, role, email, and phone.");
    print_step(3, "Delete the updated applicant after confirmation.");
    print_step(4, "Exit the program.");

    // ลำดับเหตุการณ์: เพิ่ม -> อัปเดตทุกช่อง -> ลบ -> ออกจากโปรแกรม
    const char *script =
        "1\n"
        "Liam\n"
        "\n"
        "Carter\n"
        "3\n"
        "liam@example.com\n"
        "0999888777\n"
        "\n"
        "3\n"
        "Liam\n"
        "1\n"
        "5\n"
        "William\n"
        "\n"
        "Johnson\n"
        "4\n"
        "william@example.com\n"
        "0123401234\n"
        "\n"
        "4\n"
        "William\n"
        "1\n"
        "y\n"
        "\n"
        "8\n";

    puts("\nExecuting script...");
    char *session_output = NULL;
    bool run_ok = run_cli_script(st, script, &session_output);
    bool pass = true;
    pass &= log_check(run_ok, "CLI ran without redirection issues.");

    pass &= log_check(unit_test_get_store_count(st) == 0,
                      "Store is empty after deletion.");

    char *csv_content = NULL;
    if (!read_text_file(csv_path, &csv_content, NULL)) {
        pass &= log_check(false, "CSV content could not be read.");
    } else {
        bool has_header = string_contains(csv_content, "ApplicantName,JobPosition,Email,PhoneNumber");
        bool single_line = count_lines(csv_content) == 1;
        bool no_record = !string_contains(csv_content, "William Johnson");
        pass &= log_check(has_header && single_line && no_record,
                          "CSV contains only the header after cleanup.");
        free(csv_content);
    }

    if (session_output) {
        pass &= log_check(string_contains(session_output, "Added and saved."),
                          "Addition feedback shown.");
        pass &= log_check(string_contains(session_output, "Update and saved."),
                          "Update feedback shown.");
        pass &= log_check(string_contains(session_output, "Deleted and saved."),
                          "Deletion feedback shown.");
        pass &= log_check(string_contains(session_output, "William Johnson"),
                          "Updated name surfaced in the console output.");
    } else {
        pass &= log_check(false, "Captured console output is available.");
    }

    free(session_output);
    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) {
        pass &= log_check(false, "Original CSV restored after scenario.");
    }

    puts(pass ? "\nScenario result: SUCCESS" : "\nScenario result: FAILURE");
    return pass;
}

void run_e2e_tests(Store *shared_store) {
    if (!shared_store) return;

    clear_screen();
    print_divider("                     End-to-End Test Suite                   ");
    puts("  This suite drives job.c exactly how a user would:");
    puts("      - It redirects stdin/stdout to scripted inputs.");
    puts("      - It verifies Store data and CSV persistence after each flow.");
    puts("      - It prints clear pass/fail results for every validation.");

    const char *csv_path = unit_test_get_csv_path();
    char backup_path[512];
    int written = snprintf(backup_path, sizeof(backup_path), "%s.e2e.bak", csv_path);
    if (written < 0 || (size_t)written >= sizeof(backup_path)) {
        puts("Cannot generate backup path. Aborting E2E suite.");
        pause_console();
        return;
    }

    if (!copy_file(csv_path, backup_path)) {
        puts("Cannot create backup of applicants.csv. Aborting E2E suite.");
        pause_console();
        return;
    }

    struct {
        const char *label;
        bool (*fn)(const char *);
    } scenarios[] = {
        {"Register and Review", scenario_add_and_review},
        {"Update and Cleanup",  scenario_update_and_cleanup},
    };
    const size_t scenario_count = sizeof(scenarios) / sizeof(scenarios[0]);

    int total = 0;
    int passed = 0;
    bool abort_remaining = false;

    for (size_t i = 0; i < scenario_count; ++i) {
        if (abort_remaining) {
            printf("[SKIP] %s (prior I/O failure)\n", scenarios[i].label);
            continue;
        }
        bool ok = scenarios[i].fn(backup_path);
        printf("[%s] %s\n", ok ? "PASS" : "FAIL", scenarios[i].label);
        total++;
        if (ok) {
            passed++;
        } else if (!file_exists(backup_path) || !file_exists(csv_path)) {
            puts("Critical failure detected. Stopping remaining scenarios.");
            abort_remaining = true;
        }
    }

    bool restored = restore_from_backup(backup_path);
    if (restored) {
        remove_file_if_exists(backup_path);
        load_csv(shared_store);
    } else {
        printf("Warning: original applicants.csv not restored. Backup kept at %s\n", backup_path);
    }
    puts("------------------------------------------------------------");
    printf("              Summary: %d/%d scenarios passed.\n", passed, total);
    puts("------------------------------------------------------------");
    fflush(stdout);
    pause_console();
}
