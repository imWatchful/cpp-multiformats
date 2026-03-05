#include <algorithm>
#include <array>
#include <gtest/gtest.h>
#include <varint.h>

struct EncodeCaseArgs {
    uint64_t value;
    std::vector<uint8_t> expected_bytes;
};

class EncodeCase : public testing::TestWithParam<EncodeCaseArgs> {};

TEST_P(EncodeCase, RoundTrip) {
    auto [value, expected] = GetParam();
    std::array<uint8_t, 9> buf{};
    std::size_t bytes_written;
    std::size_t bytes_read;
    uint64_t decoded_value = 0;
    ASSERT_EQ(varint::encode(value, buf, bytes_written), varint::encode_error::none);
    ASSERT_EQ(varint::decode(buf, decoded_value, bytes_read), varint::decode_error::none);
    EXPECT_EQ(value, decoded_value) << "Value mismatch after round trip";
    EXPECT_EQ(bytes_written, bytes_read) << "Byte mismatch during round trip";
}

TEST_P(EncodeCase, CorrectBytes) {
    auto [value, expected] = GetParam();
    std::array<uint8_t, 9> buf{};
    std::size_t out;
    ASSERT_EQ(varint::encode(value, buf, out), varint::encode_error::none);
    EXPECT_EQ(out, expected.size());
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), buf.begin()));
}

INSTANTIATE_TEST_SUITE_P(
    SpecExamples,
    EncodeCase,
    testing::Values(
        EncodeCaseArgs{0, {0x00}},
        EncodeCaseArgs{1, {0x01}},
        EncodeCaseArgs{127, {0x7F}},
        EncodeCaseArgs{128, {0x80, 0x01}},
        EncodeCaseArgs{255, {0xFF, 0x01}},
        EncodeCaseArgs{300, {0xAC, 0x02}},
        EncodeCaseArgs{16384, {0x80, 0x80, 0x01}},
        EncodeCaseArgs{0x7FFFFFFFFFFFFFFF, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}}
    )
);

TEST(TypeTest, Uint64OverflowCheck) {
    std::array<uint8_t, 9> buf{};
    std::size_t out;
    constexpr uint64_t overflow_value = 0x8000000000000000;
    EXPECT_EQ(varint::encode(overflow_value, buf, out), varint::encode_error::overflow);
}

TEST(TypeTest, Uint32OverflowCheck) {
    constexpr std::array<uint8_t, 6> buf{0x80, 0x80, 0x80, 0x80, 0x80, 0x01};
    uint32_t value = 0;
    std::size_t out;
    EXPECT_EQ(varint::decode<uint32_t>(buf, value, out), varint::decode_error::overflow);
}

TEST(TypeTest, Uint32MaxEncodesIn5Bytes) {
    std::array<uint8_t, 9> buf{};
    std::size_t out;
    ASSERT_EQ(varint::encode<uint32_t>(std::numeric_limits<uint32_t>::max(), buf, out), varint::encode_error::none);
    EXPECT_EQ(out, 5);
}

TEST(EncodeTest, BufTooSmallCheck) {
    std::array<uint8_t, 2> bad_buf{};
    std::size_t out;
    constexpr uint32_t value = 16384;
    EXPECT_EQ(varint::encode(value, bad_buf, out), varint::encode_error::buffer_too_small);
}

struct DecodeCaseArgs {
    std::vector<uint8_t> in_bytes;
    uint64_t expected_value;
};

class DecodeCase : public testing::TestWithParam<DecodeCaseArgs> {};

TEST_P(DecodeCase, CorrectValue) {
    auto [in, expected] = GetParam();
    uint64_t value = 0;
    std::size_t out;
    ASSERT_EQ(varint::decode(in, value, out), varint::decode_error::none);
    EXPECT_EQ(out, in.size());
    EXPECT_EQ(expected, value);
}

INSTANTIATE_TEST_SUITE_P(
    SpecExamples,
    DecodeCase,
    testing::Values(
        DecodeCaseArgs{{0x00}, 0},
        DecodeCaseArgs{{0x01}, 1},
        DecodeCaseArgs{{0x7F}, 127},
        DecodeCaseArgs{{0x80, 0x01}, 128},
        DecodeCaseArgs{{0xFF, 0x01}, 255},
        DecodeCaseArgs{{0xAC, 0x02}, 300},
        DecodeCaseArgs{{0x80, 0x80, 0x01}, 16384},
        DecodeCaseArgs{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}, 0x7FFFFFFFFFFFFFFF}
    )
);

TEST(DecodeTest, OverflowCheck) {
    constexpr std::array<uint8_t, 10> overflow_varint{0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01};
    uint64_t value = 0;
    std::size_t out;
    EXPECT_EQ(varint::decode(overflow_varint, value, out), varint::decode_error::overflow);
}

TEST(DecodeTest, FailsDecodeOnTruncatedVarint) {
    constexpr std::array<uint8_t, 2> truncated_varint{0x80, 0x80};
    uint64_t value = 0;
    std::size_t out;
    EXPECT_EQ(varint::decode(truncated_varint, value, out), varint::decode_error::buffer_too_small);
}

TEST(DecodeTest, NotMinimalCheck2Bytes) {
    constexpr std::array<uint8_t, 2> not_minimal_varint{0x81, 0x00};
    uint64_t value = 0;
    std::size_t out;
    EXPECT_EQ(varint::decode(not_minimal_varint, value, out), varint::decode_error::not_minimal);
}

TEST(DecodeTest, NotMinimalCheck3Bytes) {
    constexpr std::array<uint8_t, 3> not_minimal_varint{0x81, 0x81, 0x00};
    uint64_t value = 0;
    std::size_t out;
    EXPECT_EQ(varint::decode(not_minimal_varint, value, out), varint::decode_error::not_minimal);
}

TEST(DecodeTest, NotMinimalCheck4Bytes) {
    constexpr std::array<uint8_t, 4> not_minimal_varint{0x81, 0x81, 0x81, 0x00};
    uint64_t value = 0;
    std::size_t out;
    EXPECT_EQ(varint::decode(not_minimal_varint, value, out), varint::decode_error::not_minimal);
}

TEST(DecodeTest, StopsAtTerminalByteCheck) {
    constexpr std::array<uint8_t, 5> buf = {0x80, 0x01, 0xFF, 0xFF, 0xFF};
    uint64_t value = 0;
    size_t bytes_read = 0;
    EXPECT_EQ(varint::decode(buf, value, bytes_read), varint::decode_error::none);
    EXPECT_EQ(value, 128);
    EXPECT_EQ(bytes_read, 2);
}

TEST(DecodeTest, EmptyBufCheck) {
    constexpr std::array<uint8_t, 0> buf;
    uint64_t value = 0;
    size_t out;
    EXPECT_EQ(varint::decode(buf, value, out), varint::decode_error::buffer_too_small);
}

TEST(DecodeTest, DecodeMaxValueCheck) {
    constexpr std::array<uint8_t, 9> max_varint{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F};
    uint64_t value = 0;
    size_t out;
    EXPECT_EQ(varint::decode(max_varint, value, out), varint::decode_error::none);
    EXPECT_EQ(value, 0x7FFFFFFFFFFFFFFF);
    EXPECT_EQ(out, max_varint.size());
}