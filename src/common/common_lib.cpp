template <class T>
bool set_changed_state(const T current_state, T *old_state) {
    const bool changed = (current_state != *old_state);
    if (changed)
        *old_state = current_state;
    return changed;
}
