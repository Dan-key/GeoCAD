#include "FileStream.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <errno.h>
#include <string.h>
#include <vector>

namespace Files {

FileStream::FileStream(const std::string& fileName) :
    _fileName(fileName)
{
    _fileStream = fopen(_fileName.c_str(), "rb");
    if (!_fileStream) {
        throw std::runtime_error("can't open file " + fileName + strerror(errno));
    }

    fseek(_fileStream, 0, SEEK_END);
    _size = ftell(_fileStream);
    fseek(_fileStream, 0, SEEK_SET);
}

FileStream::FileStream(std::string&& fileName) :
    _fileName(fileName)
{
    _fileStream = fopen(_fileName.c_str(), "rb");
    if (!_fileStream) {
        throw std::runtime_error("can't open file " + fileName + strerror(errno));
    }

    fseek(_fileStream, 0, SEEK_END);
    _size = ftell(_fileStream);
    fseek(_fileStream, 0, SEEK_SET);
}

FileStream::FileStream(const char* fileName) :
    _fileName(fileName)
{
    _fileStream = fopen(_fileName.c_str(), "rb");
    if (!_fileStream) {
        throw std::runtime_error("can't open file " + _fileName + strerror(errno));
    }

    fseek(_fileStream, 0, SEEK_END);
    _size = ftell(_fileStream);
    fseek(_fileStream, 0, SEEK_SET);
}

FileStream::~FileStream()
{
    if (_fileStream) {
        fclose(_fileStream);
    }
}

std::vector<uint32_t> FileStream::readBinary() const
{
    if (_size == 0) {
        return {};
    }

    std::vector<uint32_t> v(_size);

    fread(v.data(), 1, _size, _fileStream);
    return v;
}

Vulkan::SpirvByteCode FileStream::getSpirvByteCode() const
{
    return Vulkan::SpirvByteCode(readBinary());
}

size_t FileStream::getSize() const
{
    return _size;
}

}