#include "../TestFramework.h"
#include "../../system/FileUtils.cpp"
#include "../../memory/BufferMemory.cpp"

/**
 * File utility tests (@see system/FileUtils.cpp / platform/win32/FileUtils.cpp)
 *
 * All files are created in a temporary directory next to the executable and are
 * removed again, so the test doesn't leave any files behind.
 *
 * NOTE: file_read(path, file, offset) would select the FileHandle overload
 *       (a path pointer implicitly converts to a handle), so the path based
 *       overload is always called with an explicit memory buffer.
 */

struct TestFileBuffer {
    BufferMemory memory;
    byte* data;
};

static void file_test_buffer_alloc(TestFileBuffer* buf) {
    buffer_alloc(&buf->memory, 64 * KILOBYTE, 64 * KILOBYTE);
    buf->data = (byte *) memory_get(&buf->memory, 32 * KILOBYTE, sizeof(size_t));
}

static void file_test_buffer_free(TestFileBuffer* buf) {
    buffer_free(&buf->memory);
}

#define FILE_TEST_CONTENT "line one\nline two\nline three\n"
#define FILE_TEST_DIR_LITERAL "./oms_test_files"
#define FILE_TEST_PATH_LITERAL "./oms_test_files/test.txt"
#define FILE_TEST_COPY_LITERAL "./oms_test_files/test_copy.txt"
#define FILE_TEST_MOVE_LITERAL "./oms_test_files/test_moved.txt"

static void test_file_utils_path_parts() {
    char out[PATH_MAX_LENGTH];

    // Last path element (both separators)
    filename("C:\\path\\to\\file.txt", out);
    TEST_EQUALS(strcmp(out, "file.txt"), 0);

    filename("/some/path/file.txt", out);
    TEST_EQUALS(strcmp(out, "file.txt"), 0);

    filename("file.txt", out);
    TEST_EQUALS(strcmp(out, "file.txt"), 0);

    filename("C:\\path\\to\\.hidden", out);
    TEST_EQUALS(strcmp(out, ".hidden"), 0);

    // In place variant modifies the path buffer
    char path[] = "C:\\a\\b\\c.dat";
    TEST_EQUALS(strcmp(filename(path), "c.dat"), 0);

    // The base path is the directory of the given file path
    build_base_path("C:\\base\\app.exe", out);
    TEST_EQUALS(strcmp(out, "C:\\base"), 0);

    build_base_path("/base/sub/app", out);
    TEST_EQUALS(strcmp(out, "/base/sub"), 0);

    // A relative destination is appended to the base directory
    build_path("C:\\base\\app.exe", ".\\relative.txt", out);
    TEST_EQUALS(strcmp(out, "C:\\base\\relative.txt"), 0);

    // An absolute destination is used as is
    build_path("C:\\base\\app.exe", "D:\\other\\absolute.txt", out);
    TEST_EQUALS(strcmp(out, "D:\\other\\absolute.txt"), 0);
}

static void test_file_utils_self_path() {
    char directory[PATH_MAX_LENGTH];
    const int32 directory_length = self_directory_path(directory);
    TEST_TRUE(directory_length > 0);
    TEST_EQUALS(strlen(directory), directory_length);

    // The executable has to be inside of that directory
    char executable[PATH_MAX_LENGTH];
    TEST_TRUE(self_file_path(executable) > 0);
    TEST_TRUE(str_starts_with(executable, directory));

    // relative_to_absolute prefixes the executable directory
    char absolute[PATH_MAX_LENGTH];
    relative_to_absolute("./tests/somefile.txt", absolute);
    TEST_TRUE(str_starts_with(absolute, directory));
    TEST_TRUE(str_ends_with(absolute, "tests/somefile.txt"));

    // An absolute path is not modified
    relative_to_absolute("C:\\already\\absolute.txt", absolute);
    TEST_EQUALS(strcmp(absolute, "C:\\already\\absolute.txt"), 0);

    // In place version
    char in_place[PATH_MAX_LENGTH];
    str_copy(in_place, "./in_place.txt");
    relative_to_absolute(in_place);
    TEST_TRUE(str_starts_with(in_place, directory));
}

static void test_file_utils_exists() {
    char path[PATH_MAX_LENGTH];
    relative_to_absolute(FILE_TEST_PATH_LITERAL, path);

    // The file doesn't exist before the write (the directory may contain leftovers)
    file_delete(path);
    TEST_FALSE(file_exists(path));
    TEST_FALSE(file_exists("C:\\oms_test_file_that_does_not_exist.txt"));
}

static void test_file_utils_write_read() {
    char directory[PATH_MAX_LENGTH];
    relative_to_absolute(FILE_TEST_DIR_LITERAL, directory);
    TEST_TRUE(directory_tree_create(directory));
    // Creating an existing directory is not an error
    TEST_TRUE(directory_tree_create(directory));

    char path[PATH_MAX_LENGTH];
    relative_to_absolute(FILE_TEST_PATH_LITERAL, path);

    FileBody write_body = {
        sizeof(FILE_TEST_CONTENT) - 1,
        (byte *) FILE_TEST_CONTENT
    };
    TEST_TRUE(file_write(path, &write_body));
    TEST_TRUE(file_exists(path));

    TestFileBuffer mem;
    file_test_buffer_alloc(&mem);

    // Read it back
    FileBody read_body = { 0, NULL };
    file_read(path, &read_body, 0, &mem.memory);

    TEST_EQUALS(read_body.size, sizeof(FILE_TEST_CONTENT) - 1);
    TEST_MEMORY_EQUALS(read_body.content, FILE_TEST_CONTENT, read_body.size);

    // Appending text
    TEST_TRUE(file_append(path, "line four\n"));

    FileBody appended_body = { 0, NULL };
    file_read(path, &appended_body, 0, &mem.memory);
    TEST_EQUALS(appended_body.size, sizeof(FILE_TEST_CONTENT) - 1 + 10);
    TEST_MEMORY_EQUALS(appended_body.content, FILE_TEST_CONTENT, sizeof(FILE_TEST_CONTENT) - 1);
    TEST_MEMORY_EQUALS(
        appended_body.content + sizeof(FILE_TEST_CONTENT) - 1,
        "line four\n",
        10
    );

    // Line count (number of newlines)
    const FileHandle fp = file_read_handle(path);
    TEST_NOT_EQUALS(fp, INVALID_HANDLE_VALUE);
    TEST_EQUALS(file_count_lines(fp), 4);
    file_close_handle(fp);

    // Modification time is a sane unix timestamp (after 2020-01-01)
    TEST_GREATER_THAN(file_last_modified(path), 1577836800ULL);

    file_test_buffer_free(&mem);
}

static void test_file_utils_copy_move_delete() {
    char directory[PATH_MAX_LENGTH];
    relative_to_absolute(FILE_TEST_DIR_LITERAL, directory);
    directory_tree_create(directory);

    char source[PATH_MAX_LENGTH];
    char copy[PATH_MAX_LENGTH];
    char moved[PATH_MAX_LENGTH];
    relative_to_absolute(FILE_TEST_PATH_LITERAL, source);
    relative_to_absolute(FILE_TEST_COPY_LITERAL, copy);
    relative_to_absolute(FILE_TEST_MOVE_LITERAL, moved);

    file_delete(source);
    file_delete(copy);
    file_delete(moved);

    FileBody body = {
        sizeof(FILE_TEST_CONTENT) - 1,
        (byte *) FILE_TEST_CONTENT
    };
    TEST_TRUE(file_write(source, &body));

    // Copy
    TEST_TRUE(file_copy(source, copy));
    TEST_TRUE(file_exists(source));
    TEST_TRUE(file_exists(copy));

    TestFileBuffer mem;
    file_test_buffer_alloc(&mem);

    FileBody copy_body = { 0, NULL };
    file_read(copy, &copy_body, 0, &mem.memory);
    TEST_EQUALS(copy_body.size, body.size);
    TEST_MEMORY_EQUALS(copy_body.content, body.content, body.size);

    // Move
    TEST_TRUE(file_move(copy, moved));
    TEST_FALSE(file_exists(copy));
    TEST_TRUE(file_exists(moved));

    FileBody moved_body = { 0, NULL };
    file_read(moved, &moved_body, 0, &mem.memory);
    TEST_EQUALS(moved_body.size, body.size);
    TEST_MEMORY_EQUALS(moved_body.content, body.content, body.size);

    // Delete
    file_delete(source);
    file_delete(moved);
    TEST_FALSE(file_exists(source));
    TEST_FALSE(file_exists(moved));

    file_test_buffer_free(&mem);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main SystemFileUtilsTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_file_utils_path_parts);
    TEST_RUN(test_file_utils_self_path);
    TEST_RUN(test_file_utils_exists);
    TEST_RUN(test_file_utils_write_read);
    TEST_RUN(test_file_utils_copy_move_delete);

    TEST_FINALIZE();

    return 0;
}
