/*
 * Onboard.h
 *
 *  Onboard LED/SW Driver (Latch System)
 *  setLed() saves the state, and update() reflects it to GPIO all at once.
 */

#ifndef DRIVERS_ONBOARD_H_
#define DRIVERS_ONBOARD_H_

// LED Count
#define ONBOARD_LED_COUNT 4

class Onboard {
public:
  // Initialize GPIO directions
  static void init();

  // Set LED state (Latched: not reflected to GPIO yet)
  // id: 0=RED, 1=GREEN, 2=BLUE, 3=USER
  // val: 0=OFF, 1=ON
  static void setLed(int id, int val);

  // Reflect latched LED states to GPIO registers in one go
  static void update();

  // Read USER SW
  // Return: 1=Pressed, 0=Released
  static int sw();

private:
  // LED State Latch Buffer
  static int ledState_[ONBOARD_LED_COUNT];
};

#endif /* DRIVERS_ONBOARD_H_ */
