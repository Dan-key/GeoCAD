#include "SpirvByteCode.h"
#include <cstdint>
#include <vector>

namespace Files {

SpirvByteCode::SpirvByteCode(const std::vector<uint32_t>& byteCode) :
    _byteCode(byteCode)
{}

SpirvByteCode::SpirvByteCode(std::vector<uint32_t>&& byteCode) :
    _byteCode(byteCode)
{}

SpirvByteCode::SpirvByteCode(SpirvByteCode&& other)
{
    _byteCode = std::move(other._byteCode);
}

SpirvByteCode& SpirvByteCode::operator=(SpirvByteCode&& other)
{
    _byteCode = std::move(other._byteCode);
    return *this;
}

size_t SpirvByteCode::size() const noexcept
{
    return _byteCode.size();
}

uint32_t* SpirvByteCode::data() noexcept
{
    return _byteCode.data();
}

}