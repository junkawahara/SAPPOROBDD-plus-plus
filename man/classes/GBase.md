# GBase  --- ZDDでパス/サイクル列挙を行うためのクラス

ヘッダーファイル名: "GBase.h"  
ソースファイル名: GBase.cc  
内部から呼び出しているクラス: ZDD

ZDD表現を用いて、グラフ上のパスやサイクルなどを列挙操作を行うためのクラスである。
グラフ構造は内部データ構造として持たず、グラフ上での列挙操作だけをこのクラス
で提供する。辺集合の集合をZDDで表現する。各辺は整数のアイテム番号で表される。
頂点は１から始まる整数で表され、２頂点 i, j 間の辺は、(i,j)もしくは(j,i)で示す。
i, j を昇順に正規化し、ZDD のアイテムとしては (i-1)*n + j の数値を使う。
(ただし、n は頂点数)。各辺集合はグラフの部分グラフを表し、その部分グラフ上で
パスやサイクルなどの様々な性質をもった辺集合を計算することができる。

### 関連する外部関数

### GBASIS_HEIGHT

```cpp
int GBASIS_HEIGHT(int level)
```

グラフ上の高さ level を持つ頂点番号を返す。

### GBASIS_VAR

```cpp
int GBASIS_VAR(int h)
```

グラフ上の高さ h に対応する頂点の中間変数番号を返す。

### 公開クラスメソッド

### GBase

```cpp
GBase::GBase(int n)
```

頂点数 n のグラフに関する列挙を行うためのオブジェクトを生成する。
実際のグラフデータを保持するわけではなく、グラフに関する計算のための
情報を管理する。

### ~GBase

```cpp
GBase::~GBase(void)
```

destructer。

### SetNodes

```cpp
void GBase::SetNodes(int nodes)
```

頂点数を改めて設定する。

### GetNodes

```cpp
int GBase::GetNodes(void) const
```

頂点数を返す。

### ITE

```cpp
ZDD GBase::ITE(const BDD& f, const ZDD& g, const ZDD& h) const
```

頂点変数を用いたBDDと２つのZDDを引数として、if-then-else演算を行う。
結果をZDD で返す。

### AllGraphs

```cpp
ZDD GBase::AllGraphs(void) const
```

辺全体（すなわち全グラフ）を表す集合を生成し、ZDD で返す。

### EdgeSingleton

```cpp
ZDD GBase::EdgeSingleton(int i, int j) const
```

グラフの1辺(i,j)のみを含む集合を生成し、ZDD で返す。

### SetEdge

```cpp
ZDD GBase::SetEdge(const ZDD& f, int i, int j) const
```

辺集合の各要素に辺(i,j)を追加したものを計算し、ZDD で返す。
もし、iやjが無効な頂点番号(範囲外)ならば、空集合を返す。

### ClrEdge

```cpp
ZDD GBase::ClrEdge(const ZDD& f, int i, int j) const
```

辺集合の各要素から辺(i,j)を取り除いたものを計算し、ZDD で返す。
もし、iやjが無効な頂点番号(範囲外)ならば、引数を持つ集合をそのまま返す。

### GetEdge

```cpp
ZDD GBase::GetEdge(const ZDD& f, int i, int j) const
```

辺集合の各要素の中で辺(i,j)を持つ要素だけを取り出し、ZDD で返す。

### GetEdge0

```cpp
ZDD GBase::GetEdge0(const ZDD& f, int i, int j) const
```

辺集合の各要素の中で辺(i,j)を持たない要素だけを取り出し、ZDD で返す。

### ConnUnspec1

```cpp
ZDD GBase::ConnUnspec1(const ZDD& f, int a) const
```

辺集合の各要素の中から、頂点aが他の任意の頂点と辺で結ばれているもの
だけを取り出し、ZDD で返す。

### ConnUnspec2

```cpp
ZDD GBase::ConnUnspec2(const ZDD& f, int a) const
```

辺集合の各要素の中から、頂点aが他の任意の頂点と辺で結ばれていないもの
だけを取り出し、ZDD で返す。

### ConnSpec1

```cpp
ZDD GBase::ConnSpec1(const ZDD& f, int a, int b) const
```

辺集合の各要素の中から、頂点aと頂点bが辺で結ばれているものだけを取り出し、
ZDD で返す。

### ConnSpec2

```cpp
ZDD GBase::ConnSpec2(const ZDD& f, int a, int b) const
```

辺集合の各要素の中から、頂点aと頂点bが辺で結ばれていないものだけを取り出し、
ZDD で返す。

### Paths

```cpp
ZDD GBase::Paths(const ZDD& f, int a, int b) const
```

辺集合の各要素を部分グラフとして見た時に、aからbへのパスがあるものだけを
取り出し、ZDD で返す。

### Cycles

```cpp
ZDD GBase::Cycles(const ZDD& f) const
```

辺集合の各要素を部分グラフとして見た時に、１個以上のサイクルを含むもの
だけを取り出し、ZDD で返す。

### TreeBacktrack

```cpp
ZDD GBase::TreeBacktrack(int root, int size) const
```

頂点数sizeの全域木の集合を列挙し、ZDD で返す。root は根となる頂点を表す。
アルゴリズムは分岐限定法（バックトラック法）を用いる。

### Trees

```cpp
ZDD GBase::Trees(int root, int size) const
```

頂点数sizeの全域木の集合を列挙し、ZDD で返す。root は根となる頂点を表す。
アルゴリズムは効率的なフロンティア法を用いる。

---

