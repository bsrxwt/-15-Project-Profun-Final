#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
//Macro
#define CSV_FILE "applicants.csv"
#define MAX_APP 100//จำนวนผู้สมัคร
#define LINE_LEN 512
#define NAME_LEN 100
#define POS_LEN 100
#define EMAIL_LEN 120
#define PHONE_LEN 40
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
/*ต้องแก้ทุก input ยังรับ enter ได้*/
void add_applicant(Store *st){
    if(st->count >= MAX_APP){ //ถ้าผู้สมัครเกิน 100 คน
        printf("Storage full.\n");
        return;
    }
    Applicant a;
    //รับชื่อ
    char buf[LINE_LEN];
    printf("Enter Applicant Name: "); fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    cut(buf); trim(buf);
    strncpy(a.name, buf, NAME_LEN - 1); a.name[NAME_LEN - 1] = '\0';
    // รับตำแหน่ง
    printf("Enter Job Position: "); fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    cut(buf); trim(buf);
    strncpy(a.position, buf, POS_LEN - 1); a.position[POS_LEN - 1] = '\0';
    // รับอีเมล
    printf("Enter Email: "); fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    cut(buf); trim(buf);
    strncpy(a.email, buf, EMAIL_LEN - 1); a.email[EMAIL_LEN - 1] = '\0';
    // รับเบอร์โทร
    printf("Enter Phone Number: "); fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return;
    cut(buf); trim(buf);
    strncpy(a.phone, buf, PHONE_LEN - 1); a.phone[PHONE_LEN - 1] = '\0';    
//สมมติมีข้อมูลคน 2 คน count จะเป็น 2 เมื่อ add ข้อมูลจะถูกเก็บใน arr[2] และ count เพิ่มเป็น3
    st->arr[st->count++] = a;          
    save_csv(st);                      
    puts("Added and saved.");
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
static void update_applicant(Store *st) {
    char q[LINE_LEN];
    printf("Search (name or position) to update: ");
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
    int m = list_matches(st, num, q);
    puts("=============");
    if (m <= 0) return;

    printf("Select number to update (1-%d): ", m);
    char pick[LINE_LEN];
    if (!fgets(pick, sizeof(pick), stdin)){
        puts("Invalid Input."); 
        return; 
    }
    cut(q); trim(q);

    int pick_num = atoi(pick);
    if (pick_num < 1 || pick_num > m) {
        puts("======================================="); 
        printf("Invalid Input Must be between 1 and %d.\n", m); 
        puts("=======================================");
        return; 
    }
    int i = num[pick_num - 1];
/*Update fields+choice แก้การรับ enter CTRL+Z*/
    printf("Update fields: 1)Name 2)Position 3)Email 4)Phone 5)All : ");
    int choice = 0;
    if (scanf("%d", &choice) != 1) { puts("Invalid."); return; }
    getchar();
/*ยังไม่ป้องกันinputที่ไม่ใช่choice 1-5*/
    char buf[LINE_LEN];
    if (choice == 1 || choice == 5) {
        printf("New Name: "); if (fgets(buf, sizeof(buf), stdin)) {
            cut(buf); trim(buf);
            strncpy(st->arr[i].name, buf, NAME_LEN - 1);
            st->arr[i].name[NAME_LEN - 1] = '\0';
        }
    }
    if (choice == 2 || choice == 5) {
        printf("New Position: "); if (fgets(buf, sizeof(buf), stdin)) {
            cut(buf); trim(buf);
            strncpy(st->arr[i].position, buf, POS_LEN - 1);
            st->arr[i].position[POS_LEN - 1] = '\0';
        }
    }
    if (choice == 3 || choice == 5) {
        printf("New Email: "); if (fgets(buf, sizeof(buf), stdin)) {
            cut(buf); trim(buf);
            strncpy(st->arr[i].email, buf, EMAIL_LEN - 1);
            st->arr[i].email[EMAIL_LEN - 1] = '\0';
        }
    }
    if (choice == 4 || choice == 5) {
        printf("New Phone: "); if (fgets(buf, sizeof(buf), stdin)) {
            cut(buf); trim(buf);
            strncpy(st->arr[i].phone, buf, PHONE_LEN - 1);
            st->arr[i].phone[PHONE_LEN - 1] = '\0';
        }
    }

    save_csv(st);
    printf("Updated and saved.\n");
}
void delete_applicant(Store *st) {
    char q[LINE_LEN];
    printf("Enter applicant name to delete: ");
    scanf(" %511[^\n]", q); //รับชื่อที่จะลบ

    int i;
    for (i = 0; i < st->count; i++) {
        if (strcmp(st->arr[i].name, q) == 0) { //เทียบชื่อแบบตรงๆ
            printf("Deleting: %s | %s | %s | %s\n",
                   st->arr[i].name,
                   st->arr[i].position,
                   st->arr[i].email,
                   st->arr[i].phone);

            // เลื่อนสมาชิกถัดไปทั้งหมดขึ้นมาทับ
            for (int j = i; j < st->count - 1; j++) {
                st->arr[j] = st->arr[j + 1];
            }
            st->count--;

            save_csv(st);   // บันทึกกลับไฟล์ทันที
            puts("Deleted and saved.");
            return;
        }
    }

    puts("Applicant not found.");
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
    printf("******JOB_APPLICANT******\n");
    printf("1)Add\n");
    printf("2)Search\n");
    printf("3)Update\n");
    printf("4)Delete\n");
    printf("5)Display\n");
    printf("6)Unit Test\n");
    printf("7)E2E Test\n");
    printf("8)Exit\n");   
    printf("SELECT:"); 
}

int main(){
    Store st;
    load_csv(&st);

    while(1){
    display_menu();
    int i=0;
    int c;
    if(scanf("%d",&i)!=1){
        puts("\nPlease input number.");
            while ((c = getchar()) != '\n' && c != EOF) {} //ล้าง buffer เมื่อ if ทำงาน
            continue;
    }
            while ((c = getchar()) != '\n' && c != EOF) {}//ล้าง \n จาก enter
            switch (i)
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
            default: puts("Please choose 1-8");
            }
        }
    return 0;
}