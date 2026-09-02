# SAPPOROBDD++ マニュアル

## パッケージの概要

- このパッケージはBDD/ZDDの基本操作を行うC++のクラスライブラリである。
  本プログラムは、32ビットまたは64ビットの計算機で動作する。
  （コンパイル時に、オプションB_32を指定すると32ビットモードとなり、指定しないと64ビットモードになる。この動作はオリジナルの SAPPOROBDD とは異なることに注意。）
  各操作はC++のメソッド呼び出しにより実行される。

- 入力変数番号(通称VarID)は1から始まるint型の整数で識別する（0は定数を表す）。
  負の変数番号は用いない。VarIDの最大値は定数BDD_MaxVarで与えられる。
  デフォルトは 1048575（20ビット）である。B_EXTEND を定義してコンパイルした
  場合は 32 ビット幅になるが、C++ インタフェースは int で受け渡すため
  INT_MAX に丸められる。

- 各VarIDごとにBDDでの上下の順位(通称level)の情報を保持している。
  levelもまた1から始まるint型の整数で識別する。大きい数値ほど
  上位の変数を表す（BDDの根に近く、先に展開される）。
  VarIDを何も指定せずに生成した場合はVarIDと同じ値のlevelを持つ。

- 論理演算結果のBDDは、64ビットのunsigned long long int（または32ビットのunsigned int）
 （bddwordという名前の型にtypedefされている）のインデックスで返される。
  BDDは論理関数に対して一意であり、インデックスの値もBDDに対して
  一意である。したがって、２つの論理演算結果が等価であるかどうかは、
  演算結果のインデックスの値が同じかどうかを比較することで行える。

- BDD節点テーブルの最大サイズは、BDD_Init() の２つの引数で指定する。
  BDD_Initを省略した場合の default は、初期値 256、最大値 BDD_MaxNode
  （節点テーブルが表現できる最大の節点数）に設定されている。
  計算中に記憶あふれを起こした場合は、計算を中断して
  BDDOutOfMemoryException 例外を投げる（オリジナルの SAPPOROBDD が
  返していた null オブジェクト BDD(-1) は、明示的に生成しない限り現れない）。

## SAPPOROBDD++ 独自の機能

- SAPPOROBDD++ では64ビットバージョンがデフォルトになったため、B_64 マクロの定義は必要なくなった。32ビットバージョンでコンパイルする場合は B_32 マクロを定義する。
- SAPPOROBDD++ ではすべてのパッケージは sapporobdd 名前空間に入れられている。`using namespace sapporobdd` を使用前に書く必要がある。
- SAPPOROBDD++ では ZBDD は ZDD にリネームされている。ZBDD を使い続けることも可能である。ZBDD_Meet 等の関数名も ZDD_Meet にリネームされている。
- キャッシュサイズを変更可能である。キャッシュサイズは、BDD節点テーブルのサイズに対する比率（キャッシュ比率）で設定される。
  キャッシュ比率は、2のべき乗の値（例えば、0.125、0.25、0.5、1、2、4など）でなければならない。
  デフォルトのキャッシュ比率は0.5である。キャッシュ比率は、BDD_Init関数の第3引数で初期設定し、
  後からBDD_SetCacheRatio関数で変更することもできる。また、BDD_GetCacheRatio関数で
  現在の比率を取得できる。
- エラーが発生した際は、BDDException 例外が投げられる。例外クラスは BDDException.h で定義されている。以下の例外（いずれも BDDException の子クラス）が存在する。
  - BDDInvalidBDDValueException
  - BDDOutOfRangeException
  - BDDOutOfMemoryException
  - BDDFileFormatException
  - BDDInternalErrorException

## 提供するクラスとその依存関係

個別のマニュアルがあるのは [ZDD](classes/ZDD.md)（ZDDV と BDD.h の関数を含む）、
[BDDCT](classes/BDDCT.md)、および [BDD_Hash / ZDD_Hash](classes/others.md) である。
それ以外のクラスの個別マニュアルは未整備であり、ヘッダファイルを参照されたい。

- BDD - BDDで表現された個々の論理関数を指すクラス
  - BDDV - BDDの配列（論理関数の配列）を表すクラス
    - BtoI - ２値入力整数値出力の論理関数を表すクラス
  - BDDDG - BDDを単純直交分解した結果を表すクラス
  - [ZDD](classes/ZDD.md) - ゼロサプレス型BDDで表現された組合せ集合を指すクラス
    - ZDDV - ZDDの配列（組合せ集合の配列）を表すクラス
      - CtoI - 整数値組合せ集合（整係数ユネイト論理式）を表すクラス
    - SOP - 正負のリテラルからなる積和形論理式を表現するクラス
      - SOPV - SOPの配列（積和形論理式の配列）を表すクラス
    - PiDD - 順列集合を表現するクラス
    - SeqBDD - 系列集合を表現するクラス
    - GBase - ZDDでパス/サイクル列挙を行うためのクラス
    - [BDDCT](classes/BDDCT.md) - BDD/ZDDでコスト制約付き変数を扱うためのクラス

## BDD クラスに関する注意

- `Univ(g)` / `Exist(g)` の g は消去する変数の集合であり、`BDDvar(x) | BDDvar(y) | ...`
  の形（`Support()` が返す形）で与える。積 `BDDvar(x) & BDDvar(y)` のようなキューブは
  受け付けず、BDDInvalidBDDValueException 例外を投げる（以前は先頭の変数だけが
  消去され、誤りは報告されなかった）。
- `At0()` / `At1()` / `operator~` は BDD 専用である。ZDD の節点を C API の
  bddat0() / bddat1() / bddnot() に渡すと BDDInvalidBDDValueException 例外になる
  （ZDD には OffSet() / OnSet0() を用いる）。
- `Imply()` は int を返すため誤りを戻り値で表せない。null オブジェクト BDD(-1) を
  被演算子にすると BDDInvalidBDDValueException 例外を投げる。
- `Print()` は null オブジェクトに対して `[ null (error BDD) ]` と印字する。
- BDD_Import() / ZDD_Import() が読むファイルには BDD と ZDD の区別が記録されて
  いない。詳細は [ZDD](classes/ZDD.md) の ZDD_Import の項を参照。

## BDDクラスの使用例

```cpp
int x = BDD_NewVar();
int y = BDD_NewVar();
BDD f1 = BDDvar(x);
BDD f2 = BDDvar(y);
BDD f3 = ~ f1 & f2;
BDD f4 = (~f1 ^ f3) | f2;
f3.Print();
f4.Print();
```
