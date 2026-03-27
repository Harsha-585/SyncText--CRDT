#ifndef UPDATE_H
#define UPDATE_H

#include <string>
#include <chrono>
#include <sstream>
#include <cstring>

enum class OperationType {
    INSERT,
    DELETE,
    REPLACE
};

struct Update {
    OperationType type;
    int line_number;
    int col_start;
    int col_end;
    std::string old_content;
    std::string new_content;
    long long timestamp;
    std::string user_id;

    Update() : type(OperationType::REPLACE), line_number(0), col_start(0), 
               col_end(0), timestamp(0) {}

    Update(OperationType t, int line, int col_s, int col_e,
           const std::string& old_c, const std::string& new_c,
           long long ts, const std::string& uid)
        : type(t), line_number(line), col_start(col_s), col_end(col_e),
          old_content(old_c), new_content(new_c), timestamp(ts), user_id(uid) {}

    // Get current timestamp in microseconds
    static long long getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }

    // Serialize update to string for message passing
    std::string serialize() const {
        std::ostringstream oss;
        oss << static_cast<int>(type) << "|"
            << line_number << "|"
            << col_start << "|"
            << col_end << "|"
            << old_content.length() << "|" << old_content << "|"
            << new_content.length() << "|" << new_content << "|"
            << timestamp << "|"
            << user_id;
        return oss.str();
    }

    // Deserialize update from string
    static Update deserialize(const std::string& data) {
        Update update;
        std::istringstream iss(data);
        std::string token;
        
        // Type
        std::getline(iss, token, '|');
        update.type = static_cast<OperationType>(std::stoi(token));
        
        // Line number
        std::getline(iss, token, '|');
        update.line_number = std::stoi(token);
        
        // Column start
        std::getline(iss, token, '|');
        update.col_start = std::stoi(token);
        
        // Column end
        std::getline(iss, token, '|');
        update.col_end = std::stoi(token);
        
        // Old content
        std::getline(iss, token, '|');
        int old_len = std::stoi(token);
        update.old_content.resize(old_len);
        iss.read(&update.old_content[0], old_len);
        iss.ignore(1); // Skip delimiter
        
        // New content
        std::getline(iss, token, '|');
        int new_len = std::stoi(token);
        update.new_content.resize(new_len);
        iss.read(&update.new_content[0], new_len);
        iss.ignore(1); // Skip delimiter
        
        // Timestamp
        std::getline(iss, token, '|');
        update.timestamp = std::stoll(token);
        
        // User ID
        std::getline(iss, update.user_id);
        
        return update;
    }

    std::string getTypeString() const {
        switch(type) {
            case OperationType::INSERT: return "INSERT";
            case OperationType::DELETE: return "DELETE";
            case OperationType::REPLACE: return "REPLACE";
            default: return "UNKNOWN";
        }
    }
};

#endif // UPDATE_H
