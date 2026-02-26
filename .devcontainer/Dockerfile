# PaperistのTeXLive日本語環境を含む公式イメージを採用
FROM ghcr.io/paperist/texlive-ja:debian

# 必要なパッケージ（git等）をインストール
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    git \
    ssh \
    curl \
    ca-certificates \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*
