#include <stdio.h>
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
void display(){
    printf("******JOB_APPLICANT******\n");
    printf("1)Add\n");
    printf("2)Search\n");
    printf("3)Update\n");
    printf("4)Delete\n"); 
    printf("5)Exit\n");   
    printf("SELECT:"); 
}
int main(){
    while(1){
    display();
    int i;
    scanf("%d",&i);//input ตัวอักษรแล้วพัง
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
                puts("Bye!"); 
                return 0;          
            default: puts("Please choose 1-5."); break;
            }
        }
    return 0;
}