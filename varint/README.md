# cpp-varint

Header-only C++20 implementation of the multiformats unsigned-varint spec.

Spec: https://github.com/multiformats/unsigned-varint

## Usage

Only the header file is needed.

How to use:

- `varint::encode<T>(value, out, bytes_out) -> encode_error`
- `varint::decode<T>(in, value_out, bytes_out) -> decode_error`

## Tests

TODO

## License

[MIT](LICENSE)
