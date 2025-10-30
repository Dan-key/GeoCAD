#pragma once

#include "SpirvByteCode.h"
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Files {

class FileStream {
public:
    FileStream(const std::string& fileName);
    FileStream(std::string&& fileName);
    FileStream(const char* fileName);

    ~FileStream();

    size_t getSize() const;
    SpirvByteCode getSpirvByteCode() const;

protected:
    std::vector<uint32_t> readBinary() const;

    size_t _size;
    std::string _fileName;
    FILE* _fileStream;
};

}