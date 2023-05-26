#include <cthash/cthash.hpp>
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 2) {
        return 1;
    }

    const auto in = std::string_view(argv[1]);

    std::cout << "        sha224 = " << cthash::sha224 {}.update(in).final() << "\n";
    std::cout << "        sha256 = " << cthash::sha256 {}.update(in).final() << "\n";
    std::cout << "        sha384 = " << cthash::sha384 {}.update(in).final() << "\n";
    std::cout << "        sha512 = " << cthash::sha512 {}.update(in).final() << "\n";
    std::cout << "    sha512/224 = " << cthash::sha512t<224> {}.update(in).final() << "\n";
    std::cout << "    sha512/256 = " << cthash::sha512t<256> {}.update(in).final() << "\n";

    std::cout << "      sha3-224 = " << cthash::sha3_224 {}.update(in).final() << "\n";
    std::cout << "      sha3-256 = " << cthash::sha3_256 {}.update(in).final() << "\n";
    std::cout << "      sha3-384 = " << cthash::sha3_384 {}.update(in).final() << "\n";
    std::cout << "      sha3-512 = " << cthash::sha3_512 {}.update(in).final() << "\n";

    std::cout << "shake-128/64   = " << cthash::shake128 {}.update(in).final<64>() << "\n";
    std::cout << "shake-128/128  = " << cthash::shake128 {}.update(in).final<128>() << "\n";
    std::cout << "shake-128/1024 = " << cthash::shake128 {}.update(in).final<1024>() << "\n";

    std::cout << "shake-256/64   = " << cthash::shake256 {}.update(in).final<64>() << "\n";
    std::cout << "shake-256/128  = " << cthash::shake256 {}.update(in).final<128>() << "\n";
    std::cout << "shake-256/1024 = " << cthash::shake256 {}.update(in).final<1024>() << "\n";

    std::cout << "      xxhash32 = " << cthash::xxhash32 {}.update(in).final() << "\n";
    std::cout << "      xxhash64 = " << cthash::xxhash64 {}.update(in).final() << "\n";
}
