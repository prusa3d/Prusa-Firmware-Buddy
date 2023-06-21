#include <cthash/cthash.hpp>
#include <chrono>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>

struct mapped_file {
    static constexpr int invalid = -1;

    int fd { invalid };
    size_t sz { 0 };
    void *ptr { nullptr };

    static size_t get_size(int fd) {
        if (fd == invalid) {
            return 0;
        }

        return (size_t)lseek(fd, 0, SEEK_END);
    }

    mapped_file(const char *path)
        : fd { open(path, O_RDONLY) }
        , sz { get_size(fd) }
        , ptr { mmap(nullptr, sz, PROT_READ, MAP_PRIVATE, fd, 0) } {}

    mapped_file(const mapped_file &) = delete;
    mapped_file(mapped_file &&) = delete;

    ~mapped_file() {
        if (ptr && fd != invalid) {
            munmap(ptr, sz);
            close(fd);
        }
    }

    auto get_span() const noexcept {
        return std::span<const std::byte>(reinterpret_cast<std::byte *>(ptr), sz);
    }
};

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << argv[0] << " hash file\n";
        std::cerr << "hash is one of: sha-224, sha-256, sha-384, sha-512, sha-512/223, sha-512/256, sha3-224, sha3-256, sha3-384, sha3-512, \n";
        std::cerr << "  shake-128/n, shake-256/n (where n is 32/64/128/256/512/1024/2048),\n";
        std::cerr << "  xxhash32, xxhash64\n";
        return 1;
    }

    const auto h = std::string_view(argv[1]);
    const auto f = mapped_file(argv[2]);

    if (f.fd == mapped_file::invalid) {
        std::cerr << "can't open file!\n";
        return 1;
    }

    const auto start = std::chrono::high_resolution_clock::now();

    if (h == "sha-224") {
        std::cout << cthash::sha224 {}.update(f.get_span()).final() << "\n";
    } else if (h == "sha-256") {
        std::cout << cthash::sha256 {}.update(f.get_span()).final() << "\n";
    } else if (h == "sha-384") {
        std::cout << cthash::sha384 {}.update(f.get_span()).final() << "\n";
    } else if (h == "sha-512") {
        std::cout << cthash::sha512 {}.update(f.get_span()).final() << "\n";
    } else if (h == "sha-512/224") {
        std::cout << cthash::sha512t<224> {}.update(f.get_span()).final() << "\n";
    } else if (h == "sha-512/256") {
        std::cout << cthash::sha512t<256> {}.update(f.get_span()).final() << "\n";
    } else if (h == "sha3-224") {
        std::cout << cthash::sha3_224 {}.update(f.get_span()).final() << "\n";
    } else if (h == "sha3-256") {
        std::cout << cthash::sha3_256 {}.update(f.get_span()).final() << "\n";
    } else if (h == "sha3-384") {
        std::cout << cthash::sha3_384 {}.update(f.get_span()).final() << "\n";
    } else if (h == "sha3-512") {
        std::cout << cthash::sha3_512 {}.update(f.get_span()).final() << "\n";
    } else if (h == "shake-128/32") {
        std::cout << cthash::shake128 {}.update(f.get_span()).final<32>() << "\n";
    } else if (h == "shake-128/64") {
        std::cout << cthash::shake128 {}.update(f.get_span()).final<64>() << "\n";
    } else if (h == "shake-128/128") {
        std::cout << cthash::shake128 {}.update(f.get_span()).final<128>() << "\n";
    } else if (h == "shake-128/256") {
        std::cout << cthash::shake128 {}.update(f.get_span()).final<256>() << "\n";
    } else if (h == "shake-128/512") {
        std::cout << cthash::shake128 {}.update(f.get_span()).final<512>() << "\n";
    } else if (h == "shake-128/1024") {
        std::cout << cthash::shake128 {}.update(f.get_span()).final<1024>() << "\n";
    } else if (h == "shake-128/2048") {
        std::cout << cthash::shake128 {}.update(f.get_span()).final<2048>() << "\n";
    } else if (h == "shake-256/32") {
        std::cout << cthash::shake256 {}.update(f.get_span()).final<32>() << "\n";
    } else if (h == "shake-256/64") {
        std::cout << cthash::shake256 {}.update(f.get_span()).final<64>() << "\n";
    } else if (h == "shake-256/128") {
        std::cout << cthash::shake256 {}.update(f.get_span()).final<128>() << "\n";
    } else if (h == "shake-256/256") {
        std::cout << cthash::shake256 {}.update(f.get_span()).final<256>() << "\n";
    } else if (h == "shake-256/512") {
        std::cout << cthash::shake256 {}.update(f.get_span()).final<512>() << "\n";
    } else if (h == "shake-256/1024") {
        std::cout << cthash::shake256 {}.update(f.get_span()).final<1024>() << "\n";
    } else if (h == "shake-256/2048") {
        std::cout << cthash::shake256 {}.update(f.get_span()).final<2048>() << "\n";
    } else if (h == "xxhash32") {
        std::cout << cthash::xxhash32 {}.update(f.get_span()).final() << "\n";
    } else if (h == "xxhash64") {
        std::cout << cthash::xxhash64 {}.update(f.get_span()).final() << "\n";
    } else {
        std::cerr << "unknown hash function!\n";
        return 1;
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const auto dur = end - start;

    std::cerr << "and it took " << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() << " ms\n";
}
