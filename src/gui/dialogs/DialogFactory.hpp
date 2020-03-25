#include "DialogLoadUnload.hpp"
#include "static_alocation_ptr.hpp"

class DialogFactory {
    DialogFactory() = delete;
    DialogFactory(const DialogFactory &) = delete;
    using mem_space = std::aligned_union<0, /*DialogNONE,*/ DialogLoadUnload>::type;
    static mem_space all_dialogs;

public:
    //define factory methods for all dialogs here
    static static_unique_ptr<IDialogStateful> load_unload(uint8_t data);
};
