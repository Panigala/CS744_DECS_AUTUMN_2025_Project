# CS744_DECS_AUTUMN_2025_Project( Multi-Tier Key-Value HTTP System (C++) )

## Overview
This project implements a **multi-tier HTTP key-value store** using:
- **cpp-httplib** for HTTP server/client
- **MySQL** for persistent storage
- **In-memory LRU cache** for fast access


## Files
- `server.cpp`: HTTP server with REST (PUT, GET, DELETE,..) endpoints that can handle multiple clients concurrently
- `client.cpp`: Load generator to simulate concurrent clients
- `mysql_setup.sql`: MySQL setup script
- `tester.cpp`: for testing all server request responses
- `Makefile`: for running and setting-up , compiling
- `httplib.h`: for the httplib functionality

## Setup Instructions
1. **Install dependencies**
   ```bash
    make setup_all
    #make setup_mysql #only do mysql_setup ie sudo mysql -u root -p < mysql_setup.sql
    #make setup_dir  #only do directory setup loke bin ,results 
    #make setup_httplib  #download header only if not already present 

    #sudo apt update
    #sudo apt install wget curl
    #sudo apt install g++ make libmysqlclient-dev mysql-server mpstat sysstat iostat
   
    #sudo apt install -y mysql-server mysql-client
    #put root password something remebering coz we need it later
   
    #sudo systemctl enable mysql
    #sudo systemctl start mysql
   ```
 wget https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
 Place it in the same directory as `server.cpp` and `client.cpp`

2. **Compile**
   ```bash
    make #default make build_all
    make build_all
   #compile server and the actual multithreaded-client(load generator)
   #g++ -std=c++17 server.cpp -lmysqlclient -lpthread -o server
   #g++ -std=c++17 client.cpp -lpthread -o client
   ```
3. **Run**
   ```bash
   #--------Requests supported but server--------------
   # GET , PUT/POST , DELETE , popular , stats , health
   
   # put desired cpu core where this need to be run then the arguments are
   # parameters(arguments) number of threads, time to run, GET%, POST%, DELETE%, popular%    where all % should add to 100
   # eg : taskset -c 0-7 ./client 6000 10 0 10 90 0

    make run_server CPU=7  # running server on cpu core numbered 7
    make run_client CPU=0-9 PARAMS="10 10 20 30 20 30"  # running client on cpu numbered 0 to 9 and each parameters are as above mentioned
    make run_tester CPU=5  # for running the tester on given cpu
   
   ```
4. **Cleaning**
   ```bash
   #--------cleaning the binary or result or both--------------
    make clean #only bin ie, binary is cleared
    make clean_all #both binary and results cleared
   
   ```

