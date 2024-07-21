# A simple C++ lightweight WebServer


### 1. Introduction
    //TODO

### 2. Environment
    WSL - Ubuntu 22.04
    MySQL 8.0.37
    C++ 11  
### 3. Usage
    1. Establish a MySQL database and table and modify the `code/main.cpp` file to connect to the database.
    2. Run `make` to compile the project.
    3. Run `./build/server` to start the server, and the server will listen on port 1316.
    4. Use `webbench` to test the server.
### 4. Directory Structure
```
.
├── Makefile
├── README.md
├── build
├── code
│   ├── Buffer
│   ├── Epoller
│   ├── HeapTimer
│   ├── HttpConn
│   ├── HttpRequest
│   ├── HttpResponse
│   ├── Log
│   ├── SQL
│   ├── Server
│   ├── ThreadPool
│   └── main.cpp
├── log
├── resources
├── test
└── webbench-1.5
```
