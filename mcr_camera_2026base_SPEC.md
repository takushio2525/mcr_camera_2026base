# mcr_camera_2026base_SPEC

## 1. 概要
GR-PEACH (RZ/A1H) をベースとしたマイクロマウス／ロボットカメラーカー向け制御用ベースプロジェクト。
割り込み駆動のアーキテクチャを採用し、Onboardクラスを用いたLEDラッチ表示制御と、1msタイマーによる周期的な処理基盤を提供します。

## 3. 目的
本ベースプロジェクトを活用し、カメラモジュールやモータ制御といった高機能なロボット制御基盤を効率的に実装できるようにする。
また、処理ロジックを1ms周期の割り込みハンドラ等に統合し、ハードウェア操作をドライバラッパー経由に集約することで、簡潔で見通しの良いコードにする。

## 4. 機能・UXフロー
- 電源投入後、C/C++初期化ルーチンが走り、`main()` が実行される。
- `main()` 内でLEDや各種ペリフェラル（タイマーなど）を初期化し、ループ待機状態となる。
- 実処理は `OSTM0` による1ms周期タイマー割り込み（`ostm0_interrupt_callback`）から起動される。
- 1ms割り込み内で実行される `Onboard::update()` などのハードウェア処理用メソッドにより、遅延なく一括してGPIO等へ反映（ラッチ方式）される。

## 5. データ構造
- 変数 `g_timer_1ms`: 全体で共有するグローバルな1msカウンタ
- `Onboard` クラス内ラッチバッファ: LEDの状態を保存し、`update()` 呼び出し時に一括でポート出力を更新する。

## 6. 技術スタック
- **ターゲット:** GR-PEACH (Renesas RZ/A1H MCU)
- **言語:** C / C++
- **開発環境:** e2 studio
- **コンパイラ:** GCC for Renesas ARM
- **割り込みコントローラ:** GIC (Generic Interrupt Controller)
- **タイマー:** OSTM0 (OS Timer 0) -> 1msインターバルとして使用

## 7. ファイル構成

| ファイル | 役割 |
|----------|------|
| `src/mcr_camera_2026base.cpp` | メインの初期化と 1msタイマー設定、コールバックを実装 |
| `src/drivers/Onboard.h` | オンボードのLED / SWを抽象化したラッチ管理ドライバ の定義 |
| `src/drivers/Onboard.cpp` | Onboardクラスのハードウェア（GPIO）操作の実装 |
| `generate/inthandler.c` | e2 studio 生成の中断ハンドラ実装ファイル（OSTM0コールバックをフック） |
| `doc/main.tex` | プロジェクト等の全体・旧版仕様や関連詳細資料 |

### 機能とファイルの対応表

| 機能 | メインファイル | 関連ファイル |
|------|----------------|--------------|
| オンボードLED・SW操作 | `src/drivers/Onboard.cpp` | `src/drivers/Onboard.h`, `mcr_camera_2026base.cpp` |
| 1msタイマー割り込み処理 | `src/mcr_camera_2026base.cpp` | `generate/inthandler.c` |

## 8. 詳細設計
- **`Onboard コンストラクタ`**: P6の出力・入力モード設定。ラッチバッファの初期化。
- **`Onboard::setLed(id, val)`**: 指定IDのLEDバッファ状態を1または0に保存。ハードウェアにはまだ反映しない。
- **`Onboard::update()`**: LEDバッファ状態をまとめ、GR-PEACHの対象GPIOピン（Low Active）へ実際に出力する。
- **`Onboard::sw()`**: SWのプッシュ状態をポーリングで監視し、押下時1(Active-High)を返す。
- **`initOSTM0()`**: RZ/A1HのOSTM0を1ms用に設定し、GIC(INTC)へ登録。
- **`ostm0_interrupt_callback()`**: `INT_Excep_OSTMI0` から呼ばれ、実時間をカウント(`g_timer_1ms`)し、`g_onboard` ポインタを経由してOnboardインスタンスを操作する。
- **`INT_Excep_IRQ()`**: GIC(INTC)を用いたベクタ割り込みディスパッチャ。ICCIARから要因IDを取得し、`RelocatableVectors` から適切なハンドラへ分岐・EIOを通知する実装。

## 24. 修正履歴

### 2026-02-24 16:45: GIC割り込みディスパッチャ（IRQ）の実装
**変更内容:**
- `generate/inthandler.c` の空関数だった `INT_Excep_IRQ()` に、GICからの割り込み要因取得と `RelocatableVectors` による個別ハンドラへの分岐処理、さらにEOI (End of Interrupt) 応答処理を追加しました。
- `mcr_camera_2026base_SPEC.md` の詳細設計に `INT_Excep_IRQ()` の説明を追記しました。

**解消した問題/不満:**
- タイマー割り込み（OSTM0）などの割り込み要求が発生しても、ベクタテーブルから呼ばれる大元のIRQディスパッチャが空実装であったため、割り込みハンドラが実行されず、さらにGIC側でも割り込み要求がクリアされないという問題。オンボードLEDが点滅しない原因となっていました。

**解決方法:**
- ARM Cortex-A9向けの正しいGIC IRQディスパッチロジック（ICCIARの読み出し〜ハンドラ呼び出し〜ICCEOIRへの書き込み）を実装し、各種タイマ等の個別割り込みハンドラが正しく実行されるようにしました。

### 2026-02-24 16:15: オンボードクラスのインスタンス化対応とGPIO初期化修正
**変更内容:**
- `src/drivers/Onboard.cpp` のスイッチ入力初期化に欠けていたPIPC6やPBDC6のクリア処理を追加。
- 動作安定化のため静的メソッドによる基盤操作から、インスタンスを生成（`Onboard onboard;`）し `g_onboard` ポインタ経由で割り込みハンドラから操作するオブジェクト指向構造にリファクタリング。
**解決方法:**
- Mbed由来のプロジェクト構造を参考に、より安全な初期化シーケンスと呼び出し経路に改修。

### 2026-02-24 16:04: オンボードクラス＆1msタイマーの実装

**変更内容:**
- `src/drivers/Onboard.h`, `Onboard.cpp` を追加し、GR-PEACH上のLEDやUSERスイッチのラッチ方式ドライバを実装。
- `src/mcr_camera_2026base.cpp` に `1ms` 割り込み用の `initOSTM0()` と `ostm0_interrupt_callback()` を実装。
- `generate/inthandler.c` 内の `INT_Excep_OSTMI0` 中断ハンドラから `ostm0_interrupt_callback` をコールするよう改修。
- `mcr_camera_2026base_SPEC.md` （本仕様書）を新規作成。

**解決方法:**
- mcr_camera_test1のコードを反映し、Timer0割り込みベースでのループ周期駆動アーキテクチャを確立。
