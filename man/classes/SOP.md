# SOP  --- 正負のリテラルからなる積和形論理式を表現するクラス

ヘッダーファイル名: "SOP.h"  
ソースファイル名: SOP.cc  
内部から呼び出しているクラス: ZBDD

ZBDD 表現を利用して、正と負のリテラルからなる積和形論理式を表現する
クラスである。積項（リテラルの積）の集合として表現し、各リテラルは
正または負の値を持っている。通常、リテラルは入力変数の肯定形または
否定形を表す。リテラルの番号（VarID）はBDD_NewVar() で確保される。
正リテラルは偶数番号（2*i）、負リテラルは奇数番号（2*i+1）で表される。
記憶あふれの場合は null(-1) を返す。

（使用例）
```cpp
int v1 = SOP_NewVar();
int v2 = SOP_NewVar();
int v3 = SOP_NewVar();
SOP f1 = SOP(v1 * 2) + SOP(v3 * 2 + 1);
SOP f2 = SOP(v1 * 2 + 1) * SOP(v2 * 2);
SOP f3 = f1 + f2;
f1.Print();
f2.Print();
f3.Print();
```

### 関連する外部関数

```cpp
int SOP_NewVar(void)
```

新しい論理変数をひとつ確保し、その変数番号を返す。実際には
内部で BDD_NewVar() を２回実行して、正負２つのリテラルに対応する
両方のVarIDを確保する。SOP_NewVar() の返り値は正リテラルのVarIDを
2で割った値が得られる。正リテラルのVarID は偶数であり、
対応する負リテラルのVarID はそれに1を加えた奇数である。
変数の個数が最大値を超えるとエラーを出力して異常終了する。

```cpp
SOP operator+(const SOP& f, const SOP& g)
```

f と g の論理和（集合の和）を表す SOP オブジェクトを生成して、
それを返す。記憶あふれの場合は、null を表すオブジェクトを返す。
引数にnullを与えた場合にはnullを返す。

```cpp
SOP operator*(const SOP& f, const SOP& g)
```

f と g の論理和（積項の任意の選び方による積の和集合）を表す 
SOPオブジェクトを生成して、それを返す。記憶あふれの場合は、
null を表すオブジェクトを返す。引数にnullを与えた場合にはnullを返す。

```cpp
SOP operator-(const SOP& f, const SOP& g)
```

差集合 f - g を表す SOP オブジェクトを生成して、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた場合に
はnullを返す。

```cpp
int operator==(const SOP& f, const SOP& g)
```

f と g が等しいかどうかの真偽値（1 または 0）を返す。

```cpp
int operator!=(const SOP& f, const SOP& g)
```

f と g が異なるかどうかの真偽値（1 または 0）を返す。

### 公開クラスメソッド

```cpp
SOP::SOP(void)
```

基本constructer。初期値として、空の論理式（空集合）を表す SOP オブジェクトを
生成する。空集合で表される論理式は恒偽（常に0）である。

```cpp
SOP::SOP(const SOP& f)
```

引数 f を複製する constructer。

```cpp
SOP::SOP(int ltrl)
```

リテラル単体の論理式を表す SOP オブジェクトを生成する。
ltrl は BDD_NewVar() で確保されたリテラルのVarIDでなければならない。

```cpp
SOP::SOP(const ZBDD& zbdd)
```

ZBDD で表現された集合を、リテラルの肯定形のみの積項集合の
論理式として表す SOP オブジェクトを生成する。ZBDD の要素
アイテム番号はSOPのリテラルの正リテラルのVarIDに等しく、
（すなわち、SOP_NewVar() の戻り値の２倍である）これは偶数
である。同じZBDD（ただし偶数アイテムのもの）でもSOP
として解釈するのと、CtoI などとして解釈するのとでは異なる
意味を持つことに注意せよ。

```cpp
SOP::~SOP(void)
```

destructer。

```cpp
SOP& SOP::operator=(const SOP& f)
```

自分自身に f を代入し、自分自身への参照を返す。

```cpp
SOP& SOP::operator+=(const SOP& f)
```

自分自身と f との論理和を求め、自分自身に代入する。自分自身への参照を返す。
記憶あふれの場合はnullを代入する。自分自身や引数がnullのときにはnullとなる。

```cpp
SOP& SOP::operator*=(const SOP& f)
```

自分自身と f との論理積を求め、自分自身に代入する。自分自身への参照を返す。
記憶あふれの場合はnullを代入する。自分自身や引数がnullのときにはnullとなる。

```cpp
SOP& SOP::operator-=(const SOP& f)
```

自分自身から f を除いた集合を求め、自分自身に代入する。自分自身への参照を返す。
記憶あふれの場合はnullを代入する。自分自身や引数がnullのときにはnullとなる。

```cpp
SOP SOP::And(int lt) const
```

自分自身の論理式に対して、リテラル lt との論理積を計算した結果を表す 
SOP オブジェクトを生成して、それを返す。記憶あふれの場合は、
null を表すオブジェクトを返す。自分自身がnullのときは、nullを返す。

```cpp
SOP SOP::Or(int lt) const
```

自分自身の論理式に対して、リテラル lt との論理和を計算した結果を表す 
SOP オブジェクトを生成して、それを返す。記憶あふれの場合は、
null を表すオブジェクトを返す。自分自身がnullのときは、nullを返す。

```cpp
SOP SOP::Xor(int lt) const
```

自分自身の論理式に対して、リテラル lt との排他的論理和を計算した結果を表す 
SOP オブジェクトを生成して、それを返す。記憶あふれの場合は、
null を表すオブジェクトを返す。自分自身がnullのときは、nullを返す。

```cpp
SOP SOP::Compose(const SOP& g, int v) const
```

論理式の代入。自分自身の論理式の変数vの正リテラルおよび負リテラルにgを代入
した論理式を表す SOP オブジェクトを生成して、それを返す。
gの全ての変数は、自分自身の論理式に含まれている変数番号より大きくなければならない。
記憶あふれの場合は、null を表すオブジェクトを返す。
自分自身や引数がnullのときにはnullを返す。

```cpp
SOP SOP::Limit(int n) const
```

積項サイズがn以下の積項のみを取り出した論理式を表す SOP オブジェクト
を生成して、それを返す。記憶あふれの場合は、
null を表すオブジェクトを返す。自分自身がnullのときは、nullを返す。

```cpp
SOP SOP::Permit(const SOP& g) const
```

自分自身が表す論理式の和項の項集合の中から、gの各和項をすべて含むような項
だけを選び出した結果を表す SOP オブジェクトを生成して、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身がnullのとき
またはgがnullのときは、nullを返す。

```cpp
SOP SOP::Permit0(const SOP& g) const
```

自分自身が表す論理式の和項の項集合の中から、gの正リテラルをすべて含み、
負リテラルをいずれも含まないような項だけを選び出した結果を表す SOP 
オブジェクトを生成して、それを返す。記憶あふれの場合は、null を表す
オブジェクトを返す。自分自身がnullのときまたはgがnullのときは、nullを返す。

```cpp
SOP SOP::Swap(int v1, int v2) const
```

自分自身が表す論理式に対して、変数番号v1とv2のリテラルを入れ換えた
論理式を表すSOPオブジェクトを生成し、それを返す。
v1, v2は正リテラルの番号（偶数）を指定するだけで、負リテラルも同時に
入れ替えが実行される。記憶あふれの場合は、nullを表すオブジェクトを返す。
自分自身がnullのときは、nullを返す。

```cpp
ZBDD SOP::GetZBDD(void) const
```

内部表現の ZBDD を複製したオブジェクトを生成して、それを返す。

```cpp
int SOP::TopVar(void) const
```

自分自身の論理式に含まれるリテラルの中で最小の変数番号（すなわち、
最小のVarIDを2で割った値）を返す。自分自身が定数論理式のとき、
及びnullの場合は0を返す。

```cpp
int SOP::TopLit(void) const
```

自分自身の論理式に含まれるリテラルの中で最小のVarIDを返す。
自分自身が定数論理式のとき、及びnullの場合は0を返す。

```cpp
bddword SOP::Size(void) const
```

自分自身の論理式に含まれる積項の個数を返す。
nullに対しては 0 を返す。

```cpp
bddword SOP::LitNum(void) const
```

自分自身の論理式に含まれるリテラルの総個数を返す。
（各積項に含まれるリテラルの個数を合計したもの）
nullに対しては 0 を返す。

```cpp
void SOP::Print(void) const
```

自分自身の論理式を積和形(SOP; Sum of Products)で標準出力に出力する。

```cpp
void SOP::PrintStats(void) const
```

自分自身のカーディナリティ（用いている積項数）や積項に含まれるリテラルの総数、
グラフ節点数などの情報を標準出力に出力する。

---

