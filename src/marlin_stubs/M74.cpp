#include "PrusaGcodeSuite.hpp"

/**
 * M74: Set model weight
 *
 *  W          - the weight of already printed part of the model
 */
void PrusaGcodeSuite::M74() {
    const float weight __attribute__((unused)) = parser.floatval('W');
    // TODO: adjust the Y frequency of the input shaping filter
}
