# ==========================================================
#                Makefile — Build and Setup 
# ==========================================================
# Directory Layout:
#   src/   → Source files (.cpp)
#   bin/   → Compiled binaries (server, client)
#   setup/ → Setup scripts (MySQL setup)
# ==========================================================
#  make run_client CPU="0-5" PARAMS="20 30 40 20 30 10"

#  make load_test  LOAD=' "<number_of_thread  time_duration  GET%  PUT%  DELETE%  POPULAR% >"  <CPU_CLIENT>  <CPU_SERVER>  <CPU_STATOOL>  <INTERVAL_MPSTAT>  <INTERVAL_IOSTAT>  <INTERVAL_VMSTAT> '
#  Example : make load_test LOAD='  "1000 20 10 90 0 0"   0-5  6  7  1 1 1'

# Compiler and Flags
CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -pthread
LIBS := -lmysqlclient

# Directories
SRC_DIR := src
BIN_DIR := bin
SETUP_DIR := setup
RES_DIR := results

# Source files 
SERVER_SRC := $(SRC_DIR)/server.cpp
CLIENT_SRC := $(SRC_DIR)/client.cpp
TESTER_SRC := $(SRC_DIR)/tester.cpp

# Destination folder
SERVER_BIN := $(BIN_DIR)/server
CLIENT_BIN := $(BIN_DIR)/client
TESTER_BIN := $(BIN_DIR)/tester

# configurable runtime variables
CPU ?= 0-5                    #default CPU cores for taskset
PARAMS ?= 10 10 20 20 40 20   #default parameters taskset

# ==========================================================
#                  Default Target
# ==========================================================
build_all: setup_dirs $(SERVER_BIN) $(CLIENT_BIN) $(TESTER_BIN)
	@echo 
	@echo "   Build complete! Binaries stored in ./bin"
	@echo " - $(SERVER_BIN)"
	@echo " - $(CLIENT_BIN)"
	@echo " - $(TESTER_BIN)"
	@echo 
build_server: $(SERVER_BIN)
build_client: $(CLIENT_BIN)
build_tester: $(TESTER_BIN)
$(SERVER_BIN): $(SERVER_SRC)
	@echo "Compiling server..."
	@$(CXX) -std=c++17 $(SERVER_SRC) $(MYSQL_LIBS) $(LIBS) -o $(SERVER_BIN)
	@echo "done"

$(CLIENT_BIN): $(CLIENT_SRC)
	@echo "Compiling client..."
	@$(CXX) -std=c++17 $(CLIENT_SRC) $(LIBS) -o $(CLIENT_BIN)
	@echo "done"

$(TESTER_BIN): $(TESTER_SRC)
	@echo "Compiling tester..."
	@$(CXX) -std=c++17 $(TESTER_SRC) $(LIBS) -o $(TESTER_BIN)
	@echo "done"




# ==========================================================
# Starting System Setup (Dependencies + Directories + MySQL)
# ==========================================================
setup_all:
	@echo "==========================================================="
	@echo "  Starting System Setup (Dependencies + Directories + MySQL)"
	@echo "==========================================================="
	@echo ">>>>> Updating APT repositories..."
#	@sudo apt update -y
	@echo " "
	@echo "-----------------------------------------------------------"
	@echo ">>>>> Installing required packages..."
	@sudo apt install -y wget curl g++ make libmysqlclient-dev mysql-server mysql-client sysstat
	@echo "-----------------------------------------------------------"
	@echo "    Ensured all dependencies are installed successfully"
	@echo "-----------------------------------------------------------"
	@echo " "
	@mkdir -p $(BIN_DIR)
	@mkdir -p results
	@echo "Following directories"
	@echo "- bin/" 
	@echo "- results/"
	@echo "..........all Exists.........."
	@sudo chmod +x $(SRC_DIR)/load_test.sh
	@sudo chmod +x $(SETUP_DIR)/mysql_setup.sql
	@echo " "
	@echo "Setting up MySQL Database..."
	@sudo mysql -u root -p < $(SETUP_DIR)/mysql_setup.sql
	@echo "-----------------------------------------------------------"
	@echo " "
	@echo "Checking for httplib.h in $(SRC_DIR)..."
	@if [ ! -f "$(SRC_DIR)/httplib.h" ]; then \
		echo "httplib.h not found, downloading..."; \
		wget -q -O $(SRC_DIR)/httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h; \
		echo "Downloaded httplib.h to $(SRC_DIR)/"; \
	else \
		echo "httplib.h already exists in $(SRC_DIR). Skipping download."; \
	fi
# ==========================================================
#             MySQL, Directory only Setup
# ==========================================================
setup_mysql:
	@echo "Setting up MySQL Database..."
	@sudo chmod +x $(SETUP_DIR)/mysql_setup.sql
	@sudo mysql -u root -p < $(SETUP_DIR)/mysql_setup.sql
setup_dir:
	@mkdir -p $(BIN_DIR)
	@mkdir -p results
	@echo "Following directories"
	@echo "- bin/" 
	@echo "- results/"
	@echo "..........all Exists.........."
	@sudo chmod +x $(SETUP_DIR)/mysql_setup.sql
	@sudo chmod +x $(SRC_DIR)/load_test.sh
# ==========================================================
# Setup httplib — download header only if not already present
# ==========================================================
setup_httplib:
	@echo "Checking for httplib.h in $(SRC_DIR)..."
	@if [ ! -f "$(SRC_DIR)/httplib.h" ]; then \
		echo "httplib.h not found, downloading..."; \
		wget -q -O $(SRC_DIR)/httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h; \
		echo "Downloaded httplib.h to $(SRC_DIR)/"; \
	else \
		echo "httplib.h already exists in $(SRC_DIR). Skipping download."; \
	fi




# ==========================================================
# Run Server default where server or client  pinned to core 7
# ==========================================================
run_server: $(SERVER_BIN)
	@echo "Starting server..."
	@echo "taskset -c $(CPU) $(SERVER_BIN)"
	@taskset -c $(CPU) $(SERVER_BIN)

run_client: $(CLIENT_BIN)
	@echo "Running client..."
	@echo "Command: taskset -c $(CPU) $(CLIENT_BIN) $(PARAMS)"
	@taskset -c $(CPU) $(CLIENT_BIN) $(PARAMS)

run_tester: $(TESTER_BIN)
	@echo "Running tester..."
	@echo taskset -c $(CPU) $(TESTER_BIN)
	@taskset -c $(CPU) $(TESTER_BIN)




#..........................................................................................................................................
# ==========================================================
#                      Load testing
# ==========================================================
load_test:
	@echo "Running load_test.sh with args: $(LOAD)"
	@bash ./src/load_test.sh $(LOAD)
#USAGE:
                                
#   ./load_test.sh  "<client_args>"  <CPU_CLIENT>  <CPU_SERVER>  <CPU_STATOOL>  <INTERVAL_MPSTAT>  <INTERVAL_IOSTAT>  <INTERVAL_VMSTAT>
 
#    "<client_args>"       "number_of_thread  time_duration  cpu_for_client_run "
#    <CPU_CLIENT>          cpu_for_client_run
#    <CPU_SERVER>          cpu_for_server_run
#    <CPU_STATOOL>         cpu_for_statistic_tools_run
#    <INTERVAL_MPSTAT>     stat_tool_time_interval_that_mpstat_take_statistics
#    <INTERVAL_IOSTAT>     stat_tool_time_interval_that_iostat_take_statistics
#    <INTERVAL_VMSTAT>     stat_tool_time_interval_that_vmstat_take_statistics

# Example:
#   ./load_test.sh "100 10 0 100 0 0"  2-7  7  8  1 1 1 
#..........................................................................................................................................



# ==========================================================
#                     Cleanup
# ==========================================================
clean:
	@echo "Cleaning up......."
	rm -rf $(BIN_DIR)/*
	rm -rf $(RES_DIR)/*
	@echo "Cleaning complete!"	
clean_bin:
	@echo "Cleaning up......."
	rm -rf $(BIN_DIR)/*
	@echo "Cleaning complete!"
clean_results:
	@echo "Cleaning up......."
	rm -rf $(RES_DIR)/*
	@echo "Cleaning complete!"


.PHONY: all setup_dirs clean clean_bin clean_results run_server run_client setup_mysql load_test 
