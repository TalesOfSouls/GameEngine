#include "../stdlib/Types.h"
#include "../memory/BufferMemory.h"
#include "../system/Allocator.h"
#include "../stdlib/HashMap.h"
#include "../utils/StringUtils.h"
#include "../utils/RandomUtils.h"

#define MAX_SESSION_ID_LENGTH 32

struct Session {
    uint64 last_used;

    // Hash map that contains the offsets into the data memory
    // The hash map starts at the
    HashMap hm;

    // offset into the data memory
    uint32 offset;
    uint32 data_size;
};

struct SessionManager {
    // Hash map used to find sessions by ID
    // The hash map contains the offsets into the sessions array
    // @todo make random_string() for the session_id
    HashMap hm;

    Session *sessions;

    // Data shared accross sessions
    byte* session_data;

    size_t count;
    size_t capacity;
    const char *storage_path;
    uint64 last_cleanup;
};

SessionManager* session_manager_alloc(
    SessionManager* manager,
    const char* storage_path,
    size_t initial_capacity,
    int32 alignment = 64
) {
    size_t internal_buffer_size = ROUND_TO_NEAREST(sizeof(Session) * initial_capacity, 4096);
    byte* internal_buffer = (byte *) platform_alloc_aligned(internal_buffer_size, alignment);

    // distribute internal_buffer to:
    //      session_key_data
    //      session_data
    //      hm per session

    manager->sessions = (Session *) internal_buffer;

    manager->count = 0;
    manager->capacity = initial_capacity;
    manager->storage_path = strdup(storage_path);
    manager->last_cleanup = time(NULL);

    ensure_storage_directory_exists(storage_path);

    return manager;
}

Session* session_manager_create(SessionManager *manager) {
    if (manager->count >= manager->capacity) {
        // Try to cleanup first
        session_manager_cleanup(manager);

        // If still full, move oldest session to disk
        if (manager->count >= manager->capacity) {
            // Find oldest session
            time_t oldest_time = time(NULL);
            size_t oldest_index = 0;

            for (size_t i = 0; i < manager->count; i++) {
                if (manager->sessions[i].last_used < oldest_time) {
                    oldest_time = manager->sessions[i].last_used;
                    oldest_index = i;
                }
            }

            // Save to disk
            save_session_to_disk(manager, &manager->sessions[oldest_index]);

            // Remove from memory
            if (oldest_index != manager->count - 1) {
                memmove(&manager->sessions[oldest_index],
                        &manager->sessions[oldest_index + 1],
                        (manager->count - oldest_index - 1) * sizeof(Session));
            }
            manager->count--;
        }
    }

    // Create new session
    Session *session = &manager->sessions[manager->count++];
    session_id_generate(session->id);
    session->last_used = time(NULL);
    session->data_size = 0;
    session->data[0] = '\0';

    return session;
}

Session* session_manager_get(SessionManager *manager, const char *session_id) {
    // First check memory
    for (size_t i = 0; i < manager->count; i++) {
        if (strcmp(manager->sessions[i].id, session_id) == 0) {
            manager->sessions[i].last_used = time(NULL);
            return &manager->sessions[i];
        }
    }

    // Not in memory, try disk
    Session temp_session;
    if (load_session_from_disk(manager, &temp_session, session_id)) {
        // Make space if needed
        if (manager->count >= manager->capacity) {
            session_manager_cleanup(manager);

            if (manager->count >= manager->capacity) {
                // Still full, need to move one to disk
                time_t oldest_time = time(NULL);
                size_t oldest_index = 0;

                for (size_t i = 0; i < manager->count; i++) {
                    if (manager->sessions[i].last_used < oldest_time) {
                        oldest_time = manager->sessions[i].last_used;
                        oldest_index = i;
                    }
                }

                save_session_to_disk(manager, &manager->sessions[oldest_index]);

                if (oldest_index != manager->count - 1) {
                    memmove(&manager->sessions[oldest_index],
                            &manager->sessions[oldest_index + 1],
                            (manager->count - oldest_index - 1) * sizeof(Session));
                }
                manager->count--;
            }
        }

        // Add to memory
        Session *session = &manager->sessions[manager->count++];
        memcpy(session, &temp_session, sizeof(Session));
        session->last_used = time(NULL); // Update last used time

        // Remove from disk (it's now in memory)
        delete_session_from_disk(manager, session_id);

        return session;
    }

    return NULL; // Not found
}

void session_manager_delete(SessionManager *manager, const char *session_id) {
    // Delete from memory
    for (size_t i = 0; i < manager->count; i++) {
        if (strcmp(manager->sessions[i].id, session_id) == 0) {
            if (i != manager->count - 1) {
                memmove(&manager->sessions[i],
                        &manager->sessions[i + 1],
                        (manager->count - i - 1) * sizeof(Session));
            }
            manager->count--;
            break;
        }
    }

    // Delete from disk
    delete_session_from_disk(manager, session_id);
}

void session_manager_cleanup(SessionManager *manager) {
    time_t now = time(NULL);

    // Clean memory
    size_t i = 0;
    while (i < manager->count) {
        if (now - manager->sessions[i].last_used > SESSION_EXPIRY_SECONDS) {
            // Move to disk before deleting (if we want to keep expired sessions on disk)
            // Or just delete completely:

            if (i != manager->count - 1) {
                memmove(&manager->sessions[i],
                        &manager->sessions[i + 1],
                        (manager->count - i - 1) * sizeof(Session));
            }
            manager->count--;
        } else {
            i++;
        }
    }

    // Clean disk storage (do this less frequently)
    if (now - manager->last_cleanup > (SESSION_EXPIRY_SECONDS / 2)) {
        DIR *dir = opendir(manager->storage_path);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strlen(entry->d_name) == SESSION_ID_LENGTH) {
                    char path[PATH_MAX];
                    snprintf(path, sizeof(path), "%s/%s", manager->storage_path, entry->d_name);

                    struct stat st;
                    if (stat(path, &st) == 0) {
                        if (now - st.st_mtime > SESSION_EXPIRY_SECONDS) {
                            unlink(path);
                        }
                    }
                }
            }
            closedir(dir);
        }
        manager->last_cleanup = now;
    }
}

void session_id_generate(char* id) {
    uint64 x = time_index();
    int32 id_length = (rand_fast(&x) % 6) + HASH_MAP_MAX_KEY_LENGTH - 5;

    random_string(
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz@!",
        sizeof("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz@!") - 1,
        id,
        id_length - 1
    );
}

static void save_session_to_disk(SessionManager *manager, const Session *session) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", manager->storage_path, session->id);

    FILE *file = fopen(path, "wb");
    if (!file) return;

    fwrite(session, sizeof(Session), 1, file);
    fclose(file);

    // Update file modification time to match last_used
    struct utimbuf times;
    times.actime = session->last_used;
    times.modtime = session->last_used;
    utime(path, &times);
}

static bool load_session_from_disk(SessionManager *manager, Session *session, const char *session_id) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", manager->storage_path, session_id);

    FILE *file = fopen(path, "rb");
    if (!file) return false;

    bool success = fread(session, sizeof(Session), 1, file) == 1;
    fclose(file);

    return success;
}

static void delete_session_from_disk(SessionManager *manager, const char *session_id) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", manager->storage_path, session_id);
    unlink(path);
}

static void ensure_storage_directory_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0700);
    }
}