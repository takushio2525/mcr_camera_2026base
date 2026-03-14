# mcr_camera_2026base

## プロジェクト概要

マイコンカーラリー(MCR)向けカメラベース走行制御システム。
GR-PEACH (RZ/A1H, ARM Cortex-A9) をターゲットとしたベアメタルC/C++プロジェクト。

参考プロジェクト `mcr_shiozawa_cclass_2.38m-s`（Mbed OS、最大速度2.38m/s）をベアメタルに再実装している。
現在はカメラ入力+デバッグ表示の基盤が完成済み。今後 Motor, Servo, Encoder 等を追加予定。

---

## ビルド方法

- **IDE**: e2 studio（Eclipse CDT ベース）
- **ツールチェイン**: GCC for Renesas ARM (`arm-none-eabi-gcc/g++` 13.3.1)
- **ターゲット**: Cortex-A9, VFPv3-D16 (hard ABI), Little-endian
- **ビルド構成**: HardwareDebug
- **実行方式**: XIP（SPIフラッシュ直接実行、MMU/L1キャッシュ無効）

e2 studio の GUI からビルド。コマンドラインビルドは非対応。

---

## アーキテクチャ

### 実行モデル

OSTM0 による **1ms 周期割り込み駆動**。メインループはフレーム完了待ち+表示のみ。

```
[1ms割り込み]
  ├─ g_camera.update()    // ステップ分割フレーム処理（各ステップ ~8ms）
  ├─ (将来: Motor/Servo/Encoder の update)
  └─ g_onboard.update()   // GPIO ラッチ反映

[メインループ]
  └─ フレーム完了待ち → シリアル表示
```

### IModule パターン

全ドライバは `IModule`（`src/core/IModule.h`）を継承し、`init()` / `update()` を実装する。
- `init()`: ハードウェア初期化（main から1回だけ呼ぶ）
- `update()`: 周期処理（1ms 割り込みから呼ばれる）

### 初期化順序（厳守）

```
g_onboard.init()   → g_serial.init()   → initGIC()   → g_camera.init()   → initOSTM0()
```

**GIC は必ず Camera より前に初期化すること。** Camera の VDC5 割り込み登録が GIC に依存するため。

### 3フェーズ設計（将来の走行制御向け）

```
Input  : Camera, Encoder 等のセンサ読み取り
Logic  : ライン検出、PID 演算等
Output : Motor, Servo 等のアクチュエータ出力
```

---

## ディレクトリ構成

```
src/
├── mcr_camera_2026base.cpp   # main(), GIC/OSTM0 初期化, 割り込みコールバック
├── core/
│   └── IModule.h             # ドライバ基底インターフェース
└── drivers/
    ├── Camera.h / Camera.cpp   # NTSC 160x120 キャプチャ（VDC5+DVDEC）
    ├── Onboard.h / Onboard.cpp # LED(P6_12-15) / SW(P6_0) GPIO制御
    ├── Serial.h / Serial.cpp   # SCIF2 シリアル通信 (230400bps)
    └── video/                  # VDC5/DVDEC ドライバ群（DisplayBase API）

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
- **モジュール追加**: `IModule` を継承し `init()` / `update()` を実装
- **グローバルインスタンス**: `g_xxx` 命名（例: `g_camera`, `g_serial`, `g_onboard`）
- **GPIO制御**: ラッチ方式 — `setXxx()` で値をバッファし、`update()` で一括反映
- **新ドライバの配置**: `src/drivers/` に `.h` / `.cpp` ペアで作成
- **ヘッダガード**: `#ifndef DRIVERS_XXX_H_` 形式

### クロック定数

| クロック | 周波数 | 用途 |
|---------|--------|------|
| P0Φ | 33.33 MHz | OSTM0（`CMP = 33333` で 1ms） |
| P1Φ | 66.67 MHz | SCIF2（`BRR = 35` で 230400bps） |

---

## 注意事項

### XIP 制約

SPIフラッシュからの直接実行のため、メモリコピー (`imageCopy`) に **~8ms** かかる。
Camera の `update()` はステップ分割（4ステップ × 1ms割り込み）で対応済み。

### generate/ の手動修正箇所

`generate/inthandler.c` は自動生成だが、以下を手動で追加している:
- **動的IRQハンドラテーブル** `g_irq_handlers[256]`（VDC5 割り込み用）
- **`INT_Excep_IRQ()`**: ICCIAR 読み取り → ハンドラ呼び出し → ICCEOIR 書き込み
- **`INT_Excep_OSTMI0()`**: `ostm0_interrupt_callback()` の呼び出し

e2 studio でコード生成を再実行すると**これらの修正が上書きされる**ため注意。

### Camera フレーム処理

インターレース NTSC を 4 ステップで処理:
- Step 0-1: `imageCopy()` — YCbCr422 フレームバッファからコピー
- Step 2-3: `extractBrightness()` — Y（輝度）抽出 → 160x120 グレースケール
- Vfield トグル待ちでフィールド境界を検出してから次フレーム開始

### 今後追加予定のモジュール

Motor, Servo, Encoder — いずれも `IModule` パターンで `src/drivers/` に追加し、
`main()` の初期化順序と `ostm0_interrupt_callback()` の `update()` 呼び出しに組み込む。
