Extensions are at the heart of bitsery. They allow implementing all sorts of things, that requires customizing serialization and deserialization flows separately.
Bitsery already provides a lot of useful extensions, which can be found [here](../../include/bitsery/ext).

Let's see what are the core components of an extension:

1. Extension class itself, which implements templated `serialize` and `deserialize` methods. These functions provide similar capabilities to `save` and `load` functions in other frameworks e.g. [cereal](https://uscilab.github.io/cereal/) or [boost](https://www.boost.org/doc/libs/1_71_0/libs/serialization/doc/index.html), but are more powerful because extension itself can store extension related data as well, that can be used for additional functionality.
```cpp
class MyExtension {
public:
    template<typename Ser, typename T, typename Fnc>
    void serialize(Ser& ser, const T& obj, Fnc&& fnc) const {
       ...
    }

    template<typename Des, typename T, typename Fnc>
    void deserialize(Des& des, T& obj, Fnc&& fnc) const {
       ...
    }
};
```
2. `ExtensionTraits` specialization for an extension, which specifies how it should be used:
```cpp
namespace bitsery {
    namespace traits {
        template<typename T>
        struct ExtensionTraits<ext::MyExtension, T> {
            using TValue = ...;
            static constexpr bool SupportValueOverload = ...;
            static constexpr bool SupportObjectOverload = ...;
            static constexpr bool SupportLambdaOverload = ...;
        };
    }
}
```

Now, that we know the core components of an extension, let's see how everything fits together.

An Extension can be called in 3 different ways, and `Support...Overload` methods basically define, what call syntax can be used with a particular extension.
* `SupportValueOverload` - allows to call extension by providing the size of the value type, the same as `valueNb` function. e.g. `s.ext4b(value, MyExtension{})`.
* `SupportObjectOverload` - allows to call extension the same as simple `object` function. e.g. `s.ext(value, MyExtension{})`.
* `SupportLambdaOverload` - allows to call extension by providing a custom lambda. e.g. `s.ext(value, MyExtension{}, [](...) { ... })`.

You might wonder, how there are 3 ways to call an extension, but only one signature for `serialize` and `deserialize` functions?

This is where a `TValue` from `ExtensionTraits` and the third parameter `Fnc` in `serialize` and `deserialize` comes in.

In case of lambda overload is called, the lambda is passed straight to the serialize/deserialize function as the third parameter. In theory `SupportLambdaOverload` can be any object, not necessary a callable object.
When value overload is used, then lambda is constructed by bitsery like this `[](Serializer& s, VType &v) { s.value<VSIZE>(v); }`, where `VType` is equal to `TValue` from `ExtensionTraits`.
Similarly, when object overload is used `[](Serializer& s, VType &v) { s.object(v); }` lambda is constructed.

When there is no direct mapping from object type to its underlying value type, you can disable lambda generation for value and object overload, by setting `TValue=void`, in this case, a "dummy" lambda will be provided.
