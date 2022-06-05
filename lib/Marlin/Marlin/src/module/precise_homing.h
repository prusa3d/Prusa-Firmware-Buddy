#pragma once
#include <cstdint>

/// Finds highest weighted modus of standard moduses (modi) and returns its position.
/// Values has to be 0 - 1023 in circular manner.
///
/// Motivation: Find a place where the most homings will be treated as valid.
///
/// Valid homing is a place which is not too far from a calibrated position (we are searching for).
/// So for each place we take into account all the positions not too far away (distance limit = HOMING_MSCNT_THRS).
///
/// Modus is a place with the highest number of occurrences (homings). However, in case of discrete data,
/// there could be more such places. So we take all such places and pick the best one (with the highest weighted modus).
/// Since we know the distances of near homings we can also evaluate a weighted distance for the evaluated position.
/// The closer the homing is, the higher weight is. Non-valid homings will get 0 weight. We use linear weighting.
///
/// We don't pick the highest weighted modus from all positions because the modus does not need to be the highest there
/// which is the primary goal.
///
/// \param positions array of samples
/// \param samples size of the array
/// \param range max. distance of sample to be considered as valid
int home_modus(uint16_t *positions, uint32_t samples, uint32_t range);

/// computes nearest (signed) distance to calibrated value
/// in 1024 circle
int to_calibrated(const int calibrated, const int value);
