/// If \param new_state differs from \param old_state
/// \param new_state will be saved to \param old_state
/// \returns true if states differ
template <class T>
bool set_changed_state(const T current_state, T *old_state);
