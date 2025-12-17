#include "database_engine/interpreter.h"
#include "database_engine/system_management/sm.h"
#include "database_engine/parser/parser.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <pthread.h>
#include <vector>
#include <atomic>

#define SERVER_PORT 8888
#define BUFFER_SIZE 65536
#define MAX_THREADS 100

// Thread-safe logging mutex
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Active client counter
std::atomic<int> active_clients(0);

// Message types
enum MessageType {
    MSG_QUERY = 1,
    MSG_RESULT = 2,
    MSG_ERROR = 3,
    MSG_EXIT = 4
};

// Request structure
struct QueryRequest {
    int type;
    int query_length;
    char query[4096];
};

// Response structure  
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

std::string format_show_databases_result(const ShowDatabasesResult& result) {
    std::ostringstream oss;
    oss << "\nDatabases:\n";
    oss << std::string(40, '-') << "\n";
    
    if (result.databases.empty()) {
        oss << "(No databases found)\n";
    } else {
        for (size_t i = 0; i < result.databases.size(); i++) {
            oss << "  " << (i + 1) << ". " << result.databases[i] << "\n";
        }
    }
    oss << std::string(40, '-') << "\n";
    return oss.str();
}

// Format show tables result
std::string format_show_tables_result(const ShowTablesResult& result) {
    std::ostringstream oss;
    oss << "Tables:\n";
    if (result.tables.empty()) {
        oss << "  (No tables)\n";
    } else {
        for (size_t i = 0; i < result.tables.size(); i++) {
            oss << "  " << (i + 1) << ". " << result.tables[i].table_name << "\n";
        }
    }
    return oss.str();
}

std::string format_desc_table_result(const DescTableResult& result) {
    std::ostringstream oss;
    oss << "\nTable: " << result.table_name << "\n";
    oss << std::string(50, '-') << "\n";
    oss << std::left << std::setw(20) << "Field" 
        << std::setw(20) << "Type" 
        << std::setw(10) << "Index" << "\n";
    oss << std::string(50, '-') << "\n";
    
    for (const auto& col : result.columns) {
        oss << std::left << std::setw(20) << col.field_name
            << std::setw(20) << col.type_name
            << std::setw(10) << col.has_index << "\n";
    }
    oss << std::string(50, '-') << "\n";
    return oss.str();
}

std::string format_select_result(const SelectResult& result) {
    std::ostringstream oss;
    
    if (result.rows.empty()) {
        oss << "\n(Empty result set)\n";
        return oss.str();
    }
    
    // Calculate column widths
    std::vector<size_t> col_widths;
    for (const auto& col_name : result.column_names) {
        col_widths.push_back(std::max(col_name.length(), size_t(15)));
    }
    
    // Update widths based on data
    for (const auto& row : result.rows) {
        for (size_t i = 0; i < row.values.size() && i < col_widths.size(); i++) {
            col_widths[i] = std::max(col_widths[i], row.values[i].value.length());
        }
    }
    
    // Calculate total width
    size_t total_width = 1;  // Start with 1 for first '|'
    for (auto w : col_widths) {
        total_width += w + 3;  // width + " | "
    }
    
    // Print header
    oss << "\n" << std::string(total_width, '-') << "\n";
    oss << "|";
    for (size_t i = 0; i < result.column_names.size(); i++) {
        oss << " " << std::left << std::setw(col_widths[i]) << result.column_names[i] << " |";
    }
    oss << "\n" << std::string(total_width, '-') << "\n";
    
    // Print rows
    for (const auto& row : result.rows) {
        oss << "|";
        for (size_t i = 0; i < row.values.size(); i++) {
            oss << " " << std::left << std::setw(col_widths[i]) << row.values[i].value << " |";
        }
        oss << "\n";
    }
    
    oss << std::string(total_width, '-') << "\n";
    oss << "(" << result.rows.size() << " row" << (result.rows.size() != 1 ? "s" : "") << ")\n";
    
    return oss.str();
}

// Thread-safe logging function
void safe_log(const std::string& message) {
    pthread_mutex_lock(&log_mutex);
    std::cout << message << std::flush;
    pthread_mutex_unlock(&log_mutex);
}

// Structure to pass client info to thread
struct ClientInfo {
    int socket;
    std::string ip_address;
    int port;
    int client_id;
};

// Thread function to handle individual client
void* client_thread_handler(void* arg) {
    ClientInfo* client_info = static_cast<ClientInfo*>(arg);
    int client_sd = client_info->socket;
    int client_id = client_info->client_id;
    
    // Increment active clients counter
    active_clients++;
    
    std::ostringstream oss;
    oss << "[Client #" << client_id << " connected] Socket: " << client_sd 
        << ", IP: " << client_info->ip_address 
        << ", Port: " << client_info->port 
        << " (Active clients: " << active_clients.load() << ")\n";
    safe_log(oss.str());
    safe_log(oss.str());
    
    while (true) {
        QueryRequest request;
        memset(&request, 0, sizeof(QueryRequest));
        
        // Receive request
        int bytes_read = recv_all(client_sd, &request, sizeof(QueryRequest));
        if (bytes_read <= 0) {
            oss.str("");
            oss << "[Client #" << client_id << " disconnected] Socket: " << client_sd << "\n";
            safe_log(oss.str());
            break;
        }
        
        // Check for exit message
        if (request.type == MSG_EXIT) {
            oss.str("");
            oss << "[Client #" << client_id << " exit request] Socket: " << client_sd << "\n";
            safe_log(oss.str());
            break;
        }
        
        // Process query
        std::string query(request.query, request.query_length);
        oss.str("");
        oss << "[Client #" << client_id << " Query] \"" << query << "\"\n";
        safe_log(oss.str());
        
        QueryResponse response;
        memset(&response, 0, sizeof(QueryResponse));
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            // Parse the SQL query
            auto parse_tree = parse_sql(query);
            
            // Execute query using interpreter
            InterpreterResult result = Interpreter::interp_sql(parse_tree);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            response.execution_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
            
            response.type = MSG_RESULT;
            response.success = result.success ? 1 : 0;
            response.rows_affected = 0;
            
            std::string result_text;
            
            // Format result based on command type
            switch (result.command_type) {
                case CMD_SELECT: {
                    auto select_result = std::get<SelectResult>(result.data);
                    result_text = format_select_result(select_result);
                    response.rows_affected = select_result.rows.size();
                    break;
                }
                
                case CMD_SHOW_DATABASES: {
                    auto show_db_result = std::get<ShowDatabasesResult>(result.data);
                    result_text = format_show_databases_result(show_db_result);
                    response.rows_affected = show_db_result.databases.size();
                    break;
                }
                
                case CMD_SHOW_TABLES: {
                    auto show_tables_result = std::get<ShowTablesResult>(result.data);
                    result_text = format_show_tables_result(show_tables_result);
                    response.rows_affected = show_tables_result.tables.size();
                    break;
                }
                
                case CMD_DESC_TABLE: {
                    auto desc_result = std::get<DescTableResult>(result.data);
                    result_text = format_desc_table_result(desc_result);
                    response.rows_affected = desc_result.columns.size();
                    break;
                }
                
                case CMD_INSERT: {
                    auto insert_result = std::get<InsertResult>(result.data);
                    strncpy(response.message, insert_result.message.c_str(), sizeof(response.message) - 1);
                    response.rows_affected = 1;
                    break;
                }
                
                case CMD_DELETE: {
                    auto delete_result = std::get<DeleteResult>(result.data);
                    strncpy(response.message, delete_result.message.c_str(), sizeof(response.message) - 1);
                    response.rows_affected = delete_result.affected_rows;
                    break;
                }
                
                case CMD_UPDATE: {
                    auto update_result = std::get<UpdateResult>(result.data);
                    strncpy(response.message, update_result.message.c_str(), sizeof(response.message) - 1);
                    response.rows_affected = update_result.affected_rows;
                    break;
                }
                
                case CMD_CREATE_TABLE:
                case CMD_DROP_TABLE:
                case CMD_CREATE_INDEX:
                case CMD_DROP_INDEX: {
                    auto ddl_result = std::get<DDLResult>(result.data);
                    strncpy(response.message, ddl_result.message.c_str(), sizeof(response.message) - 1);
                    break;
                }
                
                case CMD_CREATE_DATABASE:
                case CMD_DROP_DATABASE:
                case CMD_USE_DATABASE: {
                    auto db_result = std::get<DatabaseOperationResult>(result.data);
                    strncpy(response.message, db_result.message.c_str(), sizeof(response.message) - 1);
                    break;
                }
                
                case CMD_HELP: {
                    auto help_result = std::get<HelpResult>(result.data);
                    result_text = help_result.help_text;
                    break;
                }
                
                default:
                    strncpy(response.message, "Query executed successfully", sizeof(response.message) - 1);
                    break;
            }
            
            // Send response header
            response.result_length = result_text.length();
            send_all(client_sd, &response, sizeof(QueryResponse));
            
            // Send result data if any
            if (response.result_length > 0) {
                send_all(client_sd, result_text.c_str(), result_text.length());
            }
            
            oss.str("");
            oss << "[Client #" << client_id << " Query executed] Time: " << response.execution_time_ms << " ms, "
                      << "Rows: " << response.rows_affected << "\n";
            safe_log(oss.str());
            
        } catch (const std::exception& e) {
            auto end_time = std::chrono::high_resolution_clock::now();
            response.execution_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
            
            response.type = MSG_ERROR;
            response.success = 0;
            response.result_length = 0;
            strncpy(response.message, e.what(), sizeof(response.message) - 1);
            
            send_all(client_sd, &response, sizeof(QueryResponse));
            
            oss.str("");
            oss << "[Client #" << client_id << " Query error] " << e.what() << "\n";
            safe_log(oss.str());
        }
    }
    
    close(client_sd);
    
    // Decrement active clients counter
    active_clients--;
    
    oss.str("");
    oss << "[Client #" << client_id << " thread terminating] (Active clients: " << active_clients.load() << ")\n";
    safe_log(oss.str());
    
    // Clean up client info
    delete client_info;
    
    pthread_exit(NULL);
    return NULL;
}

int main() {
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);
    int client_id_counter = 0;
    std::vector<pthread_t> thread_ids;
    
    // Create socket
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        std::cerr << "Error: Failed to create socket" << std::endl;
        return 1;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error: Failed to set socket options" << std::endl;
        close(sd);
        return 1;
    }
    
    // Configure server address
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(SERVER_PORT);
    
    // Bind socket
    if (bind(sd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        std::cerr << "Error: Failed to bind socket to port " << SERVER_PORT << std::endl;
        close(sd);
        return 1;
    }
    
    // Listen for connections
    if (listen(sd, 5) < 0) {
        std::cerr << "Error: Failed to listen on socket" << std::endl;
        close(sd);
        return 1;
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "    DATABASE SERVER - Started" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Listening on port: " << SERVER_PORT << std::endl;
    std::cout << "Max concurrent clients: " << MAX_THREADS << std::endl;
    std::cout << "Waiting for client connections..." << std::endl;
    std::cout << "Press Ctrl+C to stop the server" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Accept and handle clients concurrently
    while (true) {
        int client_sd = accept(sd, (struct sockaddr *)&client, &client_len);
        if (client_sd < 0) {
            std::cerr << "Error: Failed to accept client connection" << std::endl;
            continue;
        }
        
        // Get client IP address
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        client_id_counter++;
        
        std::cout << "[New connection] Client #" << client_id_counter 
                  << ", IP: " << client_ip 
                  << ", Port: " << ntohs(client.sin_port) << std::endl;
        
        // Create client info structure
        ClientInfo* client_info = new ClientInfo();
        client_info->socket = client_sd;
        client_info->ip_address = std::string(client_ip);
        client_info->port = ntohs(client.sin_port);
        client_info->client_id = client_id_counter;
        
        // Create thread to handle client
        pthread_t thread_id;
        int result = pthread_create(&thread_id, NULL, client_thread_handler, (void*)client_info);
        
        if (result != 0) {
            std::cerr << "Error: Failed to create thread for client #" << client_id_counter << std::endl;
            close(client_sd);
            delete client_info;
            continue;
        }
        
        // Detach thread so it cleans up automatically when done
        pthread_detach(thread_id);
        thread_ids.push_back(thread_id);
        
        // Optional: limit concurrent threads
        if (thread_ids.size() > MAX_THREADS) {
            // Clean up old thread IDs (they're detached so this is just bookkeeping)
            thread_ids.erase(thread_ids.begin());
        }
    }
    
    // Cleanup mutex before exit (this code is unreachable in current design)
    pthread_mutex_destroy(&log_mutex);
    
    close(sd);
    return 0;
}
