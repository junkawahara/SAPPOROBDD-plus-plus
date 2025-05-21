# SeqBDD  --- 系列集合を表現するクラス

ヘッダーファイル名: "SeqBDD.h"  
ソースファイル名: SeqBDD.cc  
内部から呼び出しているクラス: ZBDD

ZBDD表現を用いて、要素に重複を許す順列集合（つまり系列集合）を
表現するクラスである。例えば、系列 (a,a,b,c,a) という要素を含む系列
集合を表現することができる。系列集合はZBDDの元として表現することができ
る。１つの系列において、最大 n 個のアイテムの組合せを用いるとき、
ZBDDで必要なアイテム数は、(元のアイテム数) * n である。例えば、
最大長３までの系列の場合、各アイテムごと１番目の出現、２番目の出現、
３番目の出現の３通りに分けて、３倍のアイテム数を用意しなければならない。
これらのアイテムの集合（の組合せ）としてZBDDの元を表現できる。

### 関連する外部関数

### operator+

```cpp
SeqBDD operator+(const SeqBDD& f, const SeqBDD& g)
```

f と g の和集合を表す SeqBDD オブジェクトを生成して、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。
引数にnullを与えた場合にはnullを返す。

### operator*

```cpp
SeqBDD operator*(const SeqBDD& f, const SeqBDD& g)
```

f と g の積集合を表す SeqBDD オブジェクトを生成して、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。
引数にnullを与えた場合にはnullを返す。

### operator-

```cpp
SeqBDD operator-(const SeqBDD& f, const SeqBDD& g)
```

差集合 f - g を表す SeqBDD オブジェクトを生成して、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。
引数にnullを与えた場合にはnullを返す。

### operator==

```cpp
int operator==(const SeqBDD& f, const SeqBDD& g)
```

f と g が等しいかどうかの真偽値（1 または 0）を返す。

### operator!=

```cpp
int operator!=(const SeqBDD& f, const SeqBDD& g)
```

f と g が異なるかどうかの真偽値（1 または 0）を返す。

### 公開クラスメソッド

### SeqBDD

```cpp
SeqBDD::SeqBDD(void)
```

基本constructer。初期値として、空の系列集合を表す SeqBDD オブジェクトを
生成する。

### SeqBDD

```cpp
SeqBDD::SeqBDD(const SeqBDD& f)
```

引数 f を複製する constructer。

### SeqBDD

```cpp
SeqBDD::SeqBDD(const ZBDD& zbdd, int n, int m)
```

ZBDD で表現された集合を、n 個のアイテムの系列の集合（最大長 m）として表す 
SeqBDD オブジェクトを生成する。ZBDD の要素アイテム番号は、1 から
n*m までである。

### ~SeqBDD

```cpp
SeqBDD::~SeqBDD(void)
```

destructer。

### operator=

```cpp
SeqBDD& SeqBDD::operator=(const SeqBDD& f)
```

自分自身に f を代入し、自分自身への参照を返す。

### operator+=

```cpp
SeqBDD& SeqBDD::operator+=(const SeqBDD& f)
```

自分自身と f との論理和を求め、自分自身に代入する。自分自身への参照を返す。
記憶あふれの場合はnullを代入する。自分自身や引数がnullのときにはnullとなる。

### operator*=

```cpp
SeqBDD& SeqBDD::operator*=(const SeqBDD& f)
```

自分自身と f との論理積を求め、自分自身に代入する。自分自身への参照を返す。
記憶あふれの場合はnullを代入する。自分自身や引数がnullのときにはnullとなる。

### operator-=

```cpp
SeqBDD& SeqBDD::operator-=(const SeqBDD& f)
```

自分自身から f を除いた集合を求め、自分自身に代入する。自分自身への参照を返す。
記憶あふれの場合はnullを代入する。自分自身や引数がnullのときにはnullとなる。

### GetZBDD

```cpp
ZBDD SeqBDD::GetZBDD(void) const
```

内部表現の ZBDD を複製したオブジェクトを生成して、それを返す。

### GetN

```cpp
int SeqBDD::GetN(void) const
```

系列集合の各系列に含まれるアイテムの種類数を返す。
SeqBDD の生成時に与えられた n の値である。

### GetM

```cpp
int SeqBDD::GetM(void) const
```

系列集合に含みうる系列の最大長を返す。
SeqBDD の生成時に与えられた m の値である。

### Size

```cpp
bddword SeqBDD::Size(void) const
```

系列集合の中の系列の個数を返す。系列の個数が大きすぎて bddword 型
で表現できない場合は bddnull を返す。

---

