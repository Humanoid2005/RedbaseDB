#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>

#define SERVER_PORT 8888
#define BUFFER_SIZE 65536  // 64KB buffer for large results

enum MessageType {
    MSG_QUERY = 1,
    MSG_RESULT = 2,
    MSG_ERROR = 3,
    MSG_EXIT = 4
};

struct QueryRequest {
    int type;           
    int query_length;   
    char query[4096];   
};


struct QueryResponse {
    int type;           
    int success;       
    int result_length;  
    double execution_time_ms;  
    int rows_affected; 
    char message[1024]; 
};

int send_all(int sd, const void *buf, int len) {
    int total = 0;
    const char *ptr = (const char*)buf;
    while (total < len) {
        int sent = write(sd, ptr + total, len - total);
        if (sent <= 0) return sent;
        total += sent;
    }
    return total;
}

int recv_all(int sd, void *buf, int len) {
    int total = 0;
    char *ptr = (char*)buf;
    while (total < len) {
        int recvd = read(sd, ptr + total, len - total);
        if (recvd <= 0) return recvd;
        total += recvd;
    }
    return total;
}

void print_separator(int width = 80) {
    std::cout << std::string(width, '=') << std::endl;
}

void display_help() {
    std::cout << "\n";
    print_separator();
    std::cout << "                    DATABASE CLIENT - HELP MENU" << std::endl;
    print_separator();
    std::cout << "\nSupported SQL Commands:\n" << std::endl;
    
    std::cout << "DATABASE OPERATIONS:" << std::endl;
    std::cout << "  CREATE DATABASE <db_name>    - Create a new database" << std::endl;
    std::cout << "  DROP DATABASE <db_name>      - Delete a database" << std::endl;
    std::cout << "  USE DATABASE <db_name>       - Switch to a database" << std::endl;
    std::cout << "  SHOW DATABASES               - List all databases" << std::endl;
    
    std::cout << "\nTABLE OPERATIONS:" << std::endl;
    std::cout << "  CREATE TABLE <table> (<cols>)  - Create a new table" << std::endl;
    std::cout << "  DROP TABLE <table>             - Delete a table" << std::endl;
    std::cout << "  SHOW TABLES                    - List all tables" << std::endl;
    std::cout << "  DESC <table>                   - Show table schema" << std::endl;
    
    std::cout << "\nINDEX OPERATIONS:" << std::endl;
    std::cout << "  CREATE INDEX <table>(<col>)    - Create an index" << std::endl;
    std::cout << "  DROP INDEX <table>(<col>)      - Delete an index" << std::endl;
    
    std::cout << "\nDATA OPERATIONS:" << std::endl;
    std::cout << "  SELECT * FROM <table> [WHERE <condition>]" << std::endl;
    std::cout << "  INSERT INTO <table> VALUES (<values>)" << std::endl;
    std::cout << "  UPDATE <table> SET <col>=<val> [WHERE <condition>]" << std::endl;
    std::cout << "  DELETE FROM <table> [WHERE <condition>]" << std::endl;
    
    std::cout << "\nDATA TYPES:" << std::endl;
    std::cout << "  INT       - 32-bit integer" << std::endl;
    std::cout << "  FLOAT     - 32-bit floating point" << std::endl;
    std::cout << "  CHAR(n)   - Fixed-length string" << std::endl;
    std::cout << "  DATETIME  - Date and time" << std::endl;
    
    std::cout << "\nCLIENT COMMANDS:" << std::endl;
    std::cout << "  help      - Show this help menu" << std::endl;
    std::cout << "  exit      - Disconnect and exit" << std::endl;
    std::cout << "  quit      - Disconnect and exit" << std::endl;
    
    print_separator();
    std::cout << std::endl;
}

int main() {
    struct sockaddr_in server;
    
    // Create socket
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd < 0) {
        std::cerr << "Error: Failed to create socket" << std::endl;
        return 1;
    }
    
    // Configure server address
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(SERVER_PORT);
    
    // Connect to server
    std::cout << "Connecting to database server at 127.0.0.1:" << SERVER_PORT << "..." << std::endl;
    if(connect(sd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        std::cerr << "Error: Failed to connect to server" << std::endl;
        std::cerr << "Make sure the database server is running!" << std::endl;
        close(sd);
        return 1;
    }
    
    print_separator();
    std::cout << "              DATABASE CLIENT - Connected Successfully" << std::endl;
    print_separator();
    std::cout << "Type 'help' for available commands, 'exit' to quit" << std::endl;
    std::cout << std::endl;
    
    // Main query loop
    while (true) {
        std::cout << "db> ";
        
        // Read query from user
        std::string query_line;
        std::getline(std::cin, query_line);
        
        // Skip empty lines
        if (query_line.empty()) {
            continue;
        }
        
        // Trim whitespace
        size_t start = query_line.find_first_not_of(" \t\n\r");
        size_t end = query_line.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) {
            continue;
        }
        query_line = query_line.substr(start, end - start + 1);
        
        // Check for client-side commands
        if (query_line == "exit" || query_line == "quit") {
            QueryRequest request;
            request.type = MSG_EXIT;
            request.query_length = 0;
            send_all(sd, &request, sizeof(QueryRequest));
            std::cout << "Disconnecting from server... Goodbye!" << std::endl;
            break;
        }
        
        if (query_line == "help") {
            display_help();
            continue;
        }
        
        // Prepare query request
        QueryRequest request;
        memset(&request, 0, sizeof(QueryRequest));
        request.type = MSG_QUERY;
        request.query_length = query_line.length();
        strncpy(request.query, query_line.c_str(), sizeof(request.query) - 1);
        
        // Send request to server
        auto start_time = std::chrono::high_resolution_clock::now();
        if (send_all(sd, &request, sizeof(QueryRequest)) <= 0) {
            std::cerr << "Error: Failed to send query to server" << std::endl;
            break;
        }
        
        // Receive response header
        QueryResponse response;
        memset(&response, 0, sizeof(QueryResponse));
        if (recv_all(sd, &response, sizeof(QueryResponse)) <= 0) {
            std::cerr << "Error: Failed to receive response from server" << std::endl;
            break;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double total_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        // Check response type
        if (response.type == MSG_ERROR) {
            std::cerr << "Error: " << response.message << std::endl;
            std::cout << "Query execution time: " << response.execution_time_ms << " ms" << std::endl;
            std::cout << std::endl;
            continue;
        }
        
        // Receive result data if any
        if (response.result_length > 0) {
            char *result_data = new char[response.result_length + 1];
            if (recv_all(sd, result_data, response.result_length) <= 0) {
                std::cerr << "Error: Failed to receive result data" << std::endl;
                delete[] result_data;
                break;
            }
            result_data[response.result_length] = '\0';
            
            // Display result
            std::cout << result_data;
            delete[] result_data;
        } else {
            // No result data (e.g., INSERT, UPDATE, DELETE)
            std::cout << response.message << std::endl;
        }
        
        // Display performance metrics
        std::cout << "\n";
        print_separator(50);
        std::cout << "Query execution time: " << response.execution_time_ms << " ms" << std::endl;
        std::cout << "Network + display time: " << total_time_ms << " ms" << std::endl;
        if (response.rows_affected > 0) {
            std::cout << "Rows affected: " << response.rows_affected << std::endl;
        }
        print_separator(50);
        std::cout << std::endl;
    }
    
    close(sd);
    return 0;
}
