#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Library/Vulkan/SpirvByteCode.h"

namespace Files {

class FileStream {
public:
    FileStream(const std::string& fileName);
    FileStream(std::string&& fileName);
    FileStream(const char* fileName);

    ~FileStream();

    size_t getSize() const;
    Vulkan::SpirvByteCode getSpirvByteCode() const;

protected:
    std::vector<uint32_t> readBinary() const;

    size_t _size;
    std::string _fileName;
    FILE* _fileStream;
};

}