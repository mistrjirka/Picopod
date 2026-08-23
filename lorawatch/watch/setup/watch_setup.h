#pragma once

// Initializes hardware, DTProtocol, BLE and the LVGL application.
// Returns false when a required subsystem could not be initialized safely.
bool watchSetup();

// Services radio, BLE, sensors, power events and the complete UI.
void watchLoop();
