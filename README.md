# ⚙️ System Programming
🚀 2023학년도 2학기 시스템프로그래밍 과제 Repo

### 과제01. 리눅스에 SSH 서버 설치하기
📌 [hw1](./hw1)

### 과제02. GDB를 활용해 쉘 실행하기
📌 [hw2](./hw2/)
- GDB를 활용해 실행 흐름을 바꿔 쉘을 실행한다.
- 소스코드 상에서 함수 호출과 함수의 인자를 `system` 과 `/bin/sh` 로 변경한다.
- 적절한 BreakPoint 설정과 레지스터 값을 조회할 수 있어야한다.

### 과제03. 디렉토리 관리 툴 만들기
📌 [hw3/dir_manage.c](./hw3/dir_manage.c)
- 리눅스 상에서 쉘과 동일한 기능을 하는 디렉토리 관리 툴을 개발한다.
- 제공하는 명령어 목록
  - help
  - cd <path>
  - mkdir <path>
  - rmdir <path>
  - rename <source> <target>
- 모든 기능은 수행 완료 시 성공/실패 여부를 출력한다.
  - 실패 시 에러 원인을 `perror()` 를 통해 출력한다.
- 프로그램 상에서 최상위 디렉토리(/)는 `/tmp/test` 로 설정한다.
  - 프로그램 상에서 최상위 디렉토리로 이동/생성/삭제/읽기가 불가능하도록 기능을 제한한다.
- 프로그램 실행 시 `/tmp/test` 디렉토리가 존재하지 않는다면 디렉토리를 생성한다.

### 과제04. 디렉토리 관리 툴 업그레이드
📌 [hw4/dir_manage.c](./hw4/dir_manage.c)
- ls 명령 출력 내용에 보여지는 내용을 추가한다.
  - 파일 종류, uid, gid, 접근/수정/생성 시간의 값, 하드 링크 수, 파일의 크기, 파일 이름
- ln 명령어를 통한 하드 링크와 심볼릭 링크 생성 기능을 추가한다.
  - ls 명령에서 `target.sym -> origin.txt` 와 같이 파일명 형식을 변경한다.
- rm 명령어를 통한 파일 삭제 기능을 추가한다.
- chmod 명령어를 통한 파일의 접근 권한 변경 기능을 추가한다.
  - 접근 권한은 8진수, "ug+rwx" 와 같은 모든 형태로 표현 가능하도록 구현한다.
- cat 명령어를 통한 파일 읽기 기능을 추가한다.
- cp 명령어를 통한 파일을 복사하는 기능을 추가한다.

### 과제05. 디렉토리 관리 툴 업그레이드
📌 [hw5/dir_manage.c](./hw5/dir_manage.c)
- 숫자 값인 uid, gid를 문자로 변환한다.
- 파일의 생성/접근/수정 시간을 숫자 값에서 사용자가 읽기 편한 문자열로 변환한다.
- ps 명령어를 통한 `/proc` 디렉토리를 열어 프로세스 목록을 보여주는 기능을 추가한다.
  - PID, 프로세스 이름, 프로세스 실행 명령(cmdline)을 출력하도록 구현한다.
- 프로그램 상에서 환경 변수를 설정하거나 확인할 수 있도록 구현한다.
  - `TEST=test` 와 같은 형태로 환경 변수를 설정할 수 있도록 구현한다.
  - 모든 환경 변수를 확인하는 `env` 기능을 구현한다.
  - `echo` 명령어를 통해 환경 변수는 값으로 치환해서 출력, 일반 문자열은 그대로 출력하도록 구현한다.
  - `unset` 명령어를 통해 환경 변수를 제거하는 기능을 구현한다.