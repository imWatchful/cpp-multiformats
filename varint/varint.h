#include <concepts>
#include <cstdint>
#include <limits>
#include <span>

namespace varint
{
enum class encode_error
{
    none,
    buffer_too_small,
    overflow
};

enum class decode_error
{
    none,
    buffer_too_small,
    overflow,
    not_minimal
};

template <std::unsigned_integral T> constexpr std::size_t max_encoded_size() noexcept
{
    return (std::numeric_limits<T>::digits + 6) / 7;
}

template <std::unsigned_integral T>
constexpr encode_error encode(T value, std::span<std::uint8_t> out, std::size_t &bytes_out) noexcept
{
    constexpr std::size_t spec_max_bytes = 9;
    constexpr std::size_t max_bytes = (max_encoded_size<T>() < spec_max_bytes) ? max_encoded_size<T>() : spec_max_bytes;

    if constexpr (std::numeric_limits<T>::digits > 63)
    {
        if (value >= (T(1) << 63))
            return encode_error::overflow;
    }

    std::size_t needed = 1;
    T tmp = value;
    while (tmp >= T(0x80))
    {
        tmp >>= 7;
        ++needed;
    }

    if (needed > max_bytes)
        return encode_error::overflow;

    if (out.size() < needed)
        return encode_error::buffer_too_small;

    std::size_t i = 0;
    do
    {
        std::uint8_t byte = static_cast<std::uint8_t>(value & 0x7F);
        value >>= 7;

        if (value != 0)
            byte |= 0x80;

        out[i++] = byte;
    } while (value != 0);

    bytes_out = i;
    return encode_error::none;
}

template <std::unsigned_integral T>
constexpr decode_error decode(std::span<const std::uint8_t> in, T &value_out, std::size_t &bytes_out) noexcept
{
    constexpr std::size_t spec_max_bytes = 9;
    constexpr std::size_t max_bytes = (max_encoded_size<T>() < spec_max_bytes) ? max_encoded_size<T>() : spec_max_bytes;
    constexpr std::size_t limit_bits = (std::numeric_limits<T>::digits < 63) ? std::numeric_limits<T>::digits : 63;

    T value = 0;
    std::size_t shift = 0;

    for (std::size_t i = 0; i < in.size() && i < max_bytes; ++i)
    {
        std::uint8_t byte = in[i];
        T chunk = static_cast<T>(byte & 0x7F);

        if (shift >= limit_bits)
            return decode_error::overflow;

        const std::size_t remaining = limit_bits - shift;
        if (remaining < 7 && chunk >= (T(1) << remaining))
            return decode_error::overflow;

        value |= (chunk << shift);
        shift += 7;

        if ((byte & 0x80) == 0)
        {
            if (i > 0 && byte == 0x00)
                return decode_error::not_minimal;

            value_out = value;
            bytes_out = i + 1;
            return decode_error::none;
        }
    }

    if (in.size() < max_bytes)
        return decode_error::buffer_too_small;

    return decode_error::overflow;
}
} // namespace varint
