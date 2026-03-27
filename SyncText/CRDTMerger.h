#ifndef CRDT_MERGER_H
#define CRDT_MERGER_H

#include <vector>
#include <algorithm>
#include <iostream>
#include "Update.h"
#include "Document.h"

class CRDTMerger {
private:
    // Check if two updates conflict
    bool hasConflict(const Update& u1, const Update& u2) {
        // Must be on same line
        if (u1.line_number != u2.line_number) {
            return false;
        }

        // Check for overlapping column ranges
        int u1_start = u1.col_start;
        int u1_end = u1.col_end;
        int u2_start = u2.col_start;
        int u2_end = u2.col_end;

        // No overlap if one range ends before the other starts
        if (u1_end <= u2_start || u2_end <= u1_start) {
            return false;
        }

        return true;
    }

    // Resolve conflict using Last-Writer-Wins strategy
    Update resolveConflict(const Update& u1, const Update& u2) {
        // Primary: Latest timestamp wins
        if (u1.timestamp != u2.timestamp) {
            return (u1.timestamp > u2.timestamp) ? u1 : u2;
        }

        // Tiebreaker: Smaller user_id wins
        return (u1.user_id < u2.user_id) ? u1 : u2;
    }

public:
    // Merge updates and apply to document
    std::vector<Update> mergeUpdates(std::vector<Update>& updates, Document& doc) {
        if (updates.empty()) {
            return {};
        }

        std::vector<Update> resolved_updates;
        std::vector<bool> processed(updates.size(), false);

        // Process each update
        for (size_t i = 0; i < updates.size(); i++) {
            if (processed[i]) continue;

            Update winner = updates[i];

            // Check for conflicts with other updates
            for (size_t j = i + 1; j < updates.size(); j++) {
                if (processed[j]) continue;

                if (hasConflict(winner, updates[j])) {
                    Update resolved = resolveConflict(winner, updates[j]);
                    
                    // Mark loser as processed
                    if (resolved.timestamp == winner.timestamp && 
                        resolved.user_id == winner.user_id) {
                        processed[j] = true;
                        std::cout << "Conflict detected: Line " << updates[j].line_number 
                                  << " - " << winner.user_id << " wins over " 
                                  << updates[j].user_id << "\n";
                    } else {
                        processed[i] = true;
                        winner = resolved;
                        std::cout << "Conflict detected: Line " << winner.line_number 
                                  << " - " << updates[j].user_id << " wins over " 
                                  << winner.user_id << "\n";
                    }
                }
            }

            if (!processed[i]) {
                resolved_updates.push_back(winner);
                processed[i] = true;
            }
        }

        // Sort by line number and column for consistent application
        std::sort(resolved_updates.begin(), resolved_updates.end(),
            [](const Update& a, const Update& b) {
                if (a.line_number != b.line_number) {
                    return a.line_number < b.line_number;
                }
                return a.col_start < b.col_start;
            });

        // Apply updates to document
        for (const auto& update : resolved_updates) {
            doc.applyUpdate(update);
        }

        return resolved_updates;
    }

    // Display merge results
    void displayMergeResults(const std::vector<Update>& updates) {
        if (updates.empty()) {
            std::cout << "No updates to merge.\n";
            return;
        }

        std::cout << "\n--- Merged Updates ---\n";
        for (const auto& update : updates) {
            std::cout << "User: " << update.user_id 
                      << " | Line: " << update.line_number
                      << " | Cols: [" << update.col_start << "-" << update.col_end << "]"
                      << " | Type: " << update.getTypeString()
                      << " | \"" << update.old_content << "\" → \"" 
                      << update.new_content << "\"\n";
        }
        std::cout << "----------------------\n";
    }
};

#endif // CRDT_MERGER_H
