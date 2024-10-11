#include "MItem_MK3.5.hpp"

MI_PINDA::MI_PINDA()
    : MenuItemAutoUpdatingLabel(_("P.I.N.D.A."), "%i",
        [](auto) {
            return buddy::hw::zMin.read() == buddy::hw::Pin::State::low;
        } //
    ) {
}
