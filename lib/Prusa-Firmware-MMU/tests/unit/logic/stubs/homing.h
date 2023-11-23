#pragma once

namespace logic {
class CommandBase;
}

void SimulateIdlerHoming(logic::CommandBase &cb);
void SimulateIdlerMoveToParkingPosition(logic::CommandBase &cb);
void SimulateIdlerWaitForHomingValid(logic::CommandBase &cb);

void SimulateSelectorHoming(logic::CommandBase &cb);
void SimulateSelectorWaitForReadyState(logic::CommandBase &cb);
void SimulateSelectorWaitForHomingValid(logic::CommandBase &cb);

void SimulateSelectorAndIdlerWaitForReadyState(logic::CommandBase &cb);

void SimulateIdlerAndSelectorHoming(logic::CommandBase &cb);
bool SimulateFailedHomeFirstTime(logic::CommandBase &cb);
bool SimulateFailedHomeSelectorRepeated(logic::CommandBase &cb);
