#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <signal.h>
#include <unistd.h>
#include "Update.h"
#include "UserRegistry.h"
#include "Document.h"
#include "CRDTMerger.h"
#include "MessageQueue.h"

// Global variables
std::atomic<bool> running(true);
UserRegistry* global_registry = nullptr;
MessageQueue* global_mq = nullptr;

void signalHandler(int signum) {
    std::cout << "\n\nShutting down gracefully...\n";
    running.store(false);
    
    if (global_registry) {
        global_registry->unregisterUser();
    }
    
    if (global_mq) {
        global_mq->unlink();
    }
    
    exit(signum);
}

// Listener thread function
void listenerThread(MessageQueue* mq, std::vector<Update>* all_updates_buffer,
                   std::atomic<bool>* has_new_updates, UserRegistry* registry) {
    std::cout << "Listener thread started\n";
    
    while (running.load()) {
        Update update;
        if (mq->receive(update)) {
            all_updates_buffer->push_back(update);
            has_new_updates->store(true);
            
            // Don't increment global counter here - sender already incremented it
            int current_count = registry->getGlobalUpdateCount();
            std::cout << "\n[📨 Received update from " << update.user_id 
                      << " - Global: " << current_count << "/5]\n";
        }
        
        // Small sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Listener thread stopped\n";
}

// Broadcast updates to all other users
void broadcastUpdates(const std::vector<Update>& updates, UserRegistry& registry, 
                     const std::string& user_id) {
    auto active_users = registry.getActiveUsers();
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          📢 BROADCASTING " << updates.size() 
              << " CHANGES FROM " << user_id << "                 ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    
    for (size_t i = 0; i < updates.size(); i++) {
        const auto& update = updates[i];
        std::cout << "  [" << (i+1) << "] Line " << update.line_number 
                  << ", cols [" << update.col_start << "-" << update.col_end << "]: "
                  << "\"" << update.old_content << "\" → \"" 
                  << update.new_content << "\" (" << update.getTypeString() << ")\n";
    }
    
    std::cout << "\n📤 Sending to " << active_users.size() << " user(s): ";
    for (const auto& user_pair : active_users) {
        std::cout << user_pair.first << " ";
        MessageQueue mq;
        if (mq.open(user_pair.second)) {
            for (const auto& update : updates) {
                mq.send(update);
            }
            mq.close();
        }
    }
    std::cout << "\n✅ Broadcast complete!\n";
    std::cout << "════════════════════════════════════════════════════════════\n\n";
}

// Clear terminal screen
void clearScreen() {
    std::cout << "\033[2J\033[1;1H";
}

// Display current status
void displayStatus(const std::string& user_id, Document& doc, 
                  UserRegistry& registry, int local_updates, int global_updates) {
    clearScreen();
    
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              SyncText - CRDT Text Editor                   ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "User: " << user_id << " | ";
    
    auto active_users = registry.getActiveUsers();
    std::cout << "Active users: " << user_id;
    for (const auto& user : active_users) {
        std::cout << ", " << user.first;
    }
    std::cout << " (" << (active_users.size() + 1) << " total)\n";
    std::cout << "Local buffer: " << local_updates << "/5 | ";
    std::cout << "Global buffer: " << global_updates << "/5\n";
    
    doc.displayDocument();
    
    std::cout << "\n💡 Local: Broadcasts after 5 YOUR changes\n";
    std::cout << "💡 Global: Merges after 5 TOTAL changes from all users\n";
    std::cout << "\nMonitoring for changes... (Press Ctrl+C to exit)\n";
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <user_id>\n";
        std::cerr << "Example: " << argv[0] << " user_1\n";
        return 1;
    }

    std::string user_id = argv[1];
    std::string doc_filename = user_id + "_doc.txt";
    std::string queue_name = "/queue_" + user_id;

    // Setup signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize user registry
    UserRegistry registry;
    global_registry = &registry;
    
    if (!registry.initialize()) {
        std::cerr << "Failed to initialize user registry\n";
        return 1;
    }

    // Create message queue
    MessageQueue mq;
    global_mq = &mq;
    
    if (!mq.create(queue_name)) {
        std::cerr << "Failed to create message queue\n";
        return 1;
    }

    // Register user
    if (!registry.registerUser(user_id, queue_name)) {
        std::cerr << "Failed to register user. Maximum users reached or duplicate user_id.\n";
        return 1;
    }

    std::cout << "Registered as " << user_id << "\n";
    std::cout << "Message queue created: " << queue_name << "\n";

    // Copy initial document if it doesn't exist
    if (access(doc_filename.c_str(), F_OK) != 0) {
        // Check if initial_doc.txt exists
        if (access("initial_doc.txt", F_OK) != 0) {
            std::cerr << "Error: initial_doc.txt not found. Creating default document...\n";
            // Create a default initial document
            std::ofstream default_doc("initial_doc.txt");
            if (default_doc.is_open()) {
                default_doc << "int x = 10;\n";
                default_doc << "int y = 20;\n";
                default_doc << "int z = 30;\n";
                default_doc.close();
                std::cout << "Created initial_doc.txt with default content\n";
            } else {
                std::cerr << "Failed to create initial_doc.txt\n";
                return 1;
            }
        }
        
        // Copy initial document to user's document
        std::ifstream src("initial_doc.txt", std::ios::binary);
        std::ofstream dst(doc_filename, std::ios::binary);
        
        if (!src.is_open() || !dst.is_open()) {
            std::cerr << "Failed to copy initial document\n";
            return 1;
        }
        
        dst << src.rdbuf();
        src.close();
        dst.close();
        
        std::cout << "✅ Created local document: " << doc_filename << "\n";
    } else {
        std::cout << "📄 Using existing document: " << doc_filename << "\n";
    }

    // Initialize document
    Document doc(doc_filename, user_id);
    
    // Initialize CRDT merger
    CRDTMerger merger;

    // Buffers for updates
    std::vector<Update> local_updates;           // For broadcasting (this user's changes)
    std::vector<Update> all_updates_buffer;      // For merging (all changes: own + received)
    std::atomic<bool> has_new_updates(false);

    // Start listener thread
    std::thread listener(listenerThread, &mq, &all_updates_buffer, &has_new_updates, &registry);

    // Display initial status
    displayStatus(user_id, doc, registry, registry.getGlobalUpdateCount(), registry.getGlobalUpdateCount());

    const int MERGE_THRESHOLD = 5;

    // Main monitoring loop
    while (running.load()) {
        // Check for local changes
        std::vector<Update> detected = doc.detectChanges();
        
        if (!detected.empty()) {
            std::cout << "\n[🔍 " << detected.size() << " local change(s) detected]\n";
            
            for (const auto& update : detected) {
                // Add to all_updates buffer for merging
                all_updates_buffer.push_back(update);
                
                // Increment shared global counter
                int old_global_count = registry.incrementGlobalUpdateCount();
                int actual_global_count = old_global_count + 1;  // fetch_add returns old value
                
                std::cout << "  Line " << update.line_number 
                          << ", cols [" << update.col_start << "-" << update.col_end << "]: "
                          << "\"" << update.old_content << "\" → \"" 
                          << update.new_content << "\" (Global: " << actual_global_count << "/5)\n";
                
                // Broadcast IMMEDIATELY so all users have all updates
                std::cout << "[📢 Broadcasting change immediately]\n";
                std::vector<Update> single_update = {update};
                broadcastUpdates(single_update, registry, user_id);
            }
        }

        // Check if global counter has reached threshold for MERGE
        // ALL users should merge when threshold is reached
        static int last_merged_at_count = 0;
        static bool merge_in_progress = false;
        int current_global_count = registry.getGlobalUpdateCount();
        
        // Only merge when we hit exactly 5, 10, 15, etc. (every 5th update)
        if (current_global_count >= MERGE_THRESHOLD && 
            current_global_count >= last_merged_at_count + MERGE_THRESHOLD &&
            !merge_in_progress) {
            
            merge_in_progress = true;
            
            // Wait to ensure all broadcasts are received by all users
            std::cout << "\n[⏳ Waiting for all broadcasts to arrive...]\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
            std::cout << "║     🔄 GLOBAL MERGE TRIGGERED (" << current_global_count 
                      << " updates accumulated)        ║\n";
            std::cout << "╚════════════════════════════════════════════════════════════╝\n";
            
            if (!all_updates_buffer.empty()) {
                // Merge all updates in our buffer
                std::cout << "\n[Merging " << all_updates_buffer.size() 
                          << " updates from all sources using CRDT...]\n";
                
                // CRITICAL: Reload initial document state before applying merged updates
                // This ensures all users start from the same base state
                std::cout << "[Reloading initial document state...]\n";
                doc.loadFromInitialState();
                
                // Now apply all merged updates in resolved order
                merger.mergeUpdates(all_updates_buffer, doc);
                
                // Save merged document to file
                doc.saveToFile();
                
                // Small delay to ensure file is written
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // Reload document to ensure we have the latest state
                doc.loadFromFile();
                
                // Clear local buffer
                all_updates_buffer.clear();
                has_new_updates.store(false);
                
                std::cout << "\n✅ [Document synchronized and reloaded]\n";
                std::cout << "[Local update buffer cleared]\n";
            } else {
                std::cout << "\n[No local updates to merge, but syncing from other users...]\n";
                // We need to wait and see if we receive updates from others
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                if (!all_updates_buffer.empty()) {
                    // Received updates during wait, process them
                    std::cout << "[Processing " << all_updates_buffer.size() << " received updates...]\n";
                    doc.loadFromInitialState();
                    merger.mergeUpdates(all_updates_buffer, doc);
                    doc.saveToFile();
                    doc.loadFromFile();
                    all_updates_buffer.clear();
                }
            }
            
            // CRITICAL: Wait for ALL users to complete their merge before anyone resets counter
            // This ensures no user misses the merge trigger
            std::cout << "[⏳ Waiting for all users to complete merge...]\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            
            // Reset global counter for fresh start (only after all users merged)
            registry.resetGlobalUpdateCount();
            last_merged_at_count = 0;  // Reset to 0 for next batch
            merge_in_progress = false;
            
            std::cout << "[Global counter reset to 0 - starting fresh]\n";
            std::cout << "════════════════════════════════════════════════════════════\n\n";
        }

        // Update display periodically
        static int display_counter = 0;
        if (display_counter++ % 5 == 0) {
            displayStatus(user_id, doc, registry, 
                        registry.getGlobalUpdateCount(), registry.getGlobalUpdateCount());
        }

        // Sleep to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Cleanup
    running.store(false);
    if (listener.joinable()) {
        listener.join();
    }

    registry.unregisterUser();
    mq.unlink();

    std::cout << "\nGoodbye!\n";
    return 0;
}
