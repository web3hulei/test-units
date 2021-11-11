//
// Created by ps on 2021/11/11.
//

#include <gtest/gtest.h>
#include <cstdint>
#include "../utils/file_handler.h"

static constexpr uint32_t ALIGNMENT = 4 * 1024;

std::string bytes_to_string(uint64_t bytes){
    const uint64_t KB = 1024;
    const uint64_t MB = 1024 * KB;
    const uint64_t GB = 1024 * MB;
    if (bytes % GB == 0) {
        return std::to_string(bytes / GB) + "GB";
    } else if (bytes % MB == 0) {
        return std::to_string(bytes / MB) + "MB";
    } else if (bytes % KB == 0) {
        return std::to_string(bytes / KB) + "KB";
    }
    return std::to_string(bytes) + "B";
}

void create_file(const char *file_name, uint64_t size) {
    constexpr uint64_t UNIT_SIZE = 32UL * 1024 * 1024;
    int fd = open_file(file_name, O_DIRECT | O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    uint64_t written = 0;
    std::unique_ptr<uint8_t[]> buf(new uint8_t[UNIT_SIZE + ALIGNMENT - 1]);
    uint8_t *aligned_ptr = (uint8_t*)(((uint64_t)buf.get() + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT);
    while(written < size) {
        uint64_t to_write = std::min(UNIT_SIZE, size - written);
        write_file(fd, aligned_ptr, to_write);
        written += to_write;
    }
    close_file(fd);
}

TEST(File, AccessPattern){
    constexpr uint64_t SIZE = 32UL * 1024 * 1024 * 1024;
    //Prepare file
    const char *file_name = "$FILE$ACCESSPATTERNTEST$";
    create_file(file_name, SIZE);
    int fd = open_file(file_name, O_RDONLY | O_DIRECT);
    uint64_t unit_sizes[] = {4 * 1024, 4 * 1024 * 1024, 32 * 1024 * 1024};
    for (uint64_t unit_size : unit_sizes) {
        //Test sequential read
        assert(SIZE % unit_size == 0);
        std::unique_ptr<uint8_t[]> buf(new uint8_t[unit_size + ALIGNMENT - 1]);
        uint8_t *aligned_ptr = (uint8_t*)(((uint64_t)buf.get() + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT);
        seek_file(fd, 0);
        auto start = std::chrono::steady_clock::now();
        uint64_t read = 0;
        while(read < SIZE) {
            uint64_t to_read = std::min(unit_size, SIZE - read);
            read_file(fd, aligned_ptr, to_read);
            read += to_read;
        }
        auto end = std::chrono::steady_clock::now();
        uint64_t time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        printf("Use direct IO to read %s bytes from file in sequence. Block size: %s, Bandwidth: %.2f GB/s\n",
               bytes_to_string(SIZE).c_str(), bytes_to_string(unit_size).c_str(), SIZE / 1024.0 / 1024.0 / 1024.0 * 1000.0 / time_elapsed);
        //Test Random access
        srand(0);
        uint64_t block_num = SIZE / unit_size;
        std::unique_ptr<uint64_t[]> indexes(new uint64_t[block_num]);
        for (uint64_t j = 0; j < block_num; j++) {
            indexes[j] = j;
        }
        for (uint64_t j = 0; j < block_num; j++) {
            uint64_t pos = rand() % (block_num - j);
            std::swap(indexes[j], indexes[j + pos]);
        }
        start = std::chrono::steady_clock::now();
        for (uint64_t j = 0; j < block_num; j++) {
            seek_file(fd, indexes[j] * unit_size);
            read_file(fd, aligned_ptr, unit_size);
        }
        end = std::chrono::steady_clock::now();
        time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        printf("Use direct IO to random read %s bytes from file. Block size: %s, Bandwidth: %.2f GB/s\n", bytes_to_string(SIZE).c_str(), bytes_to_string(unit_size).c_str(), SIZE / 1024.0 / 1024.0 / 1024.0 * 1000.0 / time_elapsed);
    }
    remove_file(file_name);
}