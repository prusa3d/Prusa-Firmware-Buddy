#pragma once

#include "DialogLoadUnload.hpp"
#include "static_alocation_ptr.hpp"
#include <array>

class DialogFactory {
    DialogFactory() = delete;
    DialogFactory(const DialogFactory &) = delete;
    using mem_space = std::aligned_union<0, DialogLoadUnload>::type;
    static mem_space all_dialogs;

public:
    typedef static_unique_ptr<IDialogStateful> (*fnc)(uint8_t data); //function pointer definition
    using Ctors = std::array<fnc, size_t(ClinetFSM::_count)>;
    //define factory methods for all dialogs here
    static static_unique_ptr<IDialogStateful> serial_printing(uint8_t data);
    static static_unique_ptr<IDialogStateful> load_unload(uint8_t data);

    static Ctors GetAll(); //returns all factory methods in an array
};
