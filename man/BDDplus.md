# C++版BDDパッケージ(SAPPORO-1.96)　ドキュメント

著者: 湊 真一　京都大学　大学院情報学研究科  
最新更新日: 2022.10.15

---

## パッケージの概要

- このパッケージはBDDの基本操作を行うC++のクラスライブラリである。
  本プログラムは、32ビットまたは64ビットの計算機で動作する。
  （コンパイル時に、オプションB_64を指定すると64ビットモードとなる）
  各操作はC++のメソッド呼び出しにより実行される。

- 入力変数番号(通称VarID)は1から始まるint型の整数で識別する（0は定数を表す）。
  負の変数番号は用いない。VarIDの最大値は定数BDD_MaxVarで与えられる。
  デフォルトは65535(16ビット)。

- 各VarIDごとにBDDでの上下の順位(通称level)の情報を保持している。
  levelもまた1から始まるint型の整数で識別する。大きい数値ほど
  上位の変数を表す（BDDの根に近く、先に展開される）。
  VarIDを何も指定せずに生成した場合はVarIDと同じ値のlevelを持つ。

- 論理演算結果のBDDは、32ビット（または64ビット）のunsigned int
 （bddwordという名前の型にtypedefされている）のインデックスで返される。
  BDDは論理関数に対して一意であり、インデックスの値もBDDに対して
  一意である。したがって、２つの論理演算結果が等価であるかどうかは、
  演算結果のインデックスの値が同じかどうかを比較することで行える。

- BDD節点テーブルの最大サイズは、BDD_Init() の２つの引数で指定する。
  BDD_Initを省略した場合の default は、初期値 256、最大値 1,024に
  設定されている。計算中に記憶あふれを起こした場合は、計算を中断して、
  nullオブジェクトBDD(-1) を返す。

- 提供するクラスとその依存関係:
  - [BDD](classes/BDD.md) - BDDで表現された個々の論理関数を指すクラス
    - [BDDV](classes/BDDV.md) - BDDの配列（論理関数の配列）を表すクラス
      - [BtoI](classes/BtoI.md) - ２値入力整数値出力の論理関数を表すクラス
    - [BDDDG](classes/BDDDG.md) - BDDを単純直交分解した結果を表すクラス
    - [ZBDD](classes/ZBDD.md) - ゼロサプレス型BDDで表現された組合せ集合を指すクラス
      - [ZBDDV](classes/ZBDDV.md) - ZBDDの配列（組合せ集合の配列）を表すクラス
        - [CtoI](classes/CtoI.md) - 整数値組合せ集合（整係数ユネイト論理式）を表すクラス
      - [SOP](classes/SOP.md) - 正負のリテラルからなる積和形論理式を表現するクラス
        - [SOPV](classes/SOPV.md) - SOPの配列（積和形論理式の配列）を表すクラス
      - [PiDD](classes/PiDD.md) - 順列集合を表現するクラス
      - [SeqBDD](classes/SeqBDD.md) - 系列集合を表現するクラス
      - [GBase](classes/GBase.md) - ZBDDでパス/サイクル列挙を行うためのクラス
      - [BDDCT](classes/BDDCT.md) - BDD/ZBDDでコスト制約付き変数を扱うためのクラス

- BDDクラスの使用例
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