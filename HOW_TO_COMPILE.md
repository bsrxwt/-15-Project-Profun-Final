# วิธีคอมไพล์และรันโปรเจ็กต์

คู่มือนี้สรุปขั้นตอนการคอมไพล์และรันโปรแกรมทั้งบน Windows และ Linux 

## Windows
- **คอมไพล์**
  ```
  gcc E2E_test.c job.c unit_test.c -o applicant.exe
  ```
- **รันโปรแกรม**
  ```
  .\applicant.exe
  ```

## Linux(khali)
- **เตรียมเครื่องมือ**
  - แนะนำให้ติดตั้ง `valgrind` สำหรับตรวจสอบ memory leak
- **คอมไพล์**
  ```bash
  gcc E2E_test.c job.c unit_test.c -o e2e_test
  ```
- **รันโปรแกรม**
  ```bash
  ./e2e_test
  ```
- **รันและเทสต์ memory leak**
```
gcc -g -O0 E2E_test.c job.c unit_test.c -o applicant && valgrind --leak-check=full ./applicant
```
