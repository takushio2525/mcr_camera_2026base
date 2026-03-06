# mcr_camera_2026base

GR-PEACH (RZ/A1H) をベースとしたマイクロマウス／ロボットカメラーカー向け制御用ベースプロジェクト。
割り込み駆動のアーキテクチャを採用し、クラスベースの `init`/`update` パターンによるモダンな組み込み開発基盤を提供します。

## 環境構築

### 開発環境（ファームウェアビルド）

- **IDE:** Renesas e2 studio
- **コンパイラ:** GCC for Renesas ARM
- **ターゲット:** GR-PEACH (RZ/A1H, ARM Cortex-A9)

e2 studio でプロジェクトを開き、HardwareDebug 構成でビルドしてください。

### ドキュメントビルド環境（LaTeX）

プロジェクトの仕様書は LaTeX で管理しています。コンパイルには DevContainer を使用します。

1. VSCode で本プロジェクトを開く
2. コマンドパレットから「**Dev Containers: Reopen in Container**」を実行
3. `doc/main.tex` を開く
4. **LaTeX Workshop** 拡張機能の緑色の再生ボタン（▶）をクリックしてコンパイル

> ファイル保存時にも自動コンパイルされます（`autoBuild.run: onFileChange`）。

## ドキュメント

詳細な仕様・設計情報・修正履歴は `doc/main.tex`（PDF: `doc/main.pdf`）を参照してください。
