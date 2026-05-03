/*
 * Onboard.h
 *
 *  Onboard LED/SW Driver (Latch System)
 *  EMA 準拠版: SystemData / Config ベース
 *
 *  Input  : updateInput(sys)  → sys.ob.sw       (P6_0 を読み取り)
 *  Output : updateOutput(sys) → sys.ob.*LedCmd  (P6_12〜15 へラッチ反映)
 *
 *  USER LED は Logic フェーズで sys.ob.userLedCmd に書き込み、
 *  RGB LED も同様に sys.ob.led{Red,Green,Blue}Cmd に書き込む。
 */

#ifndef DRIVERS_ONBOARD_H_
#define DRIVERS_ONBOARD_H_

#include "../core/IModule.h"
#include "../core/ProjectConfig.h"

// LED Count
#define ONBOARD_LED_COUNT 4

class Onboard : public IModule {
public:
  // Config を注入してインスタンス生成
  explicit Onboard(const OnboardConfig& cfg);

  // Initialize GPIOs
  bool init() override;

  // Input  : SW 状態を sys.ob.sw に格納
  void updateInput(SystemData& sys) override;

  // Output : sys.ob.*LedCmd を内部ラッチに転記後、GPIO に一括反映
  void updateOutput(SystemData& sys) override;

  // SW を直接読む（init() 前の起動モード判定用に public 維持）
  // 通常の走行ロジックからは sys.ob.sw 経由で読むこと。
  // Return: 1=Pressed, 0=Released
  int sw();

private:
  // Set USER LED state (内部ラッチ用)
  void setUserLed(int val);

  // Set Full Color LED state (内部ラッチ用)
  void setColorLed(int r, int g, int b);

  OnboardConfig _config;

  // LED State Latch Buffer
  int ledState_[ONBOARD_LED_COUNT];
};

extern Onboard g_onboard;

#endif /* DRIVERS_ONBOARD_H_ */
