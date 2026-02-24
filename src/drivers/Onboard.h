/*
 * Onboard.h
 *
 *  Onboard LED/SW Driver (Latch System)
 *  Object-Oriented Version (Instantiated)
 */

#ifndef DRIVERS_ONBOARD_H_
#define DRIVERS_ONBOARD_H_

#include "../core/IModule.h"

// LED Count
#define ONBOARD_LED_COUNT 4

class Onboard : public IModule {
public:
  // Constructor
  Onboard();

  // Initialize GPIOs
  void init() override;

  // Set USER LED state (Latched: not reflected to GPIO yet)
  // val: 0=OFF, 1=ON
  void setUserLed(int val);

  // Set Full Color LED state (Latched: not reflected to GPIO yet)
  // r, g, b: 0=OFF, 1=ON
  void setColorLed(int r, int g, int b);

  // Reflect latched LED states to GPIO registers in one go
  void update() override;

  // Read USER SW
  // Return: 1=Pressed, 0=Released
  int sw();

private:
  // LED State Latch Buffer
  int ledState_[ONBOARD_LED_COUNT];
};

#endif /* DRIVERS_ONBOARD_H_ */
