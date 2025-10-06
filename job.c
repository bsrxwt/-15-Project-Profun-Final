#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
//clear
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#endif
//Macro
#define CSV_FILE "applicants.csv"
#define MAX_APP 100//จำนวนผู้สมัคร
#define LINE_LEN 512
#define NAME_LEN 50
#define POS_LEN 40
#define EMAIL_LEN 50
#define PHONE_LEN 20
//ป้องกัน buff overflow
typedef struct {
    char name[NAME_LEN];       
    char position[POS_LEN];    
    char email[EMAIL_LEN];     
    char phone[PHONE_LEN];     
} Applicant;

typedef struct{
    Applicant arr[MAX_APP]; //array ของ Applicant
    int count;//จำนวนผู้ใช้งาน
} Store;
//clear
#ifdef _WIN32
void clear_screen()
{
    system("cls");
}
#else
void clear_screen()
{
    system("clear");
}
#endif

void pause()
{
    printf("\nPress Enter to continue...");
    while (getchar() != '\n');
    clear_screen();
}

//ตัด \n กับ \r ตอนใช้ fgets
void cut(char *s){
    if(!s) return;
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) {
        s[--n] = '\0';  
    }
}
static void trim(char *s) {//ตัด whitespace
    if (!s) return; 
    char *p = s;//ตัดหัว
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);//ตัดท้าย
    while (n && isspace((unsigned char)s[n-1])) s[--n] = '\0';
}
void check_csv(){
    FILE *f = fopen(CSV_FILE,"r");
    if(f){//มีไฟล์อยู่แล้ว
        fclose(f);
        return;
    }
    f = fopen(CSV_FILE,"w");
    if (!f) { perror("create csv"); exit(1); }//ไม่มีไฟล์
    fprintf(f,"ApplicantName,JobPosition,Email,PhoneNumber\n");//header
    fclose(f);
}
void load_csv(Store *st){
    st->count=0;
    check_csv();

    FILE *f = fopen(CSV_FILE, "r");
    if (!f) { perror("open csv"); exit(1); }
    
    char line[LINE_LEN];
    int line_no =0;
    while(fgets(line,sizeof(line),f)){
        cut(line);
        if(line_no++ ==0) continue;//ข้าม header
        if(line[0]=='\0') continue; //ข้ามบรรทัดว่าง
        if(st->count >= MAX_APP) break;//ถ้าพ้อยเตอร์ที่ชี้ไปยัง count >= MAX_APP จะหยุดทำงานทันที
        //แบ่งคอลัมด้วย ,
        char *name     = strtok(line, ",");
        char *position = strtok(NULL, ",");
        char *email    = strtok(NULL, ",");
        char *phone    = strtok(NULL, ",");
    
        if (!name || !position || !email || !phone) continue;//ตรวจสอบ format ของ input ถ้าไม่ตรงจะข้าม

        Applicant *a = &st->arr[st->count];
        snprintf(a->name,     sizeof(a->name),     "%s", name); //snprinf จะกำหนด size ของ string ทำให้กัน buff overflow
        snprintf(a->position, sizeof(a->position), "%s", position);
        snprintf(a->email,    sizeof(a->email),    "%s", email);
        snprintf(a->phone,    sizeof(a->phone),    "%s", phone);
        st->count++;//บวกเพิ่มตามบรรทัดที่อ่านเจอ
    }
    fclose(f);

}
void save_csv(const Store *st){
    FILE *f = fopen(CSV_FILE, "w");
    if(!f){ perror("save csv"); exit(1); }

    fprintf(f, "ApplicantName,JobPosition,Email,PhoneNumber\n");
    for(int i = 0;i<st->count;++i){//ลูบข้อมูลแต่ละคนจนกว่าจะเป็นเท็จ
        fprintf(f, "%s,%s,%s,%s\n",
            st->arr[i].name,
            st->arr[i].position,
            st->arr[i].email,
            st->arr[i].phone);
    }
    fclose(f);
}
//Add Structure
int empty_add(char *buf){
    if (buf[0] == '\0'){
        puts("==================================");
        puts("Please enter a non-empty keyword.");
        puts("==================================");
        return 0; 
    }
    return 1;
}
int check_for_comma(char *buf) {
    if (strchr(buf, ',') != NULL) {
        puts("==================================================");
        puts("Error: Input cannot contain a comma (',') for CSV.");
        puts("==================================================");
        return 0; //พบจุลภาค
    }
    return 1; //ไม่พบจุลภาค
}
int get_input_and_validate(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt); 
    fflush(stdout);
    
    //ตรวจสอบ EOF
    if (!fgets(buf, size, stdin)) {
        puts("==================");
        puts("Input cancelled.");
        puts("==================");
        pause();
        return 0; 
    }
    cut(buf); trim(buf);
    //ตรวจสอบคอมม่า
    if (check_for_comma(buf) == 0) {
        pause();
        return 0;
    }
    //ตรวจสอบค่าว่าง
    if (empty_add(buf) == 0) {
        pause();
        return 0;
    }
    return 1; //สำเร็จ
}
void add_applicant(Store *st){
    clear_screen();
    if(st->count >= MAX_APP){ //ถ้าผู้สมัครเกิน 100 คน
        printf("Storage full.\n");
        return;
    }
    Applicant a;
    char buf[LINE_LEN];
    char buf_f[LINE_LEN]; //First Name
    char buf_m[LINE_LEN]; //Middle Name
    char buf_l[LINE_LEN]; //Last Name
    char buf_full[NAME_LEN * 3];
    //รับชื่อ
    if (get_input_and_validate("Enter First Name: ", buf_f, sizeof(buf_f)) == 0) {
        return; 
    }
    if (get_input_and_validate("Enter Middle Name: ", buf_m, sizeof(buf_m)) == 0) {
        return; 
    }
    if (get_input_and_validate("Enter Last Name: ", buf_l, sizeof(buf_l)) == 0) {
        return; 
    }
    snprintf(buf_full, sizeof(buf_full), "%s %s %s", buf_f, buf_m, buf_l);
    strncpy(a.name, buf_full, NAME_LEN - 1); a.name[NAME_LEN - 1] = '\0';
    // รับตำแหน่ง
    if (get_input_and_validate("Enter Job Position: ", buf, sizeof(buf)) == 0) {
    return; 
    }
    strncpy(a.position, buf, POS_LEN - 1); a.position[POS_LEN - 1] = '\0';
    // รับอีเมล
    if (get_input_and_validate("Enter Email: ", buf, sizeof(buf)) == 0) {
    return; 
    }
    strncpy(a.email, buf, EMAIL_LEN - 1); a.email[EMAIL_LEN - 1] = '\0';
    // รับเบอร์โทร
    if (get_input_and_validate("Enter Phone Number: ", buf, sizeof(buf)) == 0) {
    return; 
    }
    strncpy(a.phone, buf, PHONE_LEN - 1); a.phone[PHONE_LEN - 1] = '\0';    
//สมมติมีข้อมูลคน 2 คน count จะเป็น 2 เมื่อ add ข้อมูลจะถูกเก็บใน arr[2] และ count เพิ่มเป็น3
    st->arr[st->count++] = a;          
    save_csv(st);
    puts("=================");                     
    puts("Added and saved.");
    puts("=================");
    pause();
}
//Search Structure
int nocase_ncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca == '\0') return (cb == '\0') ? 0 : 0 - cb; 
        
        int da = tolower(ca);
        int db = tolower(cb);
        if (da != db) return da - db;
    }
    return 0;
}
char* strcasestr_local(const char *hay, const char *needle) {
    if (!hay || !needle || !*needle) return (char*)hay;
    size_t nlen = strlen(needle);
    for (const char *p = hay; *p; ++p) {
        if (tolower((unsigned char)*p) == tolower((unsigned char)*needle)) {
            if (nocase_ncmp(p, needle, nlen) == 0) return (char*)p;
        }
    }
    return NULL;
}
int list_matches(const Store *st, int *num, const char *q) {
    int m = 0;
    for (int i = 0; i < st->count; ++i) {
        if (strcasestr_local(st->arr[i].name, q) || strcasestr_local(st->arr[i].position, q)){
            if(m>=MAX_APP) break;
            num[m++] = i;
            printf("%d) %s | %s | %s | %s\n",
                   m,
                   st->arr[i].name,
                   st->arr[i].position,
                   st->arr[i].email,
                   st->arr[i].phone);
        }
    }
    if (m == 0) printf("No matches.\n");
    return m;
}
void search_applicant(Store *st){
    char q[LINE_LEN];
    printf("Search by name or position: ");
    if (!fgets(q, sizeof(q), stdin)) return;
    cut(q); trim(q); 
    if (q[0] == '\0') {
        puts("===========================================");
        puts("Please enter a non-empty keyword to search.");
        puts("===========================================");
        return; 
    }
    int num[MAX_APP];
    puts("== Results ==");
    (void)list_matches(st, num, q);
    puts("=============");
}
void update_applicant(Store *st) {
    char q[LINE_LEN];
    printf("Search (name or position) to update: ");
    if (!fgets(q, sizeof(q), stdin)){ 
        puts("==============");
        puts("Invalid Input");
        puts("==============");
        return;}
    cut(q); trim(q);

    if (q[0] == '\0') {
        puts("===========================================");
        puts("Please enter a non-empty keyword to search.");
        puts("===========================================");
        return; 
    }

    int num[MAX_APP];
    puts("== Results ==");
    int m = list_matches(st, num, q);
    puts("=============");
    if (m <= 0) return;
//list input
    printf("Select number to update (1-%d): ", m);
    char pick[LINE_LEN];
    if (!fgets(pick, sizeof(pick), stdin)){ 
        puts("==============");
        puts("Invalid Input");
        puts("==============");
        return; 
    }
    cut(pick); trim(pick);
    if (pick[0] == '\0') {
        puts("===========================================");
        puts("Please enter a non-empty keyword to select.");
        puts("===========================================");
        return; 
    }
    char *endptr;
    long choice_m = strtol(pick, &endptr, 10); 
    int pick_num = (int)choice_m;
    if (*endptr != '\0' || pick_num < 1 || pick_num > m) {
        puts("======================================="); 
        printf("Invalid Input Must be between 1 and %d.\n", m); 
        puts("=======================================");
        return; 
    }
    int i = num[pick_num - 1];
/*Update fields+choice แก้การรับเลขที่ไม่ใช่ 1-5*/
    printf("Update fields: 1)Name 2)Position 3)Email 4)Phone 5)All : ");
    char choice_str[LINE_LEN];
    if (!fgets(choice_str, sizeof(q), stdin)){
        puts("==============");
        puts("Invalid Input");
        puts("==============");
        return;}
    cut(choice_str); trim(choice_str);
    if (choice_str[0] == '\0') {
        puts("===========================================");
        puts("Please enter a non-empty keyword to search.");
        puts("===========================================");
        return; 
    }

    long choice_l = strtol(choice_str, &endptr, 10); 
    int choice = (int)choice_l;
    if (*endptr != '\0' || choice < 1 || choice > 5) {
        puts("======================================="); 
        puts("Invalid Input Must be between 1 and 5."); 
        puts("=======================================");
        return; 
    }

    char buf[LINE_LEN];
    //update name
    if (choice == 1 || choice == 5) {
    printf("New Name: "); 
        if (!fgets(buf, sizeof(buf), stdin)) {
            puts("==============");
            puts("Invalid Input");
            puts("==============");
        return;
        }
        cut(buf); trim(buf);
        if(empty_add(buf)==0){
            return;
        }
    strncpy(st->arr[i].name, buf, NAME_LEN - 1);
    st->arr[i].name[NAME_LEN - 1] = '\0';
    }
    //update position
    if (choice == 2 || choice == 5) {
        printf("New Position: ");
        if (!fgets(buf, sizeof(buf), stdin)) {
            puts("==============");
            puts("Invalid Input");
            puts("==============");
        return;
        } 
            cut(buf); trim(buf);
            if(empty_add(buf)==0){
                return;
            }
            strncpy(st->arr[i].position, buf, POS_LEN - 1);
            st->arr[i].position[POS_LEN - 1] = '\0';
        
    }
    //update email
    if (choice == 3 || choice == 5) {
        printf("New Email: ");
        if (!fgets(buf, sizeof(buf), stdin)) {
            puts("==============");
            puts("Invalid Input");
            puts("==============");
        return;
        } 
            cut(buf); trim(buf);
            if(empty_add(buf)==0){
                return;
            }
            strncpy(st->arr[i].email, buf, EMAIL_LEN - 1);
            st->arr[i].email[EMAIL_LEN - 1] = '\0';
        
    }
    //update phone
    if (choice == 4 || choice == 5) {
        printf("New Phone: ");
        if (!fgets(buf, sizeof(buf), stdin)) {
            puts("==============");
            puts("Invalid Input");
            puts("==============");
        return;
        }
            cut(buf); trim(buf);
            if(empty_add(buf)==0){
                return;
            }
            strncpy(st->arr[i].phone, buf, PHONE_LEN - 1);
            st->arr[i].phone[PHONE_LEN - 1] = '\0';
        
    }

    save_csv(st);
    printf("Updated and saved.\n");
}
void delete_applicant(Store *st) {
    char q[LINE_LEN];
    printf("Search (name or position) to update: ");
    if (!fgets(q, sizeof(q), stdin)){
        puts("==============");
        puts("Invalid Input");
        puts("==============");
        return;
    }
    cut(q); trim(q);

    if (q[0] == '\0') {
        puts("===========================================");
        puts("Please enter a non-empty keyword to search.");
        puts("===========================================");
        return; 
    }

    int num[MAX_APP];
    puts("== Results ==");
    int m = list_matches(st, num, q);
    puts("=============");
    if (m <= 0) return;

    printf("Select number to delete (1-%d): ", m);
    char pick[LINE_LEN];
    if (!fgets(pick, sizeof(pick), stdin)){
        puts("==============");
        puts("Invalid Input");
        puts("==============");
        return; 
    }
    cut(pick); trim(pick);

    if (pick[0] == '\0') {
        puts("===========================================");
        puts("Please enter a non-empty keyword to delete.");
        puts("===========================================");
        return; 
    }

    char *endptr;
    long choice_m = strtol(pick, &endptr, 10); 
    int pick_num = (int)choice_m;
    if (*endptr != '\0' || pick_num < 1 || pick_num > m) {
        puts("======================================="); 
        printf("Invalid Input Must be between 1 and %d.\n", m); 
        puts("=======================================");
        return; 
    }
    int i = num[pick_num - 1];
    for (int k = i; k < st->count - 1; ++k) st->arr[k] = st->arr[k + 1];
    st->count--;

    save_csv(st);
    printf("Deleted and saved.\n");
}
void DisplayAll(const Store *st){
    printf("\n=== Applicants (%d) ===\n", st->count);
    for (int i = 0; i < st->count; ++i) {
        printf("%d) %s | %s | %s | %s\n",
               i + 1,
               st->arr[i].name,
               st->arr[i].position,
               st->arr[i].email,
               st->arr[i].phone);
    }
    printf("=======================\n\n");
}
void display_menu(){
    printf("\n******JOB_APPLICANT******\n");
    printf("1)Add\n");
    printf("2)Search\n");
    printf("3)Update\n");
    printf("4)Delete\n");
    printf("5)Display\n");
    printf("6)Unit Test\n");
    printf("7)E2E Test\n");
    printf("8)Exit\n");   
    printf("SELECT: "); 
}
int main(){
    clear_screen();
    Store st;
    load_csv(&st);

    while(1){
    display_menu();
    char i[LINE_LEN];
    fflush(stdout);
        if (!fgets(i, sizeof(i), stdin)){            
            printf("==================\n");
            printf("Please choose 1-8\n");
            printf("==================\n");
            pause();
            continue;
        }
        cut(i); trim(i);

        if (i[0] == '\0') {            
            puts("==================================");
            puts("Please enter a non-empty keyword.");
            puts("==================================");
            pause();
            continue;
        }
        
        char *endptr;
        long num_l = strtol(i, &endptr, 10); 
        int num = (int)num_l;


        if (*endptr != '\0' || endptr == i) {
            puts("==================");
            puts("Please choose 1-8");
            puts("==================");
            pause();
            continue;
        }
            switch (num)
            {
            case 1: 
                add_applicant(&st);
                break;       
            case 2: 
                search_applicant(&st); 
                break;   
            case 3: 
                update_applicant(&st); 
                break;   
            case 4: 
                delete_applicant(&st); 
                break;   
            case 5:
                DisplayAll(&st);
                break;
            case 6:
                
            case 7:

            case 8: 
                puts("Bye!"); 
                return 0;          
            default: 
            puts("==================");
            puts("Please choose 1-8");
            puts("==================");
            pause();
            }
        }
    return 0;
}