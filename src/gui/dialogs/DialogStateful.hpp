#pragma once

#include "IDialog.hpp"

class IDialogStateful : public IDialog {
protected:
    int16_t id_capture;
    const char *_name;
    int16_t id;

public:
    IDialogStateful(const char *name);
    virtual void Change(uint8_t phase, uint8_t progress_tot, uint8_t progress); // = 0; todo should be pure virtual
    virtual ~IDialogStateful();
};

struct StatefulState {
};

//parent for stateful dialogs dialog
template <int SZ>
class DialogStateful : public IDialogStateful {
protected:
    StatefulState states[SZ];

public:
    DialogStateful(const char *name)
        : IDialogStateful(name) {};
};
