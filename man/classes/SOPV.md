# SOPV  --- SOPの配列（積和形論理式の配列）を表すクラス

ヘッダーファイル名: "SOP.h"  
ソースファイル名: SOP.cc  
内部から呼び出しているクラス: ZBDD, ZBDDV

SOP の配列を表すクラスである。内部表現は ZBDDV にほぼ等しいが、
意味づけが異なる。

### 関連する外部関数

```cpp
SOPV operator+(const SOPV& fv, const SOPV& gv)
```

fvとgvの各要素同士の論理和を表すSOPVオブジェクトを生成し、それを
返す。配列長が一致していなければエラー（異常終了）。記憶あふれの場合は 
長さ1のnullを返す。引数にnullが含まれていた場合には、長さ1のnullを返す。

```cpp
SOPV operator*(const SOPV& fv, const SOPV& gv)
```

fvとgvの各要素同士の論理積を表すSOPVオブジェクトを生成し、それを
返す。配列長が一致していなければエラー（異常終了）。記憶あふれの場合は 
長さ1のnullを返す。引数にnullが含まれていた場合には、長さ1のnullを返す。

```cpp
SOPV operator-(const SOPV& fv, const SOPV& gv)
```

fvとgvの各要素同士の差集合を表すSOPVオブジェクトを生成し、それを
返す。配列長が一致していなければエラー（異常終了）。記憶あふれの場合は 
長さ1のnullを返す。引数にnullが含まれていた場合には、長さ1のnullを返す。

```cpp
int operator==(const SOPV& fv, const SOPV& gv)
```

fvとgvの対応する要素が全て同じ論理関数かどうかの真偽(1/0)を返す。
配列長が一致していなければエラー（異常終了）。

```cpp
int operator!=(const SOPV& fv, const SOPV& gv)
```

fvとgvの対応する要素の少なくとも1組が異なる論理関数かどうかの
真偽(1/0)を返す。配列長が一致していなければエラー（異常終了）。

```cpp
SOPV operator||(const SOPV& fv, const SOPV& gv)
```

fvの末尾にgvを連結したSOPVオブジェクトを生成し、それを返す。
fv, gv は変化しない。fv の配列長が２のべき乗数のとき、処理効率がよい。
記憶あふれの場合は 長さ1のnullを返す。引数にnullが含まれていた場合には、
長さ1のnullを返す。

### 公開クラスメソッド

```cpp
SOPV::SOPV(void)
```

基本constructer。配列長 0 のSOPVオブジェクトを生成する。

```cpp
SOPV::SOPV(const SOPV& fv)
```

引数 fv を複製する constructer。

```cpp
SOPV::SOPV(const SOP& f, int len = 1)
```

引数 fで指定したSOPがlen個続けて並んでいるSOPVオブジェクトを
生成するconstructer。f に null を与えた場合は、len指定に関わらず
長さ1となる。

```cpp
SOPV::SOPV(const ZBDDV& fv)
```

内部表現 ZBDDVを表す ZBDDVオブジェクトを複製してSOPVとして
生成するconstructer。

```cpp
SOPV::~SOPV(void)
```

destructer。

```cpp
SOPV& SOPV::operator=(const SOPV& fv)
```

自分自身の元のデータを消去し、fv を代入する。関数の値としてfvを返す。

```cpp
SOPV SOPV::operator+=(const SOPV& fv)
```

自分自身と fv の各要素同士の論理和を求め、自分自身に代入する。配列長は
一致していなければならない。記憶あふれの場合は 長さ1のnullを返す。
自分自身または引数にnullが含まれていた場合には、長さ1のnullを返す。

```cpp
SOPV SOPV::operator*=(const SOPV& fv)
```

自分自身と fv の各要素同士の論理積を求め、自分自身に代入する。配列長は
一致していなければならない。記憶あふれの場合は 長さ1のnullを返す。
自分自身または引数にnullが含まれていた場合には、長さ1のnullを返す。

```cpp
SOPV SOPV::operator-=(const SOPV& fv)
```

自分自身と fv の各要素同士の差集合を求め、自分自身に代入する。配列長は
一致していなければならない。記憶あふれの場合は 長さ1のnullを返す。
自分自身または引数にnullが含まれていた場合には、長さ1のnullを返す。

```cpp
SOP SOPV::GetSOP(int ix) const
```

自分自身の第ix番目の配列要素を返す。

```cpp
ZBDDV SOPV::GetZBDDV(void) const 
```

内部表現の ZBDDV を複製し、それを返す。

```cpp
int SOPV::Last(void) const 
```

自分自身の（意味のある）配列要素の中の、最大の要素番号を返す。

```cpp
SOPV SOPV::Swap(int, int) const 
```

自分自身の各配列要素に対して、変数番号var1とvar2のリテラルを入れ換えた
ときの積項集合の配列を表すSOPVオブジェクトを生成し、それを返す。
var1, var2は正リテラルの番号（偶数）を指定するだけで、負リテラルも同時に
入れ替えが実行される。引数はlevelではなく、変数番号で与えることに注意。
記憶あふれの場合は、nullを表すオブジェクトを返す。自分自身がnullのときは、
nullを返す。

---

