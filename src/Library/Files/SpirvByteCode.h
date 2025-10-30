#pragma once

#include <vector>
#include <cstdint>

namespace Files {

class SpirvByteCode {
public:
    SpirvByteCode(const std::vector<uint32_t>& byteCode);
    SpirvByteCode(std::vector<uint32_t>&& byteCode);
    SpirvByteCode() = default;

    SpirvByteCode(const SpirvByteCode&) = default;
    SpirvByteCode(SpirvByteCode&&);

    SpirvByteCode& operator=(const SpirvByteCode&) = default;
    SpirvByteCode& operator=(SpirvByteCode&&);

    size_t size() const noexcept;
    uint32_t* data() noexcept;

private:
    std::vector<uint32_t> _byteCode;
};

}