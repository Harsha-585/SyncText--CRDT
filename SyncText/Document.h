#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <algorithm>
#include "Update.h"

class Document {
private:
    std::string filename;
    std::vector<std::string> lines;
    time_t last_modified;
    std::string user_id;

    // Find the differences between two strings at character level
    Update findLineDifference(int line_num, const std::string& old_line, 
                             const std::string& new_line, long long timestamp) {
        int min_len = std::min(old_line.length(), new_line.length());
        int start_diff = 0;
        
        // Find first difference
        while (start_diff < min_len && old_line[start_diff] == new_line[start_diff]) {
            start_diff++;
        }

        // If lines are identical
        if (start_diff == min_len && old_line.length() == new_line.length()) {
            return Update(); // No change
        }

        int old_end = old_line.length();
        int new_end = new_line.length();

        // Find last difference
        while (old_end > start_diff && new_end > start_diff && 
               old_line[old_end - 1] == new_line[new_end - 1]) {
            old_end--;
            new_end--;
        }

        std::string old_content = old_line.substr(start_diff, old_end - start_diff);
        std::string new_content = new_line.substr(start_diff, new_end - start_diff);

        OperationType op_type;
        if (old_content.empty()) {
            op_type = OperationType::INSERT;
        } else if (new_content.empty()) {
            op_type = OperationType::DELETE;
        } else {
            op_type = OperationType::REPLACE;
        }

        return Update(op_type, line_num, start_diff, 
                     old_content.empty() ? start_diff : old_end,
                     old_content, new_content, timestamp, user_id);
    }

public:
    Document(const std::string& fname, const std::string& uid) 
        : filename(fname), last_modified(0), user_id(uid) {
        loadFromFile();
    }

    bool loadFromFile() {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        lines.clear();
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();

        updateModificationTime();
        return true;
    }

    bool saveToFile() {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        for (const auto& line : lines) {
            file << line << "\n";
        }
        file.close();
        
        updateModificationTime();
        return true;
    }

    bool loadFromInitialState() {
        // Load from initial_doc.txt to reset to baseline
        std::ifstream file("initial_doc.txt");
        if (!file.is_open()) {
            return false;
        }

        lines.clear();
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();

        return true;
    }

    void updateModificationTime() {
        struct stat file_stat;
        if (stat(filename.c_str(), &file_stat) == 0) {
            last_modified = file_stat.st_mtime;
        }
    }

    bool hasChanged() {
        struct stat file_stat;
        if (stat(filename.c_str(), &file_stat) == 0) {
            return file_stat.st_mtime != last_modified;
        }
        return false;
    }

    std::vector<Update> detectChanges() {
        std::vector<Update> updates;
        
        if (!hasChanged()) {
            return updates;
        }

        // Read new content
        std::vector<std::string> new_lines;
        std::ifstream file(filename);
        if (!file.is_open()) {
            return updates;
        }

        std::string line;
        while (std::getline(file, line)) {
            new_lines.push_back(line);
        }
        file.close();

        long long timestamp = Update::getCurrentTimestamp();

        // Compare line by line
        size_t max_lines = std::max(lines.size(), new_lines.size());
        for (size_t i = 0; i < max_lines; i++) {
            std::string old_line = (i < lines.size()) ? lines[i] : "";
            std::string new_line = (i < new_lines.size()) ? new_lines[i] : "";

            if (old_line != new_line) {
                Update update = findLineDifference(i, old_line, new_line, timestamp);
                if (update.line_number >= 0 || !update.user_id.empty()) {
                    updates.push_back(update);
                }
            }
        }

        // Update stored content
        lines = new_lines;
        updateModificationTime();

        return updates;
    }

    void applyUpdate(const Update& update) {
        try {
            // Ensure we have enough lines
            while (lines.size() <= static_cast<size_t>(update.line_number)) {
                lines.push_back("");
            }

            std::string& line = lines[update.line_number];
            
            // Apply the update at the specific position
            if (update.type == OperationType::INSERT) {
                // INSERT: If position exists and has content, replace it; otherwise insert
                if (static_cast<size_t>(update.col_start) < line.length()) {
                    // Position exists with content - REPLACE the content at that position
                    // Remove as many characters as the new content length
                    int replace_len = std::min(static_cast<int>(update.new_content.length()), 
                                              static_cast<int>(line.length() - update.col_start));
                    if (replace_len > 0) {
                        line.erase(update.col_start, replace_len);
                    }
                    line.insert(update.col_start, update.new_content);
                } else {
                    // Position doesn't exist - pad with spaces if needed, then INSERT
                    if (static_cast<size_t>(update.col_start) > line.length()) {
                        line.append(update.col_start - line.length(), ' ');
                    }
                    line.insert(update.col_start, update.new_content);
                }
            } else if (update.type == OperationType::DELETE) {
                // Delete from col_start to col_end
                int len = update.col_end - update.col_start;
                if (static_cast<size_t>(update.col_start) < line.length() && len > 0) {
                    // Only delete what exists
                    int actual_len = std::min(len, static_cast<int>(line.length() - update.col_start));
                    line.erase(update.col_start, actual_len);
                }
            } else if (update.type == OperationType::REPLACE) {
                // Replace: delete old content, insert new content at same position
                int len = update.col_end - update.col_start;
                if (static_cast<size_t>(update.col_start) < line.length() && len > 0) {
                    // First erase the old content
                    int actual_len = std::min(len, static_cast<int>(line.length() - update.col_start));
                    line.erase(update.col_start, actual_len);
                }
                // Then insert the new content at that position
                if (static_cast<size_t>(update.col_start) > line.length()) {
                    line.append(update.col_start - line.length(), ' ');
                }
                line.insert(update.col_start, update.new_content);
            }
        } catch (const std::out_of_range& e) {
            std::cerr << "⚠️  Error applying update: " << e.what() 
                      << " (line: " << update.line_number 
                      << ", col: " << update.col_start << "-" << update.col_end 
                      << ", type: " << update.getTypeString() << ")\n";
        } catch (const std::exception& e) {
            std::cerr << "⚠️  Unexpected error applying update: " << e.what() << "\n";
        }
    }

    std::string getContent() const {
        std::ostringstream oss;
        for (size_t i = 0; i < lines.size(); i++) {
            oss << "Line " << i << ": " << lines[i] << "\n";
        }
        return oss.str();
    }

    const std::vector<std::string>& getLines() const {
        return lines;
    }

    void displayDocument() const {
        std::cout << "\n========================================\n";
        std::cout << "Document: " << filename << "\n";
        std::cout << "========================================\n";
        for (size_t i = 0; i < lines.size(); i++) {
            std::cout << "Line " << i << ": " << lines[i] << "\n";
        }
        std::cout << "========================================\n";
    }
};

#endif // DOCUMENT_H
