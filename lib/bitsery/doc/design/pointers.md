*document in progress*

## Extensions
Raw pointers are managed by three extensions:
  * **PointerOwner** - manages a lifetime of the pointer, creates or destroys if required.
  * **PointerObserver** - doesn't own pointer so it doesn't create or destroy anything.
  * **ReferencedByPointer** - when a non-owning pointer (*PointerObserver*) points to reference type, this extension marks this object as a valid target for PointerObserver.

"Smart" pointers, from c++ standard lib (std), are managed by:
 * **StdSmartPtr** - can accept unique_ptr, shared_ptr and weak_ptr

## Implementation details

All aforementioned extensions derive from single base class `PointerObjectExtensionBase`.
This base class accepts three template parameters that customise its behaviour.
  * `TPtrManager\<T\>` - describes how particular pointer type should be handled:
  pointer creation/destruction describes a type of pointer (e.g. owning, shared, observer), and how to actually get value for pointer object.
  This is the place to start if you want to implement pointer support for your custom type. \<T\> is type of pointer object, e.g. `std::unique_ptr<MyType>`, `MyType*`
  * `TPolymorphicContext\<RTTI\>` - provides the functionality to register class hierarchies for your types with serializer, and deserializer, in order to polymorphically serialize/deserialize objects.
  \<RTTI\> template parameter provides runtime information about a type that is used to construct class hierarchies and save them to read/write them to buffer.  
  * `RTTI` - this template parameter provides information if a type is polymorphic, and if it is, then it is used in `TPolymorphicContext\<RTTI\>`.
  Some pointer managers, like `PointerObserver` and `ReferencedByPointer` never requires polymorphic context. In these cases, you need to provide RTTI that will return `isPolymorphic`=false for all types.
  By default all pointers extensions use `StandardRTTI` from `/ext/utils/rtti_utils.h` that internally uses `typeid` and `dynamic_cast`.
  If your environment doesn't allow RTTI, you can provide your own RTTI for your types.

## Allocation and memory resources

Allocation is implemented using memory resources (similar to `std::pmr::memory_resource` from c++17),
and it is called `MemResourceBase` from "ext/utils/memory_allocator.h". The core difference between standard one
is that it has additional `size_t typeId` field for `allocate` and `deallocate` methods, and this typeId is returned from `RTTI`.
There are few options to customise pointer allocation for your pointers by using `MemResourceBase`:
  * invoke `setMemResource` in `PointerLinkingContext`.
  * pass memory resource to pointer manager constructor, along with boolean parameter that specifies if this memory resource should propagate when deserializing child objects.
If no memory resource is provided, then `MemResourceNewDelete` is used, which calls `::operator new(bytes)` and `::operator delete(ptr)`.

**IMPORTANT**: there are few things that you should know to correctly use custom allocations with `StdSmartPtr`:
  * Memory resource must live as long as the last object, that was allocated with it (this is required by std::shared_ptr, custom deleter is provided, that will be able to deallocate correctly when a shared pointer is destroyed).
  * std::unique_ptr is allocated and deallocated using provided memory resource.
  If you create unique pointers your self, make sure that it uses the same memory resource, because bitsery will not call `.reset` method on pointer, and instead `.release()` it and deallocate manually.
