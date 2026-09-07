... TODO explain step-by-step what we need and how to get there

Instead I immediately provide implementation for an extension.

```cpp
#include "../details/adapter_common.h"
#include "../traits/core/traits.h"

namespace bitsery {

    namespace ext {

        template<size_t VERSION>
        class Version {
        public:

            template<typename Ser, typename T, typename Fnc>
            void serialize(Ser &ser, const T &v, Fnc &&fnc) const {
                details::writeSize(ser.adapter(), VERSION);
                fnc(ser, const_cast<T&>(v), VERSION);
            }

            template<typename Des, typename T, typename Fnc>
            void deserialize(Des &des, T &v, Fnc &&fnc) const {
                size_t version{};
                details::readSize(des.adapter(), version, 0u, std::false_type{});
                fnc(des, v, version);
            }

        };
    }

    namespace traits {
        template<typename T, size_t V>
        struct ExtensionTraits<ext::Version<V>, T> {
            using TValue = T;
            static constexpr bool SupportValueOverload = false;
            static constexpr bool SupportObjectOverload = false;
            static constexpr bool SupportLambdaOverload = true;
        };
    }

}
```
Adding such extension to the bitsery itself is impractical because it is very easy to implement, but at the same time it has a lot of customization options that actual user might require e.g.:
* how do you want to handle reading/writing version number? (in this case use compact representation as size, but do not check for errors if version number is too large)
* maybe you want to be able to set ReaderError, when version is larger than deserialization implementation handles. (we simply ignore this case and later we'll probably get reading error later anyway)
* or maybe you want to wrap object in `Growable` extension? so that you could ignore unknown fields in newer version of object.

Example of how to use this provided implementation:
```cpp

struct TypeV2 {
    uint16_t x{};
    uint16_t y{};
};

template <typename S>
void serialize(S& ser, TypeV2& obj) {
    ser.ext(obj, bitsery::ext::Version<2u>{}, [](S& s, TypeV2&o, size_t version) {
        s.value2b(o.x);
        if (version == 2u) {
            s.value2b(o.y);
        }
    });
}
```
