# mcr_camera_2026base

GR-PEACH (RZ/A1H) をベースとした**マイコンカーラリー (MCR) カメラクラス**向け制御用ベースプロジェクト。
mbed OS を使わず `iodefine.h` でレジスタを直接操作するベアメタル環境で、1ms タイマー割り込みによる
3 フェーズ実行モデル（Input → Logic → Output）を提供します。全モジュールは共通インターフェース
`IModule`（`init` / `updateInput` / `updateOutput`）を実装し、モジュール間のデータ受け渡しは
共有構造体 `SystemData` に集約しています。


## 設計アーキテクチャ

本プロジェクトは **EMA (Embedded-Module-Architecture)** に準拠しています。

- **正本リポジトリ:** https://github.com/takushio2525/Embedded-Module-Architecture (MIT License)
- **規約:** 同リポジトリの [`ARCHITECTURE.md`](https://github.com/takushio2525/Embedded-Module-Architecture/blob/main/ARCHITECTURE.md)
- **本プロジェクトの準拠状況:** `doc/main.pdf` の付録「EMA 準拠状況」に全数照合の結果を記載

EMA は `IModule` / `SystemData` / `{Module}Config` の 3 点セットでモジュールを構成し、
`loop()` を Input → Logic → Output の 3 フェーズに固定する設計パターンです。
本プロジェクトはこれを mbed OS 非依存のベアメタル環境へ適用した実装例にあたります。

新しいモジュールを追加するときは、まず上記 `ARCHITECTURE.md` の
「新規モジュール追加チェックリスト」を参照してください。

## 環境構築

### 開発環境（ファームウェアビルド）

- **IDE:** Renesas e2 studio
- **コンパイラ:** GNU Arm Embedded Toolchain（`arm-none-eabi-gcc` / `arm-none-eabi-g++` **13.3.1**）
- **ターゲット:** GR-PEACH (RZ/A1H, ARM Cortex-A9)

e2 studio でプロジェクトを開き、**HardwareDebug** 構成でビルドしてください。

#### ビルド検証環境

ビルドが通ることを確認済みの組み合わせです。バージョン差でビルドが通らなくなることがあるため、
別環境で再構築する場合はまずこの組み合わせを再現してください。

| 項目 | 確認済みの値 |
|------|--------------|
| IDE | Renesas e2 studio（**バージョン確認中** — 判明したら追記） |
| ビルド構成 | `HardwareDebug` |
| ツールチェイン | Arm GNU Toolchain `arm-none-eabi` 13.3 rel1 |
| コンパイラ | `arm-none-eabi-gcc` / `arm-none-eabi-g++` 13.3.1 |
| `.cproject` 登録値 | `toolchain.id = gcc-arm-embedded` / `toolchain.version = 13.3.1.arm-13-24` |
| CPU / 命令セット | Cortex-A9 / ARM (`-mcpu=cortex-a9 -marm`) |
| FPU / Float ABI | VFPv3-D16 / hard (`-mfpu=vfpv3-d16 -mfloat-abi=hard`) |
| エンディアン | リトルエンディアン (`-mlittle-endian`) |
| 最適化 | なし (`-O0`)、デバッグ情報 DWARF 4 (`-g -gdwarf-4`) |

> 上記の値は `.cproject` と、git 履歴に残る `HardwareDebug/` のビルド生成物から実測したものです。
> ターゲット設定の詳細は `doc/main.tex` の「§6.1 ビルド検証環境」を参照してください。

### ドキュメントビルド環境（LaTeX）

プロジェクトの仕様書は LaTeX で管理しています。コンパイルには DevContainer を使用します。

1. VSCode で本プロジェクトを開く
2. コマンドパレットから「**Dev Containers: Reopen in Container**」を実行
3. `doc/main.tex` を開く
4. **LaTeX Workshop** 拡張機能の緑色の再生ボタン（▶）をクリックしてコンパイル

> ファイル保存時にも自動コンパイルされます（`autoBuild.run: onFileChange`）。

## ドキュメント

詳細な仕様・設計情報は `doc/main.tex`（PDF: `doc/main.pdf`）を参照してください。
以下を含みます。

- システムアーキテクチャ（3 フェーズモデル、`SystemData`、`ModuleTimer`）
- 全モジュールの API 仕様
- パラメータ調整ガイド（`src/core/ProjectConfig.h` の実値一覧）
- ベアメタル固有の実装知見（XIP 制約、GIC 初期化順序、グローバル ctor の手動実行 等）
- 付録: 不具合と対策の記録（症状 → 原因 → 対策の対応表）

> 旧仕様書 `mcr_camera_2026base_SPEC.md` は内容を `doc/main.tex` へ統合したうえで廃止しました。
> 修正履歴は付録「不具合と対策の記録」に再構成してあります。
