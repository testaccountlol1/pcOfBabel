#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <cerrno>

// Helper function to extract letters from the path and build the text
std::string get_file_content_from_path(const std::string& path) {
    if (path == "/" || path.empty()) return "";

    std::string content = "";
    size_t start = 1; // Skip initial '/'
    size_t end = path.find('/', start);

    while (start < path.size()) {
        std::string component = path.substr(start, end - start);
        
        // If the component is our terminal file name, stop collecting letters
        if (component == "data.txt") {
            break;
        }

        // Directly append the folder name as long as it's a single character
        if (component.size() == 1) { 
            content += component;
        }

        if (end == std::string::npos) break;
        start = end + 1;
        end = path.find('/', start);
    }
    return content;
}

// 1. GETATTR: Assumes all subpaths exist either as a single-character folder or data.txt
static int fs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    std::memset(stbuf, 0, sizeof(struct stat));
    std::string p(path);

    stbuf->st_uid = fuse_get_context()->uid;
    stbuf->st_gid = fuse_get_context()->gid;

    if (p == "/") {
        stbuf->st_mode = S_IFDIR | 0777;
        stbuf->st_nlink = 2;
        return 0;
    }

    // Check if the user is requesting the terminal data file
    if (p.size() >= 8 && p.substr(p.size() - 8) == "data.txt") {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = get_file_content_from_path(p).size();
        return 0;
    }

    // Treat everything else as an infinite single-character folder container
    stbuf->st_mode = S_IFDIR | 0777;
    stbuf->st_nlink = 2;
    return 0;
}

// 2. READDIR: Generates lowercase, uppercase, numbers, spaces, and ALL special symbols
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    
    filler(buf, ".", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
    filler(buf, "..", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
    filler(buf, "data.txt", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));

    // A complete string of every single character option available to use
    std::string symbols = "abcdefghijklmnopqrstuvwxyz"
                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                          "0123456789"
                          " _*{}[]()<>+-=/\\!@#$%^&|;:.,?`~'\"";

    char name_buf[2] = {0, 0};
    for (char c : symbols) {
        name_buf[0] = c;
        filler(buf, name_buf, nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
    }

    return 0;
}

// 3. READ: Translates the folder paths back into raw bytes on the fly
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    std::string p(path);
    std::string file_content = get_file_content_from_path(p);

    size_t len = file_content.size();
    if (offset >= static_cast<off_t>(len)) return 0;
    if (offset + size > len) size = len - offset;

    std::memcpy(buf, file_content.c_str() + offset, size);
    return size;
}

static int fs_open(const char *path, struct fuse_file_info *fi) { return 0; }

int main(int argc, char *argv[]) {
    struct fuse_operations ops;
    std::memset(&ops, 0, sizeof(ops));
    
    ops.getattr = fs_getattr;
    ops.readdir = fs_readdir;
    ops.open    = fs_open;
    ops.read    = fs_read;

    return fuse_main(argc, argv, &ops, nullptr);
}
