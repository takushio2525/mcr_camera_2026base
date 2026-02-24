#!/usr/bin/env perl
# latexmk 設定ファイル
# uplatex + dvipdfmx によるビルドチェーン

# uplatex を使用
$latex = 'uplatex -synctex=1 -interaction=nonstopmode -file-line-error %O %S';

# dvipdfmx で PDF 生成
$dvipdf = 'dvipdfmx %O -o %D %S';

# PDF生成モード: 3 = latex + dvipdfmx
$pdf_mode = 3;

# bibtex (必要に応じて)
$bibtex = 'pbibtex %O %B';

# makeindex (必要に応じて)
$makeindex = 'mendex %O -o %D %S';

# 出力ディレクトリ
# $out_dir = 'out';
