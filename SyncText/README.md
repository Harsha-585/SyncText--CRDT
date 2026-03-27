# SyncText - CRDT-Based Collaborative Text Editor

A real-time collaborative text editor enabling 3-5 users to edit documents simultaneously without conflicts, using Conflict-Free Replicated Data Types (CRDT) with Last-Writer-Wins (LWW) strategy.

## Project Overview

SyncText is a real-time collaborative text editor built using Conflict-Free Replicated Data Types (CRDT) for lock-free synchronization. Multiple users can edit their local copies of a document simultaneously, and changes are automatically detected, broadcast, and merged without conflicts.

## Features

## Features

- ✅ **Real-time Collaboration**: Multiple users edit concurrently- **Multiple Concurrent Users**: Supports 3-5 users editing simultaneously

- ✅ **Automatic Conflict Resolution**: CRDT with Last-Writer-Wins strategy- **Lock-Free Design**: Uses CRDT principles with atomic operations (no locks)

- ✅ **Lock-Free Design**: Atomic operations, no mutexes- **Automatic Change Detection**: Monitors files for modifications using file system metadata

- ✅ **Immediate Broadcasting**: Changes sent instantly to all users- **Real-Time Synchronization**: Changes propagate via POSIX message queues

- ✅ **Consistent Merging**: All users converge to identical state- **Conflict Resolution**: Last-Writer-Wins (LWW) strategy with timestamp-based resolution

- ✅ **Graceful Error Handling**: Bounds checking prevents crashes- **Inter-Process Communication**: Shared memory for user registry, message queues for updates

- ✅ **macOS Compatible**: UNIX domain sockets for IPC

## System Requirements

## Quick Start- **Operating System**: Linux or macOS

- **Compiler**: g++ with C++17 support

### Build- **Libraries**: POSIX threads, POSIX message queues, POSIX shared memory

```bash

make ## Building the Project

```

### On macOS (M4 MacBook Air)

### Run```bash

Open 3 terminal windows:
```
make
```
**Terminal 1:**
```
./editor user_1

```

**Terminal 2:**

```

./editor user_2
```

**Terminal 3:**

```

./editor user_3

```
### Starting Multiple Users



### Edit DocumentsOpen separate terminal windows and run:

- Each user gets their own file: `user_1_doc.txt`, `user_2_doc.txt`, etc.

- Edit your file using any text editor (vim, nano, VS Code, etc.)**Terminal 1:**

- Changes are detected automatically every 2 seconds```bash

- Updates broadcast immediately to all users./editor user_1

- After 5 total changes across all users, automatic merge happens```

- All documents become identical after merge

**Terminal 2:**

### Cleanup```bash

If you encounter issues or want to start fresh:./editor user_2

```bash```

./cleanup.sh

```**Terminal 3:**

```bash

This removes:./editor user_3

- Socket files (`/tmp/queue_*.sock`)```

- User documents (`user_*_doc.txt`)

- Shared memory (`/synctext_registry`)Each user will:

1. Register in the shared user registry

## How It Works2. Create their own message queue (`/queue_user_X`)

3. Create a local document copy (`user_X_doc.txt`)

### Architecture4. Start monitoring for changes



```### Editing Documents

User edits file → Detect change → Create Update → Broadcast immediately

                                                         ↓1. Open your local document file in any text editor:

                                            All users receive update   - `user_1_doc.txt`, `user_2_doc.txt`, etc.

                                                         ↓   - Use vim, nano, TextEdit, VS Code, or any editor

                                            Increment global counter

                                                         ↓2. Make changes to the file

                                    When counter reaches 5 → MERGE

                                                         ↓3. Save the file

                        All users: Reload initial state → Apply all updates

                                                         ↓4. The terminal will automatically:

                                    Result: Identical documents   - Detect the changes

```   - Display what changed (line, column, content)

   - Broadcast updates to other users after 5 operations

### Key Concepts   - Receive and merge updates from others



**1. Immediate Broadcasting**### Example Workflow

- Every change broadcasts immediately (not batched)

- Ensures all users have all updates before merge**Initial Document (all users start with this):**

```

**2. Global Counter**Line 0: int x = 10;

- Atomic counter in shared memoryLine 1: int y = 20;

- Only the sender increments (receivers just read)Line 2: int z = 30;

- When count >= 5: all users trigger merge```



**3. Merge from Initial State****User 1 changes Line 0:**

- Problem: Each user's document is already diverged```

- Solution: Reload from `initial_doc.txt` baselineint x = 50;  (changed 10 to 50)

- Apply all accumulated updates via CRDT```

- Result: All users converge to same state

**User 2 changes Line 1:**

**4. CRDT Conflict Resolution**```

- If two users edit same position: Latest timestamp winsint y = 100;  (changed 20 to 100)

- Tiebreaker: Smaller user_id```

- Deterministic: All users reach same result

**Result (all users converge to):**

**5. Bounds Checking**```

- DELETE operations may reference non-existent positionsLine 0: int x = 50;

- Solution: Check bounds before all string operationsLine 1: int y = 100;

- Catch exceptions, log errors, continue gracefullyLine 2: int z = 30;

```

### Data Flow

### Handling Conflicts

```

1. user_1 types "a" → Global count: 0→1 (broadcast)If two users edit the same location:

2. user_2 types "b" → Global count: 1→2 (broadcast)

3. user_3 types "c" → Global count: 2→3 (broadcast)**User 1 edits Line 0** at timestamp 14:06:10:

4. user_1 types "d" → Global count: 3→4 (broadcast)```

5. user_2 types "e" → Global count: 4→5 (broadcast) ← TRIGGER!int x = 75;

```

MERGE PHASE:

- All users detect count >= 5**User 2 edits Line 0** at timestamp 14:06:12:

- Wait 1 second (broadcasts arrive)```

- Reload from initial_doc.txtint x = 99;

- Apply all 5 updates via CRDT```

- Save to user_X_doc.txt

- Wait 2 seconds (all users finish)**Resolution:**

- Reset counter to 0- User 2's timestamp is later → User 2 wins

- All users converge to: `int x = 99;`

Result: All user files are identical- Terminal displays: "Conflict detected and resolved using LWW"

```

## Stopping the Editor

## Files

Press `Ctrl+C` in any terminal to stop that user's editor. The program will:

### Core Components- Unregister from the user registry

- **Update.h**: Update structure with serialization- Clean up message queue

- **UserRegistry.h**: Shared memory user management- Save the current document state

- **Document.h**: File monitoring and change detection

- **CRDTMerger.h**: Conflict resolution with LWW## Cleanup

- **MessageQueue.h**: UNIX socket IPC

- **editor.cpp**: Main programTo remove all generated files and IPC resources:

```bash

### Configurationmake clean

- **Makefile**: Build configuration```

- **initial_doc.txt**: Default document content

- **cleanup.sh**: Resource cleanup scriptThis removes:

- The compiled `editor` binary

### Documentation- All `*_doc.txt` files

- **DESIGNDOC**: Detailed architecture and design decisions- Message queues in `/dev/mqueue/`

- **README.md**: This file- Shared memory segments



## System Requirements## Project Structure



- **OS**: macOS or Linux```

- **Compiler**: g++ with C++17 support.

- **Libraries**: POSIX threads, shared memory├── editor.cpp           # Main program with threading logic

├── Update.h            # Update object structure and serialization

## Usage Examples├── UserRegistry.h      # Shared memory user registry

├── Document.h          # Document management and change detection

### Example 1: Simple Collaboration├── CRDTMerger.h        # CRDT merge algorithm implementation

├── MessageQueue.h      # POSIX message queue wrapper

```bash├── Makefile            # Build configuration

# Terminal 1├── initial_doc.txt     # Initial document template

./editor user_1├── README.md           # This file

# Edit user_1_doc.txt: Add "hello" to line 0└── DESIGNDOC          # Design documentation

```

# Terminal 2

./editor user_2## Technical Details

# Edit user_2_doc.txt: Add "world" to line 1

### Architecture Components

# Terminal 3

./editor user_31. **Main Thread**:

# Edit user_3_doc.txt: Add "!" to line 2   - Monitors local file for changes (using `stat()`)

   - Detects modifications by comparing content

# After 5 total changes, all files become identical   - Accumulates operations in buffer

```   - Broadcasts after 5 operations



### Example 2: Conflict Resolution2. **Listener Thread**:

   - Continuously monitors message queue

```bash   - Receives updates from other users

# user_1 and user_2 both edit position 11 of line 0   - Stores in received buffer for merging

# user_1: Inserts "A" at time 1000

# user_2: Inserts "B" at time 10023. **CRDT Merger**:

   - Detects conflicts (same line + overlapping columns)

# Result after merge: "B" wins (later timestamp)   - Resolves using Last-Writer-Wins

```   - Applies non-conflicting updates



### Example 3: Concurrent Deletes### Communication



```bash- **Shared Memory**: User registry (`/synctext_registry`)

# Initial: "int x = 10;abc"- **Message Queues**: One per user (`/queue_user_X`)

# user_1: Deletes "abc" - **Lock-Free**: Uses atomic operations, no mutexes

# user_2: Deletes "bc"

### Change Detection Algorithm

# Result after merge: "int x = 10;" (user_1's delete wins)

# user_2's delete is skipped (position no longer exists)1. Monitor file modification time using `stat()`

```2. When changed, read new content

3. Compare line-by-line with previous version

## Troubleshooting4. For each modified line:

   - Find first and last difference positions

### Issue: "Failed to register user"   - Extract old and new content

**Solution:** Run `./cleanup.sh` to clear stale registrations   - Create Update object with timestamp

5. Store updates for broadcasting

### Issue: "Failed to create message queue"

**Solution:** Socket files may exist from previous run### CRDT Properties

```bash

./cleanup.sh- **Commutativity**: Operations can be applied in any order

```- **Associativity**: Grouping doesn't matter

- **Idempotency**: Same operation applied multiple times = once

### Issue: Global counter not incrementing correctly- **Conflict Resolution**: LWW with timestamp, tiebreaker = smaller user_id

**Solution:** Ensure all previous instances are terminated

```bash## Troubleshooting

pkill editor

./cleanup.sh### "Failed to create message queue"

```- Clean up stale queues: `rm /dev/mqueue/queue_*`

- Or run: `make clean`

### Issue: Documents not merging

**Solution:** Check that all users are running and file permissions are correct### "Failed to initialize user registry"

- Remove shared memory: `rm /dev/shm/synctext_registry`

### Issue: Segmentation fault or crash- Or run: `make clean`

**Cause:** Usually bounds checking caught an error

**Solution:** Check terminal output for error messages, restart with cleanup### "Maximum users reached"

- Only 5 users can be active simultaneously

## Performance- Stop one user before starting a new one



- **File Monitoring**: 2 second polling interval### Changes not detected

- **Message Receive**: 100ms polling in listener thread- Ensure you're saving the file after editing

- **Merge Latency**: ~3 seconds (1s wait + merge + 2s sync)- Check file permissions (should be writable)

- **Memory**: O(n) where n ≤ 5 updates in buffer- Wait 2 seconds for the monitoring cycle

- **Supported Users**: 3-5 concurrent users

## Testing

## Implementation Details

### Test 1: Non-Conflicting Edits

### Threading Model1. Start 3 users

- **Main Thread**: File monitoring, change detection, broadcasting, merge coordination2. User 1 edits Line 0

- **Listener Thread**: Receives updates from other users via message queue3. User 2 edits Line 1

4. User 3 edits Line 2

### IPC Mechanisms5. Verify all users see all changes

- **Shared Memory**: `/synctext_registry` for user registry and global counter

- **UNIX Sockets**: `/tmp/queue_user_X.sock` for message passing### Test 2: Conflicting Edits

1. Start 2 users

### Synchronization2. Both edit Line 0 simultaneously

- **Atomic Operations**: Lock-free user registration and counter updates3. Verify the later timestamp wins

- **Merge Coordination**: Deliberate delays ensure all users merge simultaneously4. All users converge to same state



## Limitations### Test 3: Dynamic Users

1. Start User 1 and User 2

- Fixed 5-user maximum2. They make some edits

- 2-second polling granularity (not real-time)3. Start User 3 (joins mid-session)

- Simple LWW strategy (no operational transformation)4. Verify User 3 receives subsequent updates

- No persistent history or undo/redo

- Single document per session## Platform-Specific Notes



## Future Enhancements### macOS (M4 MacBook Air)

- Uses BSD-style system calls

- Real-time file monitoring (inotify/FSEvents)- Message queue location: `/private/var/folders/.../mqueue/`

- Operational Transformation for finer granularity- Shared memory: `/private/var/folders/.../shm/`

- Version history and collaborative undo

- Network support for distributed editing### Linux

- GUI interface (terminal UI or graphical)- Message queue location: `/dev/mqueue/`

- Multiple simultaneous documents- Shared memory: `/dev/shm/`

- User authentication and permissions

## Performance

## Technical Stack

- File monitoring: Every 2 seconds

- **Language**: C++17- Broadcast threshold: 5 operations

- **Threading**: std::thread, std::atomic- Message queue: Max 10 messages, 8KB each

- **IPC**: POSIX shared memory, UNIX domain sockets- Lock-free: No blocking operations

- **Serialization**: Custom string format for updates

- **Build**: Make with g++## Credits



## Project StructureProject developed for CS69201 - Computing Lab

Deadline: 10th November 2025

```
25CS60R72_Project-2/
├── Update.h              # Update data structure
├── UserRegistry.h        # Shared memory user registry
├── Document.h            # File I/O and change detection
├── CRDTMerger.h          # Conflict resolution
├── MessageQueue.h        # UNIX socket IPC
├── editor.cpp            # Main program
├── Makefile              # Build configuration
├── cleanup.sh            # Resource cleanup
├── initial_doc.txt       # Default document
├── DESIGNDOC             # Detailed design documentation
└── README.md             # This file
```

## Development

### Build from Source
```bash
git clone <repository>
cd 25CS60R72_Project-2
make
```

### Clean Build
```bash
make clean
make
```

### Debug Mode
Edit Makefile to add `-g -DDEBUG` flags for debugging symbols

## Testing

### Basic Test
1. Start 3 users
2. Each user makes 2 changes
3. Verify merge triggers after 5th change
4. Check all files are identical

### Conflict Test
1. Start 2 users
2. Both edit same position simultaneously
3. Verify CRDT resolves conflict
4. Check timestamp-based winner

### Stress Test
1. Start 3 users
2. Make rapid concurrent edits
3. Verify system remains stable
4. Check final consistency

## Contributing

This is an educational project for CS60R72. For questions or issues, please refer to the DESIGNDOC for architectural details.

## License

Educational project - CS60R72 course assignment
Date: November 2025

## Credits

Developed as part of the CS60R72 Computing Lab course demonstrating:
- Distributed systems concepts
- CRDT conflict resolution
- Lock-free programming
- Inter-process communication
- Collaborative algorithms

## References

- CRDT: Conflict-Free Replicated Data Types
- Operational Transformation vs CRDT comparison
- POSIX IPC mechanisms
- Atomic operations and memory models
