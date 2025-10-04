#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//Macro
#define CSV_FILE "applicants.csv"
#define MAX_APP 100//จำนวนผู้สมัคร
#define LINE_LEN 512

//ป้องกัน buff overflow
typedef struct{
    char name[100];
    char position[100];
    char email[120];
    char phone[40];
}Applicant;

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
void add_applicant(Store *st){
    Applicant a;

    printf("Enter Applicant Name: ");
    scanf(" %99[^\n]", a.name);        

    printf("Enter Job Position: ");
    scanf(" %99[^\n]", a.position);    

    printf("Enter Email: ");
    scanf(" %119[^\n]", a.email);      

    printf("Enter Phone Number: ");
    scanf(" %39[^\n]", a.phone);       
//สมมติมีข้อมูลคน 2 คน count จะเป็น 2 เมื่อ add ข้อมูลจะถูกเก็บใน arr[2] และ count เพิ่มเป็น3
    st->arr[st->count++] = a;          
    save_csv(st);                      
    puts("Added and saved.");
}
void search_applicant(Store *st){
    char q[LINE_LEN];

    printf("Search by name or position: ");
    scanf(" %511[^\n]", q);  

    puts("\n-- Results --");
    int found = 0;
    for(int i = 0; i < st->count; i++){
        if (strstr(st->arr[i].name, q) || strstr(st->arr[i].position, q)){//strstr ใช้ค้นหารูปแบบในสตริง
            printf("%d) %s | %s | %s | %s\n",
                   i+1,
                   st->arr[i].name,
                   st->arr[i].position,
                   st->arr[i].email,
                   st->arr[i].phone);
            found = 1;
        }
    }
    if(!found) puts("No match found.");
    puts("--------------");
}
void update_applicant(Store *st) {
    char q[LINE_LEN];
    printf("Enter applicant name to update: ");
    scanf(" %511[^\n]", q);   //รับชื่อที่ต้องการค้นหา

    int i;
    for (i = 0; i < st->count; i++) {
        if (strcmp(st->arr[i].name, q) == 0) {   //เทียบชื่อแบบตรงๆ
            printf("Found: %s | %s | %s | %s\n",
                   st->arr[i].name,
                   st->arr[i].position,
                   st->arr[i].email,
                   st->arr[i].phone);

            // กรอกข้อมูลใหม่ทั้งหมด
            printf("Enter new name: ");
            scanf(" %99[^\n]", st->arr[i].name);

            printf("Enter new position: ");
            scanf(" %99[^\n]", st->arr[i].position);

            printf("Enter new email: ");
            scanf(" %119[^\n]", st->arr[i].email);

            printf("Enter new phone: ");
            scanf(" %39[^\n]", st->arr[i].phone);

            save_csv(st);   // บันทึกกลับไฟล์ทันที
            puts("Updated and saved.");
            return;
        }
    }

    puts("Applicant not found.");
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
    printf("6)Exit\n");   
    printf("SELECT:"); 
}

int main(){
    Store st;
    load_csv(&st);

    while(1){
    display_menu();
    int i;
    if(scanf("%d",&i)!=1){
        puts("\nInvalid input");
    }
    getchar();
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
                puts("Bye!"); 
                return 0;          
            default: puts("Please choose 1-6");
            }
        }
    return 0;
}