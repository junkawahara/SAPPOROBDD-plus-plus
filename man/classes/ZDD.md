# ZDD  --- ゼロサプレス型BDDで表現された組合せ集合を指すクラス

ヘッダーファイル名: "ZDD.h"  
ソースファイル名: ZDD.cc  
内部から呼び出しているクラス: BDD

ゼロサプレス型BDD 表現を用いて、組合せ集合を抽象化したクラスである。
集合の各要素は、n個のアイテムの中からk個を選ぶ組合せを表す。アイテムは
1から始まる整数で識別する。0 個の要素からなる集合を空集合と呼ぶ。また
n個のリテラルから0個を選ぶ組合せを表す要素を単位元要素と呼び、単位元
要素1個だけからなる集合を単位元集合と呼ぶ。記憶あふれの場合は処理を中断し、
null (-1) を返す。

（使用例）
```cpp
int v1 = BDD_NewVar();
int v2 = BDD_NewVar();
int v3 = BDD_NewVar();
int v4 = BDD_NewVar();
ZDD f1 = ZDD(v1) + ZDD(v2);
ZDD f2 = ZDD(v3) * ZDD(v4);
ZDD f3 = f1 * f2;
ZDD f4 = f3.Change(v1);
f3.Print();
f4.Print();
```

### 関連する外部関数

### operator*

```cpp
ZDD operator*(const ZDD& f, const ZDD& g)
```

積集合（intersection）を表すZDDオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。

### operator&

```cpp
ZDD operator&(const ZDD& f, const ZDD& g)
```

積集合（intersection）を表すZDDオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。演算子 operator* と同じ操作である。

### operator+

```cpp
ZDD operator+(const ZDD& f, const ZDD& g)
```

和集合（union）を表すZDDオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。

### operator|

```cpp
ZDD operator|(const ZDD& f, const ZDD& g)
```

和集合（union）を表すZDDオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。演算子 operator+ と同じ操作である。

### operator-

```cpp
ZDD operator-(const ZDD& f, const ZDD& g)
```

差集合を表すZDDオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。

### operator==

```cpp
int operator==(const ZDD& f, const ZDD& g)
```

f と g が同じ集合かどうかの真偽(1/0)を返す。

### operator!=

```cpp
int operator!=(const ZDD& f, const ZDD& g)
```

f と g が異なる集合かどうかの真偽(1/0)を返す。

### 公開クラスメソッド

### ZDD

```cpp
ZDD::ZDD(void)
```

基本constructer。初期値として空集合を表すZDDオブジェクトを生成する。

### ZDD

```cpp
ZDD::ZDD(int v)
```

定数式を作り出す constructer。v == 0 ならば空集合、v > 0 ならば単位元集合、
v < 0 ならば null を表すZDDオブジェクトを生成する。

### ZDD

```cpp
ZDD::ZDD(const ZDD& f)
```

引数 f を複製する constructer。

### ZDD

```cpp
ZDD::ZDD(const BDD& bdd, int offset)
```

BDDオブジェクトから変換する constructer。各変数をoffset だけシフトして
ZDD変数に割り当てる。offsetに0を指定すると、0シフト（元の変数番号のまま）
となる。記憶あふれの場合は null オブジェクトを生成する。
例外として、変数の値が0のBDDの場合は空集合のZDDとなる。

### ~ZDD

```cpp
ZDD::~ZDD(void)
```

destructer。内部のBDD節点の記憶管理は自動化されており、使用済みの節点は適当なタイミングで回収され、再利用される。

### operator=

```cpp
ZDD& ZDD::operator=(const ZDD& f)
```

自分自身に f を代入し、f を関数値として返す。

### operator+=

```cpp
ZDD ZDD::operator+=(const ZDD& f)
```

自分自身と f との和集合を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは何もしない。f が null のときは、null を代入する。

### operator-=

```cpp
ZDD ZDD::operator-=(const ZDD& f)
```

自分自身から f を除いた差集合を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは何もしない。f が null のときは、null を代入する。

### operator*=

```cpp
ZDD ZDD::operator*=(const ZDD& f)
```

自分自身と f との積集合を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは何もしない。f が null のときは、null を代入する。

### Change

```cpp
ZDD ZDD::Change(int v) const 
```

自分自身が表す組合せ集合から、変数番号 v の要素の 0/1 を反転
（含む→含まない、含まない→含む）させた集合を表すZDDオブジェクトを
生成して、それを返す。v が含まれない項と含まれる項が同時に含まれて
いると、Change(v) 操作を実行すると節点数が指数的に増加する可能性
があるので注意すべきである。記憶あふれの場合は、null を表すオブジェクトを
返す。自分自身が null のときは、nullを返す。

### Swap

```cpp
ZDD ZDD::Swap(int v1, int v2) const 
```

自分自身が表す組合せ集合から、変数番号 v1 と変数番号 v2 を
入れ替えた集合を表すZDDオブジェクトを生成して、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。
自分自身が null のときは、nullを返す。

### SymmetricDifference

```cpp
ZDD ZDD::SymmetricDifference(const ZDD& g) const 
```

自分自身が表す組合せ集合と g が表す組合せ集合の、対称差
（ f ⊖ g = (f - g) ⋃ (g - f) ）を表すZDDオブジェクトを
生成して、それを返す。記憶あふれの場合は、null を表す
オブジェクトを返す。自分自身が null のとき、または g が null
のときは、null を返す。

### Support

```cpp
BDD ZDD::Support(void) const 
```

自分自身の集合要素が用いているアイテム変数を集め、その変数の
項の論理和を表すBDDオブジェクトを生成し、それを返す。記憶あふれの場合は、
nullを表すオブジェクトを返す。自分自身がnullのときは、nullを返す。

### Restrict

```cpp
ZDD ZDD::Restrict(const ZDD& g) const 
```

自分自身が表す組合せ集合から、g が表す集合に含まれる集合を
抽出した集合を表すZDDオブジェクトを生成して、それを返す。
集合の包含関係についての filter を適用したことになる。この演算は
1の元が < の関係にある場合のみ高速に計算できる。
記憶あふれの場合は、null を表すオブジェクトを返す。
自分自身が null のとき、または g が null のときは、null を返す。

### Permit

```cpp
ZDD ZDD::Permit(const ZDD& g) const 
```

自分自身が表す組合せ集合から、g が表す集合に包含される集合を
抽出した集合を表すZDDオブジェクトを生成して、それを返す。
集合の包含関係についての filter を適用したことになる。この演算は
それほど計算効率がよくないことが多い。
記憶あふれの場合は、null を表すオブジェクトを返す。
自分自身が null のとき、または g が null のときは、null を返す。

### PermitSym

```cpp
ZDD ZDD::PermitSym(const ZDD& g) const 
```

自分自身が表す組合せ集合から、g が表すいずれかの集合に包含される集合を
抽出した集合を表すZDDオブジェクトを生成して、それを返す。
集合の包含関係についての、複数集合に関する symmetrical な
filter を適用したことになる。この演算は特殊ケースについて高速に計算できる。
記憶あふれの場合は、null を表すオブジェクトを返す。
自分自身が null のとき、または g が null のときは、null を返す。

### PermitNot

```cpp
ZDD ZDD::PermitNot(const ZDD& g) const 
```

自分自身が表す組合せ集合から、g が表すいずれの集合にも被包含されない集合を
抽出した集合を表すZDDオブジェクトを生成して、それを返す。
集合の包含関係についての、複数集合に関する filter を
適用したことになる。この演算は多くの場合効率的に計算できない。
記憶あふれの場合は、null を表すオブジェクトを返す。
自分自身が null のとき、または g が null のときは、null を返す。

### Always

```cpp
ZDD ZDD::Always(int v) const 
```

自分自身が表す組合せ集合から、変数番号 v を必ず含む集合を
抽出した部分集合を表すZDDオブジェクトを生成して、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。
自分自身が null のときは、nullを返す。

### Never

```cpp
ZDD ZDD::Never(int v) const 
```

自分自身が表す組合せ集合から、変数番号 v を含まない集合を
抽出した部分集合を表すZDDオブジェクトを生成して、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。
自分自身が null のときは、nullを返す。

### Top

```cpp
int ZDD::Top(void) const 
```

自分自身のグラフに関係する入力変数の中で、最上位の変数番号を返す。
自分自身が定数関数のとき、及びnullの場合は0を返す。

### Card

```cpp
bddword ZDD::Card(void) const 
```

自分自身が表す集合の要素数を返す。
有限の答えが得られないほど大きな要素数の場合やnullの場合は、
bddnull（BDD_MaxNodeよりも大きな数値）を返す。

### Size

```cpp
bddword ZDD::Size(void) const 
```

自分自身のグラフの節点数を返す。nullに対しては0を返す。

### Print

```cpp
void ZDD::Print(void) const 
```

インデックスの値、最上位のリテラル番号、節点数の情報を標準出力に出力する。

### ZLev

```cpp
bddword ZDD::ZLev(int lev, int last = 0) const 
```

最上位の変数のレベルがlev、最下位の変数のレベルがlastである集合の
みを取り出した集合を表すZDDオブジェクトを生成して、それを返す。
レベルの小さな変数を含む集合だけを取り出して、簡単な部分問題を
解くのに用いることができる。
記憶あふれの場合は、null を表すオブジェクトを返す。
自分自身が null のときは、nullを返す。

---

