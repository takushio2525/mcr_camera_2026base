/*
 * test_module_contract.cpp
 *
 *  IModule::init() の戻り値契約（core/ModuleInit.h の initModule()）のテスト。
 *
 *  検証する契約:
 *    1. init() が true  → enabled は true のまま
 *    2. init() が false → enabled が false に落ちる
 *    3. enabled=false のモジュールは 3 フェーズ実行から外れる
 *
 *  3 について: ファーム側のループ本体は mcr_camera_2026base.cpp の
 *  ostm0_interrupt_callback()（s_inputModules / s_outputModules）と
 *  runMainLoop() / runDebugLoop() の LineDetector・SDLogger 呼び出しにある。
 *  ISR は 1ms 周期という時間制約があり、テストのために関数へ切り出すと
 *  実行時間とスタック使用量が変わるため、ループ自体は移動していない。
 *  ここでは同じ形のループをテスト側に置いて、契約が成立することを示す。
 *
 *  Camera::init() の失敗経路（旧 while(1);）についても、
 *  実物の Camera はホストでビルドできない（VDC5 / DisplayBase 依存）ため
 *  「途中の初期化ステップで失敗したら false を返す」という同じ形の
 *  フェイクで検証する。
 *
 *  SystemData をローカルに置くときは必ず `SystemData sys{};` と値初期化する。
 *  SystemData 配下の *Data は NSDMI を持たない集約体なので、`SystemData sys;`
 *  だとスタック上の実体が不定値のままになる。ファーム側の g_sys は
 *  グローバル（BSS ゼロ初期化）なのでこの問題は起きず、テストだけの注意点。
 *  実際 CameraInitContract.FailedCameraIsSkippedByIsrLoop は、
 *  frameReady の初期値がたまたま 0 になるかどうかで成否が変わっていた。
 */

#include <gtest/gtest.h>
#include "core/IModule.h"
#include "core/ModuleInit.h"
#include "core/SystemData.h"

// ====================================================================
// テスト用フェイクモジュール
// init() の成否を外から指定でき、updateInput / updateOutput の
// 呼ばれた回数を数える。
// ====================================================================
class FakeModule : public IModule
{
public:
  explicit FakeModule(bool initResult)
      : initResult_(initResult), initCount_(0), inputCount_(0), outputCount_(0)
  {
  }

  bool init() override
  {
    initCount_++;
    return initResult_;
  }

  void updateInput(SystemData& sys) override
  {
    (void)sys;
    inputCount_++;
  }

  void updateOutput(SystemData& sys) override
  {
    (void)sys;
    outputCount_++;
  }

  int initCount()   const { return initCount_; }
  int inputCount()  const { return inputCount_; }
  int outputCount() const { return outputCount_; }

private:
  bool initResult_;
  int  initCount_;
  int  inputCount_;
  int  outputCount_;
};

// ====================================================================
// Camera::init() を模したフェイク。
// 実物は 5 つの初期化ステップを順に実行し、どこかで失敗したら
// その時点で false を返す（以前はここで while(1); と無限ループしていた）。
// ====================================================================
class FakeCamera : public IModule
{
public:
  // failAtStep: 1〜5 のいずれかで失敗させる。0 なら全ステップ成功。
  explicit FakeCamera(int failAtStep)
      : failAtStep_(failAtStep), stepsDone_(0), inputCount_(0)
  {
  }

  bool init() override
  {
    stepsDone_ = 0;
    for (int step = 1; step <= 5; step++)
    {
      if (step == failAtStep_)
      {
        // 実物の Camera::init() と同じく、ここで無限ループせず false を返す
        return false;
      }
      stepsDone_++;
    }
    return true;
  }

  void updateInput(SystemData& sys) override
  {
    (void)sys;
    inputCount_++;
  }

  int stepsDone()  const { return stepsDone_; }
  int inputCount() const { return inputCount_; }

private:
  int failAtStep_;
  int stepsDone_;
  int inputCount_;
};

// ====================================================================
// ファーム側の 3 フェーズループと同じ形のヘルパ
// （mcr_camera_2026base.cpp:283-289 / :311-317 と同じ判定）
// ====================================================================
static void runInputPhase(IModule** mods, int n, SystemData& sys)
{
  for (int i = 0; i < n; ++i)
  {
    if (mods[i]->enabled)
    {
      mods[i]->updateInput(sys);
    }
  }
}

static void runOutputPhase(IModule** mods, int n, SystemData& sys)
{
  for (int i = 0; i < n; ++i)
  {
    if (mods[i]->enabled)
    {
      mods[i]->updateOutput(sys);
    }
  }
}

// ====================================================================
// initModule() の契約
// ====================================================================

TEST(ModuleInitContract, EnabledDefaultsToTrue)
{
  // init() を呼ぶ前は有効（IModule.h の既定値）
  FakeModule mod(true);
  EXPECT_TRUE(mod.enabled);
}

TEST(ModuleInitContract, SuccessKeepsEnabled)
{
  FakeModule mod(true);
  EXPECT_TRUE(initModule(mod));
  EXPECT_TRUE(mod.enabled);
  EXPECT_EQ(1, mod.initCount());
}

TEST(ModuleInitContract, FailureClearsEnabled)
{
  // ★本コミットの中心的な契約: init() が false を返したら enabled が false になる
  FakeModule mod(false);
  EXPECT_FALSE(initModule(mod));
  EXPECT_FALSE(mod.enabled);
  EXPECT_EQ(1, mod.initCount());
}

TEST(ModuleInitContract, FailureClearsAlreadyEnabledModule)
{
  // 一度 true にしてあっても失敗すれば落ちる（上書きであって OR ではない）
  FakeModule mod(false);
  mod.enabled = true;
  initModule(mod);
  EXPECT_FALSE(mod.enabled);
}

TEST(ModuleInitContract, ReturnValueMatchesInitResult)
{
  FakeModule ok(true);
  FakeModule ng(false);
  EXPECT_EQ(ok.enabled, initModule(ok));
  EXPECT_EQ(ng.enabled, initModule(ng));
}

// ====================================================================
// enabled=false のモジュールが 3 フェーズ実行から外れること
// ====================================================================

TEST(ModuleInitContract, DisabledModuleIsSkippedInInputPhase)
{
  SystemData sys{};  // 値初期化必須（理由はファイル冒頭の注記を参照）
  FakeModule good(true);
  FakeModule bad(false);
  initModule(good);
  initModule(bad);

  IModule* mods[] = {&good, &bad};
  runInputPhase(mods, 2, sys);
  runInputPhase(mods, 2, sys);

  EXPECT_EQ(2, good.inputCount());
  EXPECT_EQ(0, bad.inputCount());
}

TEST(ModuleInitContract, DisabledModuleIsSkippedInOutputPhase)
{
  SystemData sys{};  // 値初期化必須（理由はファイル冒頭の注記を参照）
  FakeModule good(true);
  FakeModule bad(false);
  initModule(good);
  initModule(bad);

  IModule* mods[] = {&good, &bad};
  runOutputPhase(mods, 2, sys);

  EXPECT_EQ(1, good.outputCount());
  EXPECT_EQ(0, bad.outputCount());
}

TEST(ModuleInitContract, WithoutInitModuleTheFailureIsNotIsolated)
{
  // 対照実験: init() を直接呼んで戻り値を捨てると（= 本修正前の main() の形）
  // enabled が true のまま残り、壊れたモジュールが毎周期叩かれ続ける。
  SystemData sys{};  // 値初期化必須（理由はファイル冒頭の注記を参照）
  FakeModule bad(false);
  bad.init();                 // 戻り値を捨てる

  IModule* mods[] = {&bad};
  runOutputPhase(mods, 1, sys);

  EXPECT_TRUE(bad.enabled);
  EXPECT_EQ(1, bad.outputCount());
}

// ====================================================================
// Camera::init() の失敗経路
// ====================================================================

TEST(CameraInitContract, AllStepsOkReturnsTrue)
{
  FakeCamera cam(0);
  EXPECT_TRUE(initModule(cam));
  EXPECT_TRUE(cam.enabled);
  EXPECT_EQ(5, cam.stepsDone());
}

TEST(CameraInitContract, FailureReturnsInsteadOfHanging)
{
  // 各ステップで失敗させても init() は必ず戻ってくる
  // （旧実装は while(1); でここから戻らなかった）
  for (int step = 1; step <= 5; step++)
  {
    FakeCamera cam(step);
    EXPECT_FALSE(initModule(cam)) << "failAtStep=" << step;
    EXPECT_FALSE(cam.enabled)     << "failAtStep=" << step;
    // 失敗したステップの手前までしか進んでいない
    EXPECT_EQ(step - 1, cam.stepsDone()) << "failAtStep=" << step;
  }
}

TEST(CameraInitContract, FailedCameraIsSkippedByIsrLoop)
{
  // カメラ初期化に失敗しても ISR の Input フェーズは回り続け、
  // カメラだけがスキップされる（= ハングしない）
  SystemData sys{};  // 値初期化必須（理由はファイル冒頭の注記を参照）
  FakeCamera cam(2);
  FakeModule onboard(true);
  initModule(cam);
  initModule(onboard);

  IModule* inputs[] = {&onboard, &cam};
  for (int tick = 0; tick < 10; tick++)
  {
    runInputPhase(inputs, 2, sys);
  }

  EXPECT_EQ(10, onboard.inputCount());  // 他モジュールは通常どおり動く
  EXPECT_EQ(0,  cam.inputCount());      // カメラだけ外れる
  // フレームは一度も上がらないので、メインループは frameReady 待ちのまま回る
  EXPECT_FALSE(sys.cam.frameReady);
}
