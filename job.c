#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//Macro
#define CSV_FILE "applicants.csv"
#define MAX_APP 100
#define LINE_LEN 512

//ป้องกัน buff overflow
typedef struct{
    char name[100];
    char position[100];
    char email[120];
    char phone[40];
}Applicant;

typedef struct{
    Applicant arr[MAX_APP];
    int count;
} Store;
//ตัด \n ตอนใช้ fgets
void cut(char *s){
    if(!s)
    return;
    size_t n =strlen(s);
    while(n&&(s[n-1]=='\n')){
        s[n--]= '\0';
    }
}
void check_csv(){
    FILE *f = fopen(CSV_FILE,"r");
    if(f){
        fclose(f);
        return;
    }
    f = fopen(CSV_FILE,"w");
    if (!f) { perror("create csv"); exit(1); }
    fprintf(f,"ApplicantName,JobPosition,Email,PhoneNumber\n");
    fclose(f);
}
void load_csv(Store *st){
    check_csv();

    FILE *f = fopen(CSV_FILE, "r");
    if (!f) { perror("open csv"); exit(1); }
    
    char line[LINE_LEN];
    int line_no =0;
    while(fgets(line,sizeof(line),f)){
        cut(line);
        if(line_no++ ==0) continue;//ข้าม header
        if(line[0]=='\0') continue; //ข้ามบรรทัดว่าง
        if(st->count >= MAX_APP) break;
        
    }
    fclose(f);

}
void save_csv(const Store *st){
    FILE *f = fopen(CSV_FILE, "w");
    if(!f){ perror("save csv"); exit(1); }

    fprintf(f, "ApplicantName,JobPosition,Email,PhoneNumber\n");
    for(int i = 0;i<st->count;++i){
        fprintf(f, "%s,%s,%s,%s\n",st->arr[i].name,st->arr[i].position,st->arr[i].email,st->arr[i].phone);
    }
    fclose(f);
}
void add_applicant(){
    printf("1\n");
}
void search_applicant(){
    printf("2\n");
}
void update_applicant(){
    printf("3\n");
}
void delete_applicant(){
    printf("4\n");
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
    printf("6)Exit\n");   
    printf("SELECT:"); 
}
int main(){
    Store st;
    load_csv(&st);

    while(1){
    display_menu();
    int i;
    scanf("%d",&i);//input ตัวอักษรแล้วพัง
    getchar();
            switch (i)
            {
            case 1: 
                add_applicant();
                break;       
            case 2: 
                search_applicant(); 
                break;   
            case 3: 
                update_applicant(); 
                break;   
            case 4: 
                delete_applicant(); 
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