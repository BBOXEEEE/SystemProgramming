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

### 과제06. 디렉토리 관리 툴 업그레이드
📌 [hw6/dir_manage.c](./hw6/dir_manage.c)
- 실습용 쉘에서 CTRL+C (SIGINT) 시그널을 무시하도록 구현한다.
  - `signal()` 함수 사용
- kill 명령어를 구현한다.
  - kill <signal num> <pid> 와 같이 입력이 주어진다.
  - signal num에 해당하는 시그널을 pid에 대해 실행한다.
- mmap()을 이용한 메모리 복사 방식의 `cp` 로 변경한다.
  - read(), write() 인 기존 파일 복사 방식을 변경한다.
  - mmap()을 이용한 원본 파일을 메모리 맵핑한다.
  - 복사할 파일을 생성하고, 원본 파일의 크기만큼 `ftruncate()`로 파일 크기를 확장한다.
  - memcpy()를 통해 메모리 복사 방식으로 구현한다.

### 과제07. 파이프를 이용한 단체 채팅방 서버
📌 [채팅방 서버 프로그램](./hw7/server.c)

📌 [클라이언트 프로그램](./hw7/client.c)
- 서버는 클라이언트 접속용 server pipe file을 생성하고 기다린다.
- 클라이언트는 server pipe file을 사용하여 클라이언트 접속을 서버에게 알린다. 이 때 클라이언트가 생성한 client pipe file 이름을 포함한 클라이언트 정보를 서버에게 알려준다.
- 서버는 client pipe file을 open하고 클라이언트 목록에 방금 접속한 클라이언트 정보를 추가한다.
- 클라이언트 이름, client pipe file descriptor
- 서버는 클라이언트가 접속할 때마다 클라이언트 정보를 화면에 출력한다.
- 서버에 접속한 클라이언트 중 하나가 메시지를 입력하면 서버는 해당 메시지를 모든 클라이언트에게 전달한다.
- 클라이언트는 서버로부터 메시지를 받으면 화면에 해당 메시지를 출력한다.
-  파이프는 1:N 통신이 가능하다.
- 클라이언트는 자식 프로세스를 생성해서 서버로 부터 오는 메시지를 받는 역할만 수행하도록 구현할 수 있다. → 부모 프로세스는 사용자의 입력 메시지를 서버에 전달하는 역할만 수행한다.
- 파이프를 open() 할 때, O_RDONLY나 O_WRONLY 옵션을 주게 되면 통신할
프로세스가 사라질 경우 또는 통신하는 상대방이 파이프를 close() 할 경우 read()나 write()에서 0이나 -1이 리턴됨. 이를 통해 클라이언트 접속이 끊김을 확인할 수 있음.