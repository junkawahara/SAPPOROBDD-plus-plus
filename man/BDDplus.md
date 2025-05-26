# SAPPOROBDD++ マニュアル

## パッケージの概要

- このパッケージはBDD/ZDDの基本操作を行うC++のクラスライブラリである。
  本プログラムは、32ビットまたは64ビットの計算機で動作する。
  （コンパイル時に、オプションB_32を指定すると32ビットモードとなり、指定しないと64ビットモードになる。この動作はオリジナルの SAPPOROBDD とは異なることに注意。）
  各操作はC++のメソッド呼び出しにより実行される。

- 入力変数番号(通称VarID)は1から始まるint型の整数で識別する（0は定数を表す）。
  負の変数番号は用いない。VarIDの最大値は定数BDD_MaxVarで与えられる。
  デフォルトは65535(16ビット)。

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
  BDD_Initを省略した場合の default は、初期値 256、最大値 1,024に
  設定されている。計算中に記憶あふれを起こした場合は、計算を中断して、
  nullオブジェクトBDD(-1) を返す。

## SAPPOROBDD++ 独自の機能

- SAPPOROBDD++ では64ビットバージョンがデフォルトになったため、B_64 マクロの定義は必要なくなった。32ビットバージョンでコンパイルする場合は B_32 マクロを定義する。
- SAPPOROBDD++ ではすべてのパッケージは sapporobdd 名前空間に入れられている。`using namespace sapporobdd` を使用前に書く必要がある。
- SAPPOROBDD++ では ZBDD は ZDD にリネームされている。ZBDD を使い続けることも可能である。ZBDD_Meet 等の関数名も ZDD_Meet にリネームされている。
- エラーが発生した際は、BDDException 例外が投げられる。例外クラスは BDDException.h で定義されている。以下の例外（いずれも BDDException の子クラス）が存在する。
  - BDDInvalidBDDValueException
  - BDDOutOfRangeException
  - BDDOutOfMemoryException
  - BDDFileFormatException
  - BDDInternalErrorException

## 提供するクラスとその依存関係

- [BDD](classes/BDD.md) - BDDで表現された個々の論理関数を指すクラス
  - [BDDV](classes/BDDV.md) - BDDの配列（論理関数の配列）を表すクラス
    - [BtoI](classes/BtoI.md) - ２値入力整数値出力の論理関数を表すクラス
  - [BDDDG](classes/BDDDG.md) - BDDを単純直交分解した結果を表すクラス
  - [ZDD](classes/ZDD.md) - ゼロサプレス型BDDで表現された組合せ集合を指すクラス
    - [ZDDV](classes/ZDDV.md) - ZDDの配列（組合せ集合の配列）を表すクラス
      - [CtoI](classes/CtoI.md) - 整数値組合せ集合（整係数ユネイト論理式）を表すクラス
    - [SOP](classes/SOP.md) - 正負のリテラルからなる積和形論理式を表現するクラス
      - [SOPV](classes/SOPV.md) - SOPの配列（積和形論理式の配列）を表すクラス
    - [PiDD](classes/PiDD.md) - 順列集合を表現するクラス
    - [SeqBDD](classes/SeqBDD.md) - 系列集合を表現するクラス
    - [GBase](classes/GBase.md) - ZDDでパス/サイクル列挙を行うためのクラス
    - [BDDCT](classes/BDDCT.md) - BDD/ZDDでコスト制約付き変数を扱うためのクラス

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
