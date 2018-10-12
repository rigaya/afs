
# 自動フィールドシフト 高速化版
  by rigaya  
  original by [aji様](http://www.geocities.jp/aji_0/)

自動フィールドシフト 高速化版は[aji様](http://www.geocities.jp/aji_0/)の
自動フィールドシフトを勝手に高速化したものです。

もともとMMX命令で高速化されているものを、
AVX512 / AVX2 / AVX / SSE4.1 / SSSE3 / SSE2
などにより高速化しました。
環境に合わせて、最速のものが自動的に選択されます。

## ダウンロード & 更新履歴
[rigayaの日記兼メモ帳＞＞](http://rigaya34589.blog135.fc2.com/blog-category-14.html)

## 基本動作環境
Windows Vista, 7, 8, 8.1, 10 (x86/x64)  
Aviutl 0.99g4 以降

## 注意事項
無保証です。自己責任で使用してください。  
自動フィールドシフト 高速化版を使用したことによる、いかなる損害・トラブルについても責任を負いません。  

## afs.auf.iniによる設定

afs.aufと同じフォルダに、下記のように記述したafs.auf.iniを置くと、使用するSIMD関数を選択したり、Windows Large Pageを使用できるように動作を変更できます。変更の際には、Aviutlの再起動が必要です。

基本的にはテスト用です。

使用されているSIMD関数群は自動フィールドシフト設定画面の上部に表示されます。

```
[AFS]
simd=auto
large_page=0
```

|simd="?" |使用されるもの|
|:---|:---|
| auto   | 環境に合わせ自動選択 |
| avx512 | avx512 (xbyak最適化版含む) |
| avx2   | avx2  (xbyak最適化版含む) |
| avx2_intrinsic | avx2  (xbyak最適化版含まず) |
| avx | 128bit-AVX |
| sse4.1 | SSE4.1 |
| ssse3 | SSSE3 |
| sse2 | SSE2 |


|large_page="?" |使用されるもの|
|:---|:---|
| 0 | Windows Large Pageを使用しない (デフォルト) |
| 1 | Windows Large Pageを使用する |

注1: 現状、AVX512を使っても特に速くなりません…。  
注2: 現状、Large Pageを使っても特に速くなりません…。  
注3: 一部のバージョンのWin10は、Large Pageにバグがあり、正常に動作しないようです。  

## 謝辞
素晴らしいプラグインを公開してくださっている、[aji様](http://www.geocities.jp/aji_0/)に深く感謝いたします。

また、本プラグインは[xbyak](https://github.com/herumi/xbyak)を使用したAVX512/AVX2コード生成を行っています。素敵なJITアセンブラを開発してくださり、感謝申し上げます。


### ソースの構成
VCビルド  
文字コード: UTF-8-BOM  
改行: CRLF  
インデント: 空白x4  
