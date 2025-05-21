# PiDD  --- 順列集合を表現するクラス

ヘッダーファイル名: "PiDD.h"  
ソースファイル名: PiDD.cc  
内部から呼び出しているクラス: ZDD

ZDD 表現を用いて順列集合を表現するクラスである。要素数 n の順列集合
には最大 n! 個の順列が含まれうる。各順列は n 個のアイテムの並べ方を
表す。このクラスでは、各順列をZDDで表現するにあたり、２つのアイテム
の大小関係を表す番号を用いる。例えば、４つのアイテム a, b, c, d に
ついて、a < b の関係を表すアイテムを１番、a < c の関係を表すアイテム
を２番、a < d の関係を表すアイテムを３番、b < c の関係を表すアイテム
を４番、b < d の関係を表すアイテムを５番、c < d の関係を表すアイテム
を６番、という具合に番号を付ける。この場合、順列 (a,c,b,d) は集合
{2,5,6} で表される。なぜなら、a < c, b < d, c < d という関係を表す
アイテム番号の集合だからである。また、順列 (d,c,b,a) は、集合
{} （空集合）で表される。なぜなら、この順列にはどの大小関係もないからである。

### 関連する外部関数

### operator+

```cpp
PiDD operator+(const PiDD& f, const PiDD& g)
```

f と g の和集合を表す PiDD オブジェクトを生成して、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。
引数にnullを与えた場合にはnullを返す。

### operator*

```cpp
PiDD operator*(const PiDD& f, const PiDD& g)
```

f と g の積集合を表す PiDD オブジェクトを生成して、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。
引数にnullを与えた場合にはnullを返す。

### operator-

```cpp
PiDD operator-(const PiDD& f, const PiDD& g)
```

差集合 f - g を表す PiDD オブジェクトを生成して、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。
引数にnullを与えた場合にはnullを返す。

### operator==

```cpp
int operator==(const PiDD& f, const PiDD& g)
```

f と g が等しいかどうかの真偽値（1 または 0）を返す。

### operator!=

```cpp
int operator!=(const PiDD& f, const PiDD& g)
```

f と g が異なるかどうかの真偽値（1 または 0）を返す。

### 公開クラスメソッド

### PiDD

```cpp
PiDD::PiDD(void)
```

基本constructer。初期値として、空の順列集合を表す PiDD オブジェクトを
生成する。

### PiDD

```cpp
PiDD::PiDD(const PiDD& f)
```

引数 f を複製する constructer。

### PiDD

```cpp
PiDD::PiDD(const ZDD& zdd, int n)
```

ZDD で表現された集合を、n 個のアイテムの順列集合として表す 
PiDD オブジェクトを生成する。ZDD の要素 アイテム番号は
順列の大小関係を表す番号である。

### ~PiDD

```cpp
PiDD::~PiDD(void)
```

destructer。

### operator=

```cpp
PiDD& PiDD::operator=(const PiDD& f)
```

自分自身に f を代入し、自分自身への参照を返す。

### operator+=

```cpp
PiDD& PiDD::operator+=(const PiDD& f)
```

自分自身と f との論理和を求め、自分自身に代入する。自分自身への参照を返す。
記憶あふれの場合はnullを代入する。自分自身や引数がnullのときにはnullとなる。

### operator*=

```cpp
PiDD& PiDD::operator*=(const PiDD& f)
```

自分自身と f との論理積を求め、自分自身に代入する。自分自身への参照を返す。
記憶あふれの場合はnullを代入する。自分自身や引数がnullのときにはnullとなる。

### operator-=

```cpp
PiDD& PiDD::operator-=(const PiDD& f)
```

自分自身から f を除いた集合を求め、自分自身に代入する。自分自身への参照を返す。
記憶あふれの場合はnullを代入する。自分自身や引数がnullのときにはnullとなる。

### Swap

```cpp
PiDD PiDD::Swap(int i, int j) const
```

順列集合の中の全順列に対してアイテム i と j を入れ換えた集合を
表す PiDD オブジェクトを生成して、それを返す。記憶あふれの場合は、
null を表すオブジェクトを返す。自分自身がnullのときは、nullを返す。

### GetZDD

```cpp
ZDD PiDD::GetZDD(void) const
```

内部表現の ZDD を複製したオブジェクトを生成して、それを返す。

### GetN

```cpp
int PiDD::GetN(void) const
```

順列集合の各順列に含まれるアイテムの個数を返す。
PiDD の生成時に与えられた値である。

### Size

```cpp
bddword PiDD::Size(void) const
```

順列集合の中の順列の個数を返す。順列の個数が大きすぎて bddword 型
で表現できない場合は bddnull を返す。

---

