//
// Created by ps on 2021/11/11.
//

#ifndef TESTUNIT_FILE_HANDLER_H
#define TESTUNIT_FILE_HANDLER_H

#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <string>
#include <cstdarg>

#define FORMAT_PERROR(...) {                                                                 \
    char buf[200];                                                                           \
    sprintf(buf, __VA_ARGS__);                                                               \
    perror(buf);                                                                             \
}

inline std::string get_file_path(int fd){
    char file_path[32];
    sprintf(file_path, "/proc/self/fd/%d", fd);
    char buf[PATH_MAX];
    ssize_t num = readlink(file_path, buf, PATH_MAX);
    if (num == -1) {
        FORMAT_PERROR("Cannot call readlink on file %s", file_path);
        throw "readlink() error";
    } else {
        buf[num] = '\0';
    }
    return buf;
}

inline int open_file(const char *path, int flag, ...) {
    va_list args;
    va_start(args, flag);
    if (flag & O_CREAT) {
        int access_flag = 0;
        access_flag = va_arg(args, int);
        int fd = open(path, flag, access_flag);
        if (fd == -1) {
            FORMAT_PERROR("Can not open file %s with flag %d and access flag %d", path, flag, access_flag);
            throw "Failed to open file";
        }
        return fd;
    } else {
        int fd = open(path, flag);
        if (fd == -1) {
            FORMAT_PERROR("Can not open file %s with flag %d", path, flag);
            throw "Failed to open file";
        }
        return fd;
    }

}

inline int create_file(const char *path, int access_flag) {
    int fd = open(path, O_CREAT, access_flag);
    if (fd == -1) {
        FORMAT_PERROR("Can not create file %s with access flag %d", path, access_flag);
        throw "Failed to create file";
    }
    return fd;
}

inline off_t seek_file(int fd, __off_t offset, int whence) {
    off_t result;
    if ((result = lseek(fd, offset, whence)) == -1){
        FORMAT_PERROR("Can not seek file %s to offset %ld(whence = %d)", get_file_path(fd).c_str(), offset, whence);
        throw "Failed to lseek file";
    }
    return result;
}

inline off_t seek_file(int fd, __off_t offset) {
    return seek_file(fd, offset, SEEK_SET);
}

inline void read_file(int fd, void *buf, size_t bytes) {
    int bytes_read;
    if ((bytes_read = read(fd, buf, bytes)) != bytes){
        FORMAT_PERROR("Can not read %lu bytes from file %s, only %d bytes read", bytes, get_file_path(fd).c_str(), bytes_read);
        throw "Failed to read file";
    }
}

inline void write_file(int fd, const void *buf, size_t bytes) {
    int bytes_written;
    if ((bytes_written = write(fd, buf, bytes)) != bytes){
        FORMAT_PERROR("Can not write %lu bytes to file %s, only %d bytes is written", bytes, get_file_path(fd).c_str(), bytes_written);
        throw "Failed to lseek file";
    }
}

inline off_t get_file_size(int fd) {
    struct stat file_status;
    fstat(fd, &file_status);
    return file_status.st_size;
}

inline void truncate_file(int fd, off_t length) {
    if (-1 == ftruncate(fd, length)) {
        FORMAT_PERROR("Can not truncate file %s to size %lu bytes", get_file_path(fd).c_str(), length);
        throw "Failed to truncate file";
    }
}

inline void close_file(int fd) {
    if (-1 == close(fd)) {
        FORMAT_PERROR("Can not close file %s", get_file_path(fd).c_str());
        throw "Failed to close file";
    }
}

inline void remove_file(const char *file_path) {
    if (-1 == remove(file_path)) {
        FORMAT_PERROR("Can not remove file %s", file_path);
        throw "Failed to remove file";
    }
}

inline void clear_sys_cache(){
    int fd = open_file("/proc/sys/vm/drop_caches", O_WRONLY);
    const char *data = "3";
    write_file(fd, data, sizeof(char));
}

inline void copy_file(int ofd, int ifd, size_t count) {
    ssize_t copied_bytes;
    while((copied_bytes = sendfile(ofd, ifd, NULL, count)) != count) {
        if (copied_bytes == -1) {
            FORMAT_PERROR("Failed to copy from file %s to file %s\n", get_file_path(ifd).c_str(), get_file_path(ofd).c_str());
            exit(-1);
        }
        count -= copied_bytes;
    }
}

inline void copy_file(const char *ofile, const char *ifile, size_t count) {
    int ofd = open_file(ofile, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR);
    int ifd = open_file(ifile, O_RDONLY);
    ssize_t copied_bytes;
    while((copied_bytes = sendfile(ofd, ifd, NULL, count)) != count) {
        if (copied_bytes == -1) {
            FORMAT_PERROR("Failed to copy from file %s to file %s\n", get_file_path(ifd).c_str(), get_file_path(ofd).c_str());
            exit(-1);
        }
        count -= copied_bytes;
    }
    close_file(ofd);
    close(ifd);
}

#endif //TESTUNIT_FILE_HANDLER_H
