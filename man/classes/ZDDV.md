# ZDDV  --- ZDDの配列（組合せ集合の配列）を表すクラス

ヘッダーファイル名: "ZDD.h"  
ソースファイル名: ZDD.cc  
内部から呼び出しているクラス: BDD, ZDD

ZDD の配列を表すクラスである。配列長は可変である。要素の番号は 0 か
ら始まる整数である。内部構造は、BDDV クラスと同様に、「出力選択変数」
を導入して、１個のZDDに束ねて配列を表現しているので、もし配列長が
大きくても、各要素が同じ集合であれば、メモリ使用量は１個のZDDと
変わらない。また、配列の複製が配列長に関わらず定数時間で行える。
BDDV_Init()を実行すると、入力変数番号1からBDDV_SysVarTop(通常20）までの
変数が出力選択変数としてシステムに確保され、ユーザが論理演算に用いる
入力変数はその次の番号(通常21)から始まる。出力選択変数はユーザ変数よりも
必ず上位になるように自動的に配置される。

### 関連する定数値

```cpp
extern const int BDDV_SysVarTop
```

出力選択変数の個数。通常20である。ユーザの使用する論理変数の
番号は(BDDV_SysVarTop + 1)番から始まる。

```cpp
extern const int BDDV_MaxLen
```

配列の最大長。BDDV_SysVarTopのべき乗である。

### 関連する外部関数

### operator+

```cpp
ZDDV operator+(const ZDDV& fv, const ZDDV& gv)
```

fvとgvの各要素同士の和集合(union)を表すZDDVオブジェクトを生成し、それを
返す。配列長が一致していなければエラー（異常終了）。記憶あふれの場合は 
長さ1のnullを返す。引数にnullが含まれていた場合には、長さ1のnullを返す。

### operator-

```cpp
ZDDV operator-(const ZDDV& fv, const ZDDV& gv)
```

fvとgvの各要素同士の差集合(difference)を表すZDDVオブジェクトを生成し、それを
返す。配列長が一致していなければエラー（異常終了）。記憶あふれの場合は 
長さ1のnullを返す。引数にnullが含まれていた場合には、長さ1のnullを返す。

### operator*

```cpp
ZDDV operator*(const ZDDV& fv, const ZDDV& gv)
```

fvとgvの各要素同士の積集合(intersection)を表すZDDVオブジェクトを生成し、それを
返す。配列長が一致していなければエラー（異常終了）。記憶あふれの場合は 
長さ1のnullを返す。引数にnullが含まれていた場合には、長さ1のnullを返す。

### operator==

```cpp
int operator==(const ZDDV& fv, const ZDDV& gv)
```

fvとgvの対応する要素が全て同じ組合せ集合かどうかの真偽(1/0)を返す。
配列長が一致していなければエラー（異常終了）。

### operator!=

```cpp
int operator!=(const ZDDV& fv, const ZDDV& gv)
```

fvとgvの対応する要素の少なくとも1組が異なる組合せ集合かどうかの
真偽(1/0)を返す。配列長が一致していなければエラー（異常終了）。

### operator||

```cpp
ZDDV operator||(const ZDDV& fv, const ZDDV& gv)
```

fvの末尾にgvを連結したZDDVオブジェクトを生成し、それを返す。
fv, gv は変化しない。fv の配列長が２のべき乗数のとき、処理効率がよい。
記憶あふれの場合は 長さ1のnullを返す。引数にnullが含まれていた場合には、
長さ1のnullを返す。

### 公開クラスメソッド

### ZDDV

```cpp
ZDDV::ZDDV(void)
```

基本constructer。配列長 0 のZDDVオブジェクトを生成する。

### ZDDV

```cpp
ZDDV::ZDDV(const ZDDV& fv)
```

引数 fv を複製する constructer。

### ZDDV

```cpp
ZDDV::ZDDV(const ZDD& f, int len = 1)
```

引数 fで指定したZDDがlen個続けて並んでいるZDDVオブジェクトを
生成するconstructer。f に null を与えた場合は、len指定に関わらず
長さ1となる。

### ~ZDDV

```cpp
ZDDV::~ZDDV(void)
```

destructer。

### operator=

```cpp
ZDDV& ZDDV::operator=(const ZDDV& fv)
```

自分自身の元のデータを消去し、fv を代入する。関数の値としてfvを返す。

### operator+=

```cpp
ZDDV ZDDV::operator+=(const ZDDV& fv)
```

自分自身と fv の各要素同士の和集合(union)を求め、自分自身に代入する。配列長は一致
していなければならない。記憶あふれの場合は 長さ1のnullを返す。自分自身
または引数にnullが含まれていた場合には、長さ1のnullを返す。

### operator-=

```cpp
ZDDV ZDDV::operator-=(const ZDDV& fv)
```

自分自身と fv の各要素同士の差集合(difference)を求め、自分自身に代入する。配列長は
一致していなければならない。記憶あふれの場合は 長さ1のnullを返す。自分自身
または引数にnullが含まれていた場合には、長さ1のnullを返す。

### operator*=

```cpp
ZDDV ZDDV::operator*=(const ZDDV& fv)
```

自分自身と fv の各要素同士の積集合(intersection)を求め、自分自身に代入する。配列長は
一致していなければならない。記憶あふれの場合は 長さ1のnullを返す。自分自身
または引数にnullが含まれていた場合には、長さ1のnullを返す。

### Change

```cpp
ZDDV ZDDV::Change(int v) const 
```

自分自身の各要素に対して、Change操作（指定したアイテムの有無を反転）を適用
した結果を表すZDDVオブジェクトを生成し、それを返す。記憶あふれの場合は
null を表すオブジェクトを返す。自分自身が null のときは、nullを返す。

### Swap

```cpp
ZDDV ZDDV::Swap(int v1, int v2) const 
```

自分自身の各要素に対して、Swap操作（指定した２つのアイテムを交換）を適用
した結果を表すZDDVオブジェクトを生成し、それを返す。記憶あふれの場合は
null を表すオブジェクトを返す。自分自身が null のときは、nullを返す。

### Restrict

```cpp
ZDDV ZDDV::Restrict(const ZDD& g) const 
```

自分自身の全要素に対して、g が表す集合に含まれる集合を抽出した結果を表す
ZDDVオブジェクトを生成し、それを返す。記憶あふれの場合はnull を表す
オブジェクトを返す。自分自身が null のとき、または g が nullのときは、nullを返す。

### Permit

```cpp
ZDDV ZDDV::Permit(const ZDD& g) const 
```

自分自身の全要素に対して、g が表す集合に包含される集合を抽出した結果を表す
ZDDVオブジェクトを生成し、それを返す。記憶あふれの場合はnull を表す
オブジェクトを返す。自分自身が null のとき、または g が nullのときは、nullを返す。

### PermitSym

```cpp
ZDDV ZDDV::PermitSym(const ZDD& g) const 
```

自分自身の全要素に対して、g が表すいずれかの集合に包含される集合を抽出した結果を表す
ZDDVオブジェクトを生成し、それを返す。記憶あふれの場合はnull を表す
オブジェクトを返す。自分自身が null のとき、または g が nullのときは、nullを返す。

### Top

```cpp
int ZDDV::Top(void) const 
```

自分自身が含むZDDに含まれる最上位の変数番号を返す。nullのときは、0を返す。

### GetZDD

```cpp
ZDD ZDDV::GetZDD(int ix) const 
```

自分自身のix番目の要素のZDDを返す。ixが0以上配列長未満でなければ
エラー（異常終了）。

### Last

```cpp
int ZDDV::Last(void) const 
```

自分自身の配列長を返す。正常な配列長は 0 以上の値である。

### Size

```cpp
bddword ZDDV::Size(void) const 
```

自分自身の総節点数を返す。nullのときは、0を返す。

### Print

```cpp
void ZDDV::Print(void) const 
```

ZDDVの配列長、およびZDDクラスの Print() を全配列要素に対して実行する。

---

