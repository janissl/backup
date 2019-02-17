#include <iostream>
#include <cerrno>
#include <memory>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <utime.h>

using std::cerr;
using std::endl;
using std::unique_ptr;
using std::make_unique;


FILE *log_stream;


void PrintUsage(const char *app_name) {
    cerr << endl;
    cerr << "USAGE: " << app_name << " SOURCE_DIRECTORY DESTINATION_DIRECTORY" << endl;
    cerr << endl;
    cerr << "\tSOURCE DIRECTORY:         The path of the directory to copy from" << endl;
    cerr << "\tDESTINATION DIRECTORY:    The path of the directory to copy to" << endl;
    cerr << endl;
}


char GetFileSeparator() {
    char file_sep;

#ifdef _WIN32_WINNT
    file_sep = '\\';
#else
    file_sep  = '/';
#endif

    return file_sep;
}


const char *GetFileNameFromPath(const char *path) {
    char file_sep = GetFileSeparator();
    size_t last_sep_pos;

    for (last_sep_pos = std::strlen(path) - 1; last_sep_pos >= 0; --last_sep_pos) {
        if (path[last_sep_pos] == file_sep) {
            break;
        }
    }

    return &path[last_sep_pos + 1];
}


time_t GetLastModificationTime(const char *path) {
    time_t mod_time = 0;

    try {
        struct stat buf{};

        if ((stat(path, &buf)) != 0) {
            throw;
        }

        mod_time = buf.st_mtime;
    } catch (...) {
        fprintf(log_stream, "Could not read the metadata of '%s' - %s\n", path, strerror(errno));
    }

    return mod_time;
}


mode_t GetFileMode(const char *path) {
    mode_t file_mode = 0;

    try {
        struct stat buf{};

        if ((stat(path, &buf)) != 0) {
            throw;
        }

        file_mode = buf.st_mode;
    } catch (...) {
        fprintf(log_stream, "Could not read the metadata of '%s' - %s\n", path, strerror(errno));
    }

    return file_mode;
}


void SetOriginalTimes(const char *source_path, const char *destination_path) {
    struct stat src_times{};
    struct utimbuf dst_times{};

    stat(source_path, &src_times);
    dst_times.actime = src_times.st_atime;
    dst_times.modtime = src_times.st_mtime;
    utime(destination_path, &dst_times);
}



bool FileExists(const char *path) {
    struct stat buf{};
    int res = stat(path, &buf);
    return (res == 0);
}


bool IsBackupRequired(const char *source_path, const char *dest_path) {
    return (!FileExists(dest_path) || GetLastModificationTime(dest_path) < GetLastModificationTime(source_path));
}


bool CopyNewFile(const char *source_path, const char *destination_path) {
    bool success = true;

    try {
        FILE *in = fopen(source_path, "rb");

        if (in == nullptr) {
            fprintf(log_stream, "FAILED to open the file '%s' for reading - %s\n", source_path, strerror(errno));
            throw;
        }

        FILE *out = fopen(destination_path, "wb");

        if (out == nullptr) {
            fprintf(log_stream, "FAILED to open the file '%s' for writing - %s\n", destination_path, strerror(errno));
            fclose(in);
            throw;
        }

        char buf[BUFSIZ];
        size_t read = 0;

        while ((read = fread(buf, sizeof(char), BUFSIZ, in)) == BUFSIZ)
            fwrite(buf, sizeof(char), BUFSIZ, out);

        fwrite(buf, sizeof(char), read, out);

        fclose(out);
        fclose(in);

        if (!FileExists(destination_path)) {
            fprintf(log_stream, "FAILED to create the file '%s'\n", destination_path);
            throw;
        }

        SetOriginalTimes(source_path, destination_path);
    } catch (...) {
        success = false;
    }

    return success;
}


bool CreateNewDirectory(const char *destination_directory) {
    bool success = true;

    try {
        if (!FileExists(destination_directory)) {
#ifdef _WIN32_WINNT
            auto err = mkdir(destination_directory);
#else
            auto err = mkdir(destination_directory, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
            if (err != 0) {
                fprintf(log_stream, "FAILED to create '%s' - %s\n", destination_directory, strerror(err));
                throw;
            }
        }
    } catch (...) {
        success = false;
    }

    return success;
}


unique_ptr<char[]> JoinPath(const char *parent, const char *child) {
    char file_sep[2] = {0};
    file_sep[0] = GetFileSeparator();

    const size_t full_path_len = std::strlen(parent) + std::strlen(file_sep) + std::strlen(child) + 1;
    auto new_path = make_unique<char[]>(full_path_len);

    std::strncpy(new_path.get(), parent, full_path_len);
    std::strncat(new_path.get(), file_sep, full_path_len);
    std::strncat(new_path.get(), child, full_path_len);

    return new_path;
}


void BackupDirectoryTree(const char *source_directory, const char *destination_directory) {
    struct dirent *dp;
    DIR *dir = opendir(source_directory);

    if (!dir) {
        fprintf(log_stream, "The source directory '%s' NOT found\n", source_directory);
        return;
    }

    while ((dp = readdir(dir)) != nullptr) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {

            auto source_path = JoinPath(source_directory, dp->d_name);
            auto dest_path = JoinPath(destination_directory, dp->d_name);

            if (!CreateNewDirectory(destination_directory))
                break;

            if (FileExists(source_path.get())) {
                if (S_ISDIR(GetFileMode(source_path.get()))) {
                    BackupDirectoryTree(source_path.get(), dest_path.get());
                } else if (S_ISREG(GetFileMode(source_path.get()))) {
                    if (IsBackupRequired(source_path.get(), dest_path.get())) {
                        if (!CopyNewFile(source_path.get(), dest_path.get())) {
                            fprintf(log_stream, "FAILED to copy '%s' to '%s'\n", source_path.get(), dest_path.get());
                        } else {
                            fprintf(log_stream, "'%s' -> '%s'\n", source_path.get(), dest_path.get());
                        }
                    }
                }
            }
        }
    }

    closedir(dir);
}


int RunBackup(const char *source_root, const char *dest_root) {
    int err = 0;
    log_stream = fopen("last.log", "w");

    if (!log_stream) {
        err = errno;
        cerr << "Could not create the last.log file - " << strerror(err) << endl;
    } else {
        BackupDirectoryTree(source_root, dest_root);
        fclose(log_stream);
    }

    return err;
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        const char *bin_name = GetFileNameFromPath(argv[0]);
        PrintUsage(bin_name);
        return 1;
    }

    auto err = RunBackup(argv[1], argv[2]);

    return err;
}
