# ==========================================================
#                Makefile — Build and Setup 
# ==========================================================
# Directory Layout:
#   src/   → Source files (.cpp)
#   bin/   → Compiled binaries (server, client)
#   setup/ → Setup scripts (MySQL setup)
# ==========================================================

# Compiler and Flags
CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -pthread
LIBS := -lmysqlclient

# Directories
SRC_DIR := src
BIN_DIR := bin
SETUP_DIR := setup

# Source files 
SERVER_SRC := $(SRC_DIR)/server.cpp
CLIENT_SRC := $(SRC_DIR)/client.cpp
TESTER_SRC := $(SRC_DIR)/tester.cpp

# Destination folder
SERVER_BIN := $(BIN_DIR)/server
CLIENT_BIN := $(BIN_DIR)/client
TESTER_BIN := $(BIN_DIR)/tester





# ==========================================================
# Default Target
# ==========================================================
build_all: setup_dirs $(SERVER_BIN) $(CLIENT_BIN) $(TESTER_BIN)
	@echo "==============================================="
	@echo "   Build complete! Binaries stored in ./bin"
	@echo " - $(SERVER_BIN)"
	@echo " - $(CLIENT_BIN)"
	@echo " - $(TESTER_BIN)"
	@echo "==============================================="



# ==========================================================
# Starting System Setup (Dependencies + Directories + MySQL)
# ==========================================================
setup_all:
	@echo "==========================================================="
	@echo "  Starting System Setup (Dependencies + Directories + MySQL)"
	@echo "==========================================================="
	@echo "--------> Updating APT repositories..."
#	@sudo apt update -y
	@echo "-----------------------------------------------------------"
	@echo "--------> Installing required packages..."
	@sudo apt install -y wget curl g++ make libmysqlclient-dev mysql-server mysql-client sysstat
	@echo "-----------------------------------------------------------"
	@echo "    Ensured all dependencies are installed successfully"
	@echo "-----------------------------------------------------------"
	@mkdir -p $(BIN_DIR)
	@mkdir -p results
	@echo "Following directories"
	@echo "- bin/" 
	@echo "- results/"
	@echo "..........all Exists.........."
	@echo " "
	@echo "Setting up MySQL Database..."
	@sudo chmod +x $(SETUP_DIR)/mysql_setup.sql
	@sudo mysql -u root -p < $(SETUP_DIR)/mysql_setup.sql
# ==========================================================
# MySQL only Setup
# ==========================================================
setup_mysql:
	@echo "Setting up MySQL Database..."
	@sudo chmod +x $(SETUP_DIR)/mysql_setup.sql
	@sudo mysql -u root -p < $(SETUP_DIR)/mysql_setup.sql




# ==========================================================
# Build Rules
# ==========================================================
$(SERVER_BIN): $(SERVER_SRC)
	@echo "Compiling server..."
	@$(CXX) -std=c++17 $(SERVER_SRC) $(MYSQL_LIBS) $(LIBS) -o $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_SRC)
	@echo "Compiling client..."
	@$(CXX) -std=c++17 $(CLIENT_SRC) $(LIBS) -o $(CLIENT_BIN)

$(TESTER_BIN): $(TESTER_SRC)
	@echo "Compiling tester..."
	@$(CXX) -std=c++17 $(TESTER_SRC) $(LIBS) -o $(TESTER_BIN)




# ==========================================================
# Run Server default where server pinned to core 7
# Run Client defaault pinned to core 0-5
# ==========================================================
run_server: $(SERVER_BIN)
	@echo "Starting server..."
	@taskset -c 7 $(SERVER_BIN)

run_client: $(CLIENT_BIN)
	@echo "Running client..."
	@taskset -c 0-5 $(CLIENT_BIN) 10 10 50 20 20 10

run_tester: $(TESTER_BIN)
	@echo "Running tester..."
	@taskset -c 0-5 $(TESTER_BIN)




# ==========================================================
# Cleanup
# ==========================================================
clean:
	@echo "Cleaning up......."
	rm -f $(BIN_DIR)/*
	@echo "Cleaning complete!"

.PHONY: all setup_dirs clean run_server run_client setup_mysql
