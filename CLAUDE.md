# mcr_camera_2026base

## プロジェクト概要

マイコンカーラリー(MCR)向けカメラベース走行制御システム。
GR-PEACH (RZ/A1H, ARM Cortex-A9) をターゲットとしたベアメタルC/C++プロジェクト。

参考プロジェクト `mcr_shiozawa_cclass_2.38m-s`（Mbed OS、最大速度2.38m/s）をベアメタルに再実装している。
カメラ入力+デバッグ表示の基盤に加え、Motor, Servo, Encoder, LineDetector, SDLogger が実装済み。

---

## ビルド方法

- **IDE**: e2 studio（Eclipse CDT ベース）
- **ツールチェイン**: GCC for Renesas ARM (`arm-none-eabi-gcc/g++` 13.3.1)
- **ターゲット**: Cortex-A9, VFPv3-D16 (hard ABI), Little-endian
- **ビルド構成**: HardwareDebug
- **実行方式**: XIP（SPIフラッシュ直接実行）。**MMU と L1 キャッシュは有効**
  （`main()` が `SystemInit()` を呼び、`src/system/system_init.c` で
  `MMU_CreateTranslationTable()` → `MMU_Enable()` → `L1C_EnableCaches()` を実行）

e2 studio の GUI からビルド・確認を行う。**Claude はビルドコマンドを実行しない。**
コードを作成・編集したらユーザーが e2 studio でビルドして確認する。

e2 studio の GUI からビルド。コマンドラインビルドは非対応。

---

## アーキテクチャ

### 実行モデル

OSTM0 による **1ms 周期割り込み駆動**。メインループはフレーム完了待ち+表示のみ。

```
[1ms割り込み] ostm0_interrupt_callback()
  ├─ Input  : g_onboard.updateInput()  // SW → sys.ob.sw
  │           g_camera.updateInput()   // ステップ分割フレーム処理
  ├─ Logic  : applyDrivingPattern()    // 走行モードのみ（デバッグモードは LED だけ）
  └─ Output : g_motor.updateOutput()   // sys.mot.*Cmd → PWM
              g_servo.updateOutput()   // sys.srv.angleCmd → PWM
              g_onboard.updateOutput() // GPIO ラッチ反映

[メインループ]
  ├─ sys.cam.frameReady で g_lineDetector.updateInput()
  ├─ g_sdlogger.updateOutput()  // 走行中ログ記録 + 保存要求処理
  └─ シリアル表示
```

`Encoder` はインスタンスだけ生成されており、`init()` も 3 フェーズ配列への登録も
まだ行っていない（`src/drivers/Encoder.h` 冒頭の注記を参照）。
`Serial` / `SDCard` は `IModule` を継承しているがどちらの配列にも載せていない。

### IModule パターン

全ドライバは `IModule`（`src/core/IModule.h`）を継承する。
- `init()`: ハードウェア初期化（main から1回だけ呼ぶ）。成功 = true
- `updateInput(SystemData&)`: 入力フェーズ（ハードウェア → SystemData）
- `updateOutput(SystemData&)`: 出力フェーズ（SystemData → ハードウェア）
- `deinit()`: 終了処理（デフォルト空実装。現状オーバーライドも呼出もない）

入力専用モジュールは `updateInput` のみ、出力専用は `updateOutput` のみを
override する（未 override 側はデフォルトの空実装が使われる）。

### 初期化順序（厳守）

```
runGlobalConstructors() → SystemInit() →
g_onboard.init() → g_serial.init() → initGIC() → g_sdlogger.init() →
g_camera.init() → g_motor.init() → g_servo.init() → g_lineDetector.init() →
runLogicInit(g_sys) → initOSTM0()
```

**`runGlobalConstructors()` は必ず最初に呼ぶこと。** 後述のとおり
`start.S` が `__libc_init_array()` を呼ばないため、これより前に
グローバルインスタンスを触ると `_config` が BSS ゼロのままになる。

**GIC は必ず Camera より前に初期化すること。** Camera の VDC5 割り込み登録が GIC に依存するため。
**SDLogger は GIC の後、Camera の前に初期化すること。** SPI通信に割り込みは不要だが、カメラより先に初期化して起動時間を短縮する。
**`initOSTM0()` は必ず最後に呼ぶこと。** EMA 化により ISR の Output フェーズが Motor/Servo を毎ms 叩く。OSTM0 を Motor/Servo init より前に起動すると、MTU2 がスタンバイ状態のまま PWM レジスタへ書き込みが入り、サーボ挙動異常やバス例外でハングする。

### 3フェーズ設計

```
Input  : Camera, Onboard(SW) 等のセンサ読み取り        → SystemData へ書く
Logic  : src/logic/RunLogic.cpp の純関数群（29状態の走行状態機械）
Output : Motor, Servo, Onboard(LED) 等のアクチュエータ出力 → SystemData から読む
```

`SystemData`（`src/core/SystemData.h`）が全モジュールの Data を集約するハブ。
各モジュールは原則「自身の Data 構造体だけ」を読み書きし、他モジュールの
データを参照するのは Logic フェーズの関数のみ、というのが基本ルール。
ただし `LineDetector` は性能上の理由で `g_camera.getPixel()` を直接呼んでおり、
これは意図的な例外（`src/drivers/Camera.h` の注記を参照）。

---

## ディレクトリ構成

```
src/
├── mcr_camera_2026base.cpp   # main(), GIC/OSTM0 初期化, 割り込みコールバック,
│                             # 全グローバルインスタンスの実体定義
├── core/
│   ├── IModule.h             # ドライバ基底インターフェース
│   ├── SystemData.h          # 全モジュールの Data を集約するハブ
│   ├── ModuleTimer.h         # g_timer_1ms ベースのノンブロッキング ms 計測
│   └── ProjectConfig.h       # 全 *_CONFIG の実値を一括定義（調整はここ）
├── logic/
│   └── RunLogic.h / .cpp     # 走行状態機械（SystemData ベースの純関数群）
├── system/
│   ├── system_init.c         # SystemInit(): FPU/MMU/L1キャッシュ有効化
│   ├── mmu_rzA1H.c           # MMU 翻訳テーブル生成
│   └── cmsis_*.h, core_ca.h  # CMSIS（ベンダ提供）
└── drivers/
    ├── Camera.h / Camera.cpp       # NTSC 160x120 キャプチャ（VDC5+DVDEC）
    ├── Encoder.h / Encoder.cpp     # MTU2 位相計数エンコーダ
    ├── LineDetector.h / .cpp       # ライン検出
    ├── Motor.h / Motor.cpp         # MTU2 PWM モーター制御
    ├── Onboard.h / Onboard.cpp     # LED(P6_12-15) / SW(P6_0) GPIO制御
    ├── SDCard.h / SDCard.cpp       # SDカード SPI ドライバ (RSPI2)
    ├── SDLogger.h / SDLogger.cpp   # ログバッファ + CSV書き出し
    ├── Serial.h / Serial.cpp       # SCIF2 シリアル通信 (230400bps)
    ├── Servo.h / Servo.cpp         # MTU2 PWM サーボ制御
    ├── fatfs/                      # ChaN's FatFs R0.11a
    │   ├── ff.c / ff.h             # FatFs コア
    │   ├── ffconf.h                # FatFs 設定（ベアメタル用カスタム）
    │   ├── diskio.h / diskio.cpp   # ディスクI/Oブリッジ（→SDCard）
    │   └── integer.h               # 型定義
    └── video/                      # VDC5/DVDEC ドライバ群（DisplayBase API）

generate/                       # e2 studio 自動生成（下記「注意事項」参照）
├── iodefine.h                  # I/O レジスタ定義
├── inthandler.c                # 割り込みディスパッチ ★手動修正あり
├── hwsetup.c, vects.c, ...
└── iodefines/                  # ペリフェラル別レジスタ定義 (40+ファイル)

doc/                            # LaTeX 仕様書
```

---

## コーディング規約

- **コメント**: 日本語
- **モジュール追加**: `IModule` を継承し `init()` と、必要な側の
  `updateInput()` / `updateOutput()` を実装。`{Module}Config` と `{Module}Data` は
  モジュールヘッダ側で宣言し、Config の実値は `ProjectConfig.h`、Data は
  `SystemData` に集約する
- **グローバルインスタンス**: `g_xxx` 命名（例: `g_camera`, `g_serial`, `g_onboard`）
- **GPIO制御**: ラッチ方式 — `setXxx()` で値をバッファし、`updateOutput()` で一括反映
- **新ドライバの配置**: `src/drivers/` に `.h` / `.cpp` ペアで作成
- **ヘッダガード**: `#ifndef DRIVERS_XXX_H_` 形式

### クロック定数

| クロック | 周波数 | 用途 |
|---------|--------|------|
| P0Φ | 33.33 MHz | OSTM0（`CMP = 33333` で 1ms） |
| P1Φ | 66.67 MHz | SCIF2（`BRR = 35` で 230400bps）, RSPI2（SDカードSPI） |

---

## 注意事項

### XIP 制約

SPIフラッシュからの直接実行のため、メモリコピー (`imageCopy`) に **~8ms** かかる。
Camera の `updateInput()` はステップ分割（4ステップ × 1ms割り込み）で対応済み。

**注意**: この「~8ms」は MMU / L1 キャッシュを有効化する前
（コミット `3216ec4` より前）の測定値。現在はキャッシュが効くため実際には
短縮されている可能性が高いが、**再測していないため今も成立するかは不明**。
ステップ分割の要否を判断するときは実機で測り直すこと。

### generate/ の手動修正箇所

`generate/inthandler.c` は自動生成だが、以下を手動で追加している:
- **動的IRQハンドラテーブル** `g_irq_handlers[256]`（VDC5 割り込み用）
- **`INT_Excep_IRQ()`**: ICCIAR 読み取り → ハンドラ呼び出し → ICCEOIR 書き込み
- **`INT_Excep_OSTMI0()`**: `ostm0_interrupt_callback()` の呼び出し

`generate/linker_script.ld` も以下を手動で追加している:
- **`.init_array` 収集** と `__init_array_start` / `__init_array_end` ラベル定義
  （GCC 13 はグローバル C++ ctor を `.init_array` に出力する）

e2 studio でコード生成を再実行すると**これらの修正が上書きされる**ため注意。

### グローバル C++ コンストラクタ手動実行（重要）

`generate/start.S` は `__libc_init_array()` を呼ばず、`HardwareSetup()` の後
直接 `bl main` する。GCC 13 が `.init_array` に出力するグローバル ctor は
そのままだと**一切実行されない**。`main()` 冒頭の `runGlobalConstructors()`
で `__init_array_start..end` を手動反復している。

**EMA 化前は症状が顕在化しなかった**: 各ドライバが `static const` のみ
使用しており ctor 内の処理が無くても問題無かったため。EMA 化で `_config`
メンバが追加されたことで「ctor が走らないと `_config` が BSS ゼロのまま」
になり、`Servo` PWM 出力 0 (ペリフェラルロック)、`LineDetector` 全閾値 0
（検出全崩壊）等の致命的不具合になる。

新しい `g_xxx` グローバルインスタンスを追加する場合、内部状態を ctor 初期化
リストに置いた時点でこの仕組みに依存することを忘れない。

### Camera フレーム処理

インターレース NTSC を 4 ステップで処理:
- Step 0-1: `imageCopy()` — YCbCr422 フレームバッファからコピー
- Step 2-3: `extractBrightness()` — Y（輝度）抽出 → 160x120 グレースケール
- Vfield トグル待ちでフィールド境界を検出してから次フレーム開始

### SDLogger の使い方

走行データの記録・保存フロー:
1. `g_sdlogger.init()` — SDカード初期化 + FatFs マウント（main起動時）
2. `g_sdlogger.current().xxx = value` + `g_sdlogger.commit()` — 走行ループ内でデータ記録
3. `g_sdlogger.saveToSD()` — 走行終了後にCSVバッチ書き出し

ファイル: `/data0000.csv` 〜 `/data9999.csv`（`renban.txt` で連番管理）
バッファ: `SDLOG_T g_logData[4000]`（約2.7MB）

### SDカード ピン割り当て

| 信号 | ピン | 機能 |
|------|------|------|
| MOSI | P8_5 | RSPI2 MOSI (Function 3) |
| MISO | P8_6 | RSPI2 MISO (Function 3) |
| CLK  | P8_3 | RSPI2 RSPCK (Function 3) |
| CS   | P8_4 | GPIO出力 (手動制御) |
| CD   | P7_8 | GPIO入力 (カード検出, Low=挿入) |
