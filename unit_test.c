#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

typedef struct Store Store;

void add_applicant(Store *st);
void search_applicant(Store *st);
void load_csv(Store *st);
void clear_screen(void);
void pause(void);

Store *unit_test_create_store(void);
void unit_test_destroy_store(Store *st);
void unit_test_clear_store(Store *st);
void unit_test_append_applicant(Store *st,
                                const char *name,
                                const char *position,
                                const char *email,
                                const char *phone);
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
int unit_test_get_max_app(void);
int unit_test_get_name_capacity(void);
int unit_test_get_position_capacity(void);
int unit_test_get_email_capacity(void);
int unit_test_get_phone_capacity(void);

typedef struct {
    int fd;
    FILE *stream;
} StreamBackup;

static void init_backup(StreamBackup *backup) {
    backup->fd = -1;
    backup->stream = NULL;
}

static int redirect_stream(FILE *stream, const char *path, const char *mode, StreamBackup *backup) {
    init_backup(backup);
    int fd = DUP(FILENO(stream));
    if (fd == -1) return -1;
    if (!freopen(path, mode, stream)) {
        CLOSE(fd);
        return -1;
    }
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
    if (GetTempFileNameA(temp_dir, "job", 0, template_buf) == 0) return false;

    size_t needed = strlen(template_buf) + 1;
    if (needed > len) {
        remove(template_buf);
        return false;
    }

    strncpy(buffer, template_buf, len - 1);
    buffer[len - 1] = '\0';
#else
    char template_buf[] = "/tmp/jobtestXXXXXX";
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

static bool restore_from_backup(const char *backup_path) {
    if (!backup_path || backup_path[0] == '\0') return false;
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) {
        puts("Unit test warning: failed to restore applicants.csv from backup.");
        return false;
    }
    return true;
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
static bool backup_environment_accessible(const char *backup_path, const char *csv_path) {
    return file_exists(backup_path) && file_exists(csv_path);
}

static bool string_contains(const char *haystack, const char *needle) {
    if (!haystack || !needle) return false;
    return strstr(haystack, needle) != NULL;
}

static bool run_with_redirect(Store *st,
                              const char *input,
                              char **captured_output,
                              void (*target)(Store *)) {
    if (captured_output) *captured_output = NULL;
    char input_path[512] = {0};
    char output_path[512] = {0};
    StreamBackup in_backup, out_backup;
    bool stdin_redirected = false;
    bool stdout_redirected = false;
    char *output = NULL;
    bool ok = false;

    if (!make_temp_path(input_path, sizeof(input_path))) goto cleanup;
    if (!make_temp_path(output_path, sizeof(output_path))) goto cleanup;
    if (!write_text_file(input_path, input)) goto cleanup;

    if (redirect_stream(stdin, input_path, "r", &in_backup) != 0) goto cleanup;
    stdin_redirected = true;
    if (redirect_stream(stdout, output_path, "w", &out_backup) != 0) goto cleanup;
    stdout_redirected = true;

    target(st);
    fflush(stdout);
    if (stdin_redirected) {
        int ch;
        while ((ch = getchar()) != EOF) {
        }
        clearerr(stdin);
    }

    //ส่งให้ตัวเทสตรวจสอบข้อความ
    if (!read_text_file(output_path, &output, NULL)) goto cleanup;
    ok = true;

cleanup:
    if (stdout_redirected) restore_stream(&out_backup);
    if (stdin_redirected) restore_stream(&in_backup);

    if (captured_output) *captured_output = output;
    else free(output);

    remove_file_if_exists(output_path[0] ? output_path : NULL);
    remove_file_if_exists(input_path[0] ? input_path : NULL);

    return ok;
}

static void build_repeated_string(char *buf, size_t buf_len, char ch) {
    if (!buf || buf_len == 0) return;
    size_t fill = buf_len - 1;
    memset(buf, (unsigned char)ch, fill);
    buf[fill] = '\0';
}

//-----add_applicant tests-----
//กรณีปกติ
static bool test_add_normal(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    const char *input =
        "Alice\n"
        "\n"
        "Smith\n"
        "1\n"
        "alice@example.com\n"
        "0123456789\n"
        "\n";

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, add_applicant);
    free(output);

    bool pass = ok &&
                unit_test_get_store_count(st) == 1;
    if (pass) {
        char name[128] = {0}, position[128] = {0}, email[128] = {0}, phone[128] = {0};
        unit_test_extract_applicant(st, 0, name, sizeof(name), position, sizeof(position), email, sizeof(email), phone, sizeof(phone));
        pass = strcmp(name, "Alice Smith") == 0 &&
               strcmp(position, "Developer") == 0 &&
               strcmp(email, "alice@example.com") == 0 &&
               strcmp(phone, "0123456789") == 0;
    }

    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

//กรณีInput ช่องว่างในชื่อ
static bool test_add_normalize_internal_spaces(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    const char *input =
        "Ap         ple\n" //first name with many internal spaces
        "\n"
        "Tester\n"
        "1\n"
        "apple@example.com\n"
        "0999999999\n"
        "\n";

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, add_applicant);
    free(output);

    bool pass = ok && unit_test_get_store_count(st) == 1;
    if (pass) {
        char name[128] = {0}, position[128] = {0}, email[128] = {0}, phone[128] = {0};
        unit_test_extract_applicant(st, 0, name, sizeof(name), position, sizeof(position), email, sizeof(email), phone, sizeof(phone));
        pass = strcmp(name, "Apple Tester") == 0 &&
               strcmp(position, "Developer") == 0 &&
               strcmp(email, "apple@example.com") == 0 &&
               strcmp(phone, "0999999999") == 0;
    }

    if (pass) {
        char *csv_content = NULL;
        if (!read_text_file(csv_path, &csv_content, NULL)) {
            pass = false;
        } else {
            pass = string_contains(csv_content, "Apple Tester,Developer,apple@example.com,0999999999");
            free(csv_content);
        }
    }

    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

//กรณีมีแท็บนำหน้าและท้ายข้อมูล
static bool test_add_trim_tabs(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    const char *input =
        "   Alice   \n"
        "\n"
        "   Smith   \n"
        "   1   \n"
        "   apple.trim@example.com   \n"
        "   0123456789   \n"
        "\n";

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, add_applicant);
    free(output);

    bool pass = ok && unit_test_get_store_count(st) == 1;
    if (pass) {
        char name[128] = {0}, position[128] = {0}, email[128] = {0}, phone[128] = {0};
        unit_test_extract_applicant(st, 0, name, sizeof(name), position, sizeof(position), email, sizeof(email), phone, sizeof(phone));
        pass = strcmp(name, "Alice Smith") == 0 &&
               strcmp(position, "Developer") == 0 &&
               strcmp(email, "apple.trim@example.com") == 0 &&
               strcmp(phone, "0123456789") == 0;
    }

    if (pass) {
        char *csv_content = NULL;
        if (!read_text_file(csv_path, &csv_content, NULL)) {
            pass = false;
        } else {
            pass = string_contains(csv_content, "Alice Smith,Developer,apple.trim@example.com,0123456789");
            free(csv_content);
        }
    }

    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

//กรณีความจุคนสมัครเต็ม
static bool test_add_boundary_capacity(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    int max_app = unit_test_get_max_app();
    for (int i = 0; i < max_app; ++i) {
        char name[64];
        char position[32];
        char email[64];
        char phone[32];
        snprintf(name, sizeof(name), "User%d", i);
        snprintf(position, sizeof(position), "Role%d", i);
        snprintf(email, sizeof(email), "user%d@example.com", i);
        snprintf(phone, sizeof(phone), "09%08d", i);
        unit_test_append_applicant(st, name, position, email, phone);
    }

    char *output = NULL;
    bool ok = run_with_redirect(st, "\n", &output, add_applicant);
    bool pass = ok &&
                unit_test_get_store_count(st) == max_app &&
                string_contains(output, "Storage full.");

    free(output);
    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

//กรณีExtreme input ค่ายาวๆ
static bool test_add_extreme_input(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    int name_cap = unit_test_get_name_capacity();
    int email_cap = unit_test_get_email_capacity();
    int phone_cap = unit_test_get_phone_capacity();

    char *long_first = (char *)malloc((size_t)name_cap);
    char *long_last = (char *)malloc((size_t)name_cap);
    size_t local_len = (size_t)email_cap + 20;
    char *email_local = (char *)malloc(local_len + 1);
    size_t email_total = local_len + strlen("@example.com") + 1;
    char *long_email = (char *)malloc(email_total);
    size_t phone_len = (size_t)phone_cap + 10;
    char *long_phone = (char *)malloc(phone_len + 1);

    if (!long_first || !long_last || !email_local || !long_email || !long_phone) {
        free(long_first); free(long_last); free(email_local); free(long_email); free(long_phone);
        unit_test_destroy_store(st);
        if (!restore_from_backup(backup_path)) return false;
        return false;
    }

    build_repeated_string(long_first, (size_t)name_cap, 'A');
    build_repeated_string(long_last, (size_t)name_cap, 'Z');
    build_repeated_string(email_local, local_len + 1, 'e');
    snprintf(long_email, email_total, "%s@example.com", email_local);
    for (size_t i = 0; i < phone_len; ++i) {
        long_phone[i] = (char)('0' + (int)((i % 10)));
    }
    long_phone[phone_len] = '\0';

    char input[2048];
    snprintf(input, sizeof(input),
             "%s\n\n%s\n1\n%s\n%s\n\n",
             long_first,
             long_last,
             long_email,
             long_phone);

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, add_applicant);
    free(output);

    bool pass = false;
    if (ok && unit_test_get_store_count(st) == 1) {
        char name[256] = {0}, position[128] = {0}, email[512] = {0}, phone[128] = {0};
        unit_test_extract_applicant(st, 0, name, sizeof(name), position, sizeof(position), email, sizeof(email), phone, sizeof(phone));
        pass = strlen(name) == (size_t)name_cap - 1 &&
               strlen(email) == (size_t)email_cap - 1 &&
               strlen(phone) == (size_t)phone_cap - 1 &&
               name[0] == 'A' &&
               email[0] == 'e' &&
               phone[0] == '0';
    }

    free(long_first);
    free(long_last);
    free(email_local);
    free(long_email);
    free(long_phone);
    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

//เช็ค memory leak
static bool test_add_memory_leak(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    const int iterations = 5;
    bool pass = true;

    for (int i = 0; i < iterations && pass; ++i) {
        char input[256];
        char first[32];
        char last[32];
        char email[64];
        char phone[32];
        char suffix = (char)('A' + i);

        snprintf(first, sizeof(first), "User%c", suffix);
        snprintf(last, sizeof(last), "Tester%c", suffix);
        snprintf(email, sizeof(email), "user%d@example.com", i);
        snprintf(phone, sizeof(phone), "%010d", 1000000000 + i);

        snprintf(input, sizeof(input),
                 "%s\n\n%s\n1\n%s\n%s\n\n",
                 first, last, email, phone);
        char *output = NULL;
        if (!run_with_redirect(st, input, &output, add_applicant)) {
            pass = false;
        }
        free(output);
    }

    if (pass && unit_test_get_store_count(st) != iterations) {
        pass = false;
    }

    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

//กรณีenter
static bool test_add_validation_empty_first(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    const char *input =
        "\n"   // empty first name
        "\n";  // pause

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, add_applicant);
    bool pass = ok &&
                unit_test_get_store_count(st) == 0 &&
                string_contains(output, "Please enter a non-empty keyword.");

    free(output);
    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

// กรณีชื่อกลางมีตัวเลขหรืออักขระพิเศษ เพราะชื่อกลางเขียนไม่เหมือน first/last name
static bool test_add_validation_invalid_middle(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    const char *input =
        "John\n" //first name
        "M1*ddle\n" //middle name
        "\n";

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, add_applicant);
    bool pass = ok &&
                unit_test_get_store_count(st) == 0 &&
                string_contains(output, "Error: Name contains invalid characters");

    free(output);
    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

// กรณีenter
static bool test_add_validation_empty_last(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    const char *input =
        "John\n" //first name
        "\n" //middle name
        "\n" //last name
        "\n"; //exit / pause

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, add_applicant);
    bool pass = ok &&
                unit_test_get_store_count(st) == 0 &&
                string_contains(output, "Please enter a non-empty keyword.");

    free(output);
    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

// กรณีเลือกหมายเลขpositionนอกช่วงที่กำหนด
static bool test_add_validation_invalid_position(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    const char *input =
        "John\n"   //first name
        "\n"    //middle name
        "Doe\n" //last name
        "0\n" //invalid position
        "\n"; //pause

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, add_applicant);
    bool pass = ok &&
                unit_test_get_store_count(st) == 0 &&
                string_contains(output, "Invalid Input Must be between 1");

    free(output);
    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

//กรณี(sensitive) มากโดยจะตรวจสอบ , ที่อินพุตเข้ามาในไฟล์ csv
static bool test_add_validation_comma_in_name(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    const char *input =
        "Jo,hn\n"  //first name with comma
        "\n";

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, add_applicant);
    bool pass = ok &&
                unit_test_get_store_count(st) == 0 &&
                string_contains(output, "Error: Input cannot contain a comma");

    free(output);
    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

// กรณีป้อนตัวอักษรในช่องตำแหน่งงาน
static bool test_add_validation_non_numeric_position(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    const char *input =
        "John\n" //first name
        "\n" //middle name
        "Doe\n" //last name
        "abc\n"  //not number
        "\n"; //pause

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, add_applicant);
    bool pass = ok &&
                unit_test_get_store_count(st) == 0 &&
                string_contains(output, "Invalid Input Must be between 1");

    free(output);
    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

// กรณีอีเมลไม่มี @ หรือรูปแบบไม่ถูกต้อง
static bool test_add_validation_invalid_email(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    const char *input =
        "Jane\n" // first name
        "\n" // middle name
        "Doe\n" // last name
        "1\n" // position
        "invalid-email\n"   // invalid email
        "\n"; // pause

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, add_applicant);
    bool pass = ok &&
                unit_test_get_store_count(st) == 0 &&
                string_contains(output, "Email must contain an '@' symbol.");

    free(output);
    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

// กรณีหมายเลขโทรศัพท์มีตัวอักษร
static bool test_add_validation_invalid_phone(const char *backup_path) {
    const char *csv_path = unit_test_get_csv_path();
    if (!copy_file(backup_path, csv_path)) return false;

    Store *st = unit_test_create_store();
    if (!st) return false;

    const char *input =
        "Mark\n"
        "\n"
        "Twain\n"
        "1\n"
        "mark@example.com\n"
        "12345abc\n" // invalid phone with letters
        "\n";

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, add_applicant);
    bool pass = ok &&
                unit_test_get_store_count(st) == 0 &&
                string_contains(output, "Phone Number must contain only digits");

    free(output);
    unit_test_destroy_store(st);
    if (!restore_from_backup(backup_path)) return false;
    return pass;
}

//-----search_applicant tests-----

// กรณีปกติ ป้อนชื่อตรงกับผู้สมัคร
static bool test_search_normal(void) {
    Store *st = unit_test_create_store();
    if (!st) return false;

    unit_test_append_applicant(st, "Alice Johnson", "Developer", "alice@example.com", "0123456789");
    unit_test_append_applicant(st, "Bob Smith", "Marketing", "bob@example.com", "0987654321");

    const char *input =
        "Alice\n"
        "\n";

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, search_applicant);
    bool pass = ok &&
                string_contains(output, "Alice Johnson") &&
                !string_contains(output, "Bob Smith");

    free(output);
    unit_test_destroy_store(st);
    return pass;
}

// กรณีมีแท็บนำหน้าและท้ายคำค้นหา
static bool test_search_trim_tabs(void) {
    Store *st = unit_test_create_store();
    if (!st) return false;

    unit_test_append_applicant(st, "Charlie Tabber", "Marketing", "charlie@example.com", "0112233445");

    const char *input =
        "   Charlie   \n"
        "\n";

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, search_applicant);
    bool pass = ok && string_contains(output, "Charlie Tabber");

    free(output);
    unit_test_destroy_store(st);
    return pass;
}

// กรณีขอบเขต enterหรือค่าว่างไม่ควรทำให้แสดง output ผู้ใช้ทั้งหมดออกมา
static bool test_search_boundary_empty_input(void) {
    Store *st = unit_test_create_store();
    if (!st) return false;

    unit_test_append_applicant(st, "Carol White", "Sales", "carol@example.com", "0112233445");
    int original_count = unit_test_get_store_count(st);

    const char *input =
        "\n"
        "\n";

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, search_applicant);
    bool pass = ok &&
                string_contains(output, "Please enter a non-empty keyword to search.") &&
                unit_test_get_store_count(st) == original_count;

    free(output);
    unit_test_destroy_store(st);
    return pass;
}

// กรณีExtreme เทสขInput ค่ายาวๆ
static bool test_search_extreme_long_query(void) {
    Store *st = unit_test_create_store();
    if (!st) return false;

    unit_test_append_applicant(st, "Derek Long", "HR Specialist", "derek@example.com", "0778899000");

    char query_line[512];
    size_t qlen = sizeof(query_line) - 2;
    for (size_t i = 0; i < qlen; ++i) query_line[i] = 'Q';
    query_line[qlen] = '\n';
    query_line[qlen + 1] = '\0';

    char input[1024];
    snprintf(input, sizeof(input), "%s\n", query_line);

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, search_applicant);
    bool pass = ok && string_contains(output, "No matches.");

    free(output);
    unit_test_destroy_store(st);
    return pass;
}

// กรณีที่เสิร์ชซ้ำๆแล้วข้อมูลของผู้สมัครไม่เปลี่ยนแปลง
static bool test_search_memory_leak(void) {
    Store *st = unit_test_create_store();
    if (!st) return false;

    unit_test_append_applicant(st, "Eve Adams", "Developer", "eve@example.com", "0667788990");
    unit_test_append_applicant(st, "Frank Stone", "Marketing", "frank@example.com", "0556677889");
    int initial_count = unit_test_get_store_count(st);

    const int iterations = 8;
    bool pass = true;

    for (int i = 0; i < iterations && pass; ++i) {
        const char *input =
            "Eve\n"
            "\n";
        char *output = NULL;
        if (!run_with_redirect(st, input, &output, search_applicant)) {
            pass = false;
        } else if (!string_contains(output, "Eve Adams")) {
            pass = false;
        }
        free(output);
    }

    if (pass && unit_test_get_store_count(st) != initial_count) {
        pass = false;
    }

    unit_test_destroy_store(st);
    return pass;
}

// กรณีไม่สนตัวพิมพ์ใหญ่เล็ก
static bool test_search_case_insensitive(void) {
    Store *st = unit_test_create_store();
    if (!st) return false;

    unit_test_append_applicant(st, "Grace Hopper", "Developer", "grace@example.com", "0101010101");

    const char *input =
        "grace\n"
        "\n";

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, search_applicant);
    bool pass = ok && string_contains(output, "Grace Hopper");
    free(output);
    unit_test_destroy_store(st);
    return pass;
}

//กรณีค้นหาด้วยตำแหน่งงาน
static bool test_search_by_position(void) {
    Store *st = unit_test_create_store();
    if (!st) return false;

    unit_test_append_applicant(st, "Henry Ford", "Marketing", "henry@example.com", "0222333444");

    const char *input =
        "marketing\n"
        "\n";

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, search_applicant);
    bool pass = ok && string_contains(output, "Henry Ford");
    free(output);
    unit_test_destroy_store(st);
    return pass;
}

//กรณีหลายผลลัพธ์
static bool test_search_multiple_matches(void) {
    Store *st = unit_test_create_store();
    if (!st) return false;

    unit_test_append_applicant(st, "Ivan Stone", "Developer", "ivan@example.com", "0333444555");
    unit_test_append_applicant(st, "Ivy Lee", "Developer", "ivy@example.com", "0444555666");

    const char *input =
        "iv\n"
        "\n";

    char *output = NULL;
    bool ok = run_with_redirect(st, input, &output, search_applicant);
    bool pass = ok &&
                string_contains(output, "1)") &&
                string_contains(output, "2)") &&
                string_contains(output, "Ivan Stone") &&
                string_contains(output, "Ivy Lee");

    free(output);
    unit_test_destroy_store(st);
    return pass;
}

//output สรุปผลการทดสอบทั้งหมด
void run_unit_tests(Store *shared_store) {
    if (!shared_store) return;

    clear_screen();
    puts("====================================");
    puts("Running unit tests for applicants...");
    puts("====================================");

    const char *csv_path = unit_test_get_csv_path();
    char backup_path[512];
    int written = snprintf(backup_path, sizeof(backup_path), "%s.unittest.bak", csv_path);
    if (written < 0 || (size_t)written >= sizeof(backup_path)) {
        puts("Cannot generate backup path. Aborting tests.");
        pause();
        return;
    }

    if (!copy_file(csv_path, backup_path)) {
        puts("Cannot create backup of applicants.csv. Aborting tests.");
        pause();
        return;
    }

    int total = 0;
    int passed = 0;
    bool abort_remaining = false;

    //add_applicant
    struct AddTest {
        const char *label;
        bool (*fn)(const char *);
    } add_tests[] = {
        {"Normal",                        test_add_normal},
        {"Normalization (internal spaces)", test_add_normalize_internal_spaces},
        {"Trim (tab around input)",       test_add_trim_tabs},
        {"Boundary (capacity limit)",     test_add_boundary_capacity},
        {"Extreme (long input)",          test_add_extreme_input},
        {"Memory (repeated additions)",   test_add_memory_leak},
        {"Validation (empty first name)", test_add_validation_empty_first},
        {"Validation (invalid middle)",   test_add_validation_invalid_middle},
        {"Validation (empty last name)",  test_add_validation_empty_last},
        {"Validation (invalid position)", test_add_validation_invalid_position},
        {"Validation (comma in name)",    test_add_validation_comma_in_name},
        {"Validation (non-numeric position)", test_add_validation_non_numeric_position},
        {"Validation (invalid email)",    test_add_validation_invalid_email},
        {"Validation (invalid phone)",    test_add_validation_invalid_phone},
    };
    const size_t add_count = sizeof(add_tests) / sizeof(add_tests[0]);

    //search_applicant
    struct SearchTest {
        const char *label;
        bool (*fn)(void);
    } search_tests[] = {
        {"Normal",                        test_search_normal},
        {"Trim (tab around query)",       test_search_trim_tabs},
        {"Boundary (empty query)",        test_search_boundary_empty_input},
        {"Extreme (long query)",          test_search_extreme_long_query},
        {"Memory (repeated searches)",    test_search_memory_leak},
        {"Case insensitive match",        test_search_case_insensitive},
        {"Position match",                test_search_by_position},
        {"Multiple matches listing",      test_search_multiple_matches},
    };
    const size_t search_count = sizeof(search_tests) / sizeof(search_tests[0]);

    puts("---- add_applicant ----");
    for (size_t i = 0; i < add_count; ++i) {
        if (abort_remaining) {
            printf("[SKIP] add_applicant %s test (prior I/O error)\n", add_tests[i].label);
            continue;
        }
        //เรียกเทสต์แต่ละตัว พร้อมพิมพ์ PASS/FAIL
        bool ok = add_tests[i].fn(backup_path);
        printf("[%s] add_applicant %s test\n", ok ? "PASS" : "FAIL", add_tests[i].label);
        total++;
        if (ok) {
            passed++;
        } else if (!backup_environment_accessible(backup_path, csv_path)) {
            puts("Fatal I/O issue detected. Aborting remaining unit tests.");
            abort_remaining = true;
        }
    }

    //puts("");
    puts("---- search_applicant ----");
    if (!abort_remaining) {
        for (size_t i = 0; i < search_count; ++i) {
            bool ok = search_tests[i].fn();
            printf("[%s] search_applicant %s test\n", ok ? "PASS" : "FAIL", search_tests[i].label);
            total++;
            if (ok){
            passed++;
            }
        }
    } else {
        puts("[SKIP] search_applicant suite skipped due to prior I/O error.");
    }

    bool restored = restore_from_backup(backup_path);
    if (restored) {
        remove_file_if_exists(backup_path);
        load_csv(shared_store);
    } else {
        printf("Warning: original applicants.csv not restored. Backup retained at %s\n", backup_path);
    }
    printf("\nResult: %d/%d tests passed.\n", passed, total);
    fflush(stdout);
    pause();
}
