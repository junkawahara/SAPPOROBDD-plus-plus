# BtoI  --- ２値入力整数値出力の論理関数を表すクラス

ヘッダーファイル名: "BtoI.h"  
ソースファイル名: BtoI.cc  
内部から呼び出しているクラス: BDD, BDDV

BDDを用いて２値入力整数値出力の論理関数を表すクラス。整数出力を
２進数のビット列として考え、各ビット位置毎に値を表す論理関数
（1のとき、そのビット位置に1が出力され、0のとき、そのビット位置に0が出力）
をBDDで保持する。各ビットごとの関数をBDDVの形式でまとめて保持している。
BtoI オブジェクトの生成にあたって、BDDV_Init()を最初に実行する必要がある。

### 関連する外部関数

```cpp
BtoI operator+(const BtoI& a, const BtoI& b)
```

a と b の加算を表すBtoIオブジェクトを生成し、それを返す。記憶あふれの
場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。

```cpp
BtoI operator-(const BtoI& a, const BtoI& b)
```

a から b を減算した値を表すBtoIオブジェクトを生成し、それを返す。記憶あふれの
場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。

```cpp
BtoI operator*(const BtoI& a, const BtoI& b)
```

a と b の乗算を表すBtoIオブジェクトを生成し、それを返す。記憶あふれの
場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。

```cpp
BtoI operator/(const BtoI& a, const BtoI& b)
```

a を b で除算した値を表すBtoIオブジェクトを生成し、それを返す。記憶あふれの
場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。b が0の場合も nullを返す。

```cpp
BtoI operator&(const BtoI& a, const BtoI& b)
```

a と b のビットごとの論理積を表すBtoIオブジェクトを生成し、それを返す。記憶あふれの
場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。

```cpp
BtoI operator|(const BtoI& a, const BtoI& b)
```

a と b のビットごとの論理和を表すBtoIオブジェクトを生成し、それを返す。記憶あふれの
場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。

```cpp
BtoI operator^(const BtoI& a, const BtoI& b)
```

a と b のビットごとの排他的論理和を表すBtoIオブジェクトを生成し、それを返す。記憶あふれの
場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。

```cpp
BtoI BtoI_ITE(const BDD& f, const BtoI& a, const BtoI& b)
```

f が真のとき a, 偽のとき b を返すような関数を表すBtoIオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。

### 公開クラスメソッド

```cpp
BtoI::BtoI(void)
```

基本constructer。初期値として整数値 0 (恒偽関数)を表す BtoI オブジェクトを生成する。

```cpp
BtoI::BtoI(const BtoI& fv)
```

引数 fv を複製する constructer。

```cpp
BtoI::BtoI(const BDDV& fv)
```

BDDVで表された関数fvを２値入力整数値出力の論理関数として扱うconstructer。
配列要素0が最下位ビット、配列要素n-1が最上位ビットの２進数を表す関数となる。

```cpp
BtoI::BtoI(const BDD& f)
```

BDDで表された関数fを２値入力整数値出力の論理関数として扱う。長さ2（最大1ビット長の整数）の
関数を保持する。fが定数0ならば、常に0を出力する関数となる。fが定数1ならば、常に1を
出力する関数となる。fが定数でなければ、入力によって0または1を出力する関数となる。

```cpp
BtoI::BtoI(int val)
```

整数値valを常に出力する定数関数のBtoIオブジェクトを作るconstructer。
valは正または負の整数でよい。val < 0 の場合は2の補数表現となる。

```cpp
BtoI::~BtoI(void)
```

destructer。

```cpp
BtoI& BtoI::operator=(const BtoI& fv)
```

自分自身に fv を代入し、fvを関数値として返す。

```cpp
BtoI& BtoI::operator+=(const BtoI& fv)
```

自分自身と fv の加算を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
何もしない。fv が null のときは、null を代入する。

```cpp
BtoI& BtoI::operator-=(const BtoI& fv)
```

自分自身から fv を減算した値を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
何もしない。fv が null のときは、null を代入する。

```cpp
BtoI& BtoI::operator*=(const BtoI& fv)
```

自分自身と fv の乗算を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
何もしない。fv が null のときは、null を代入する。

```cpp
BtoI& BtoI::operator/=(const BtoI& fv)
```

自分自身を fv で除算した値を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
何もしない。fv が null または 0 のときは、null を代入する。

```cpp
BtoI& BtoI::operator%=(const BtoI& fv)
```

自分自身を fv で除算した余りを求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
何もしない。fv が null または 0 のときは、null を代入する。

```cpp
BtoI& BtoI::operator&=(const BtoI& fv)
```

自分自身と fv のビットごとの論理積を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
何もしない。fv が null のときは、null を代入する。

```cpp
BtoI& BtoI::operator|=(const BtoI& fv)
```

自分自身と fv のビットごとの論理和を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
何もしない。fv が null のときは、null を代入する。

```cpp
BtoI& BtoI::operator^=(const BtoI& fv)
```

自分自身と fv のビットごとの排他的論理和を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
何もしない。fv が null のときは、null を代入する。

```cpp
BtoI& BtoI::operator<<=(const BtoI& fv)
```

自分自身を fv ビットだけ左シフトした値を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
何もしない。fv が null のときは、null を代入する。

```cpp
BtoI& BtoI::operator>>=(const BtoI& fv)
```

自分自身を fv ビットだけ右シフトした値を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
何もしない。fv が null のときは、null を代入する。

```cpp
BtoI BtoI::operator-(void) const
```

自分自身の符号を反転した関数を表す BtoIオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
nullを返す。

```cpp
BtoI BtoI::operator~(void) const
```

自分自身のビットごとの否定を表す BtoIオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
nullを返す。

```cpp
BtoI BtoI::operator!(void) const
```

自分自身の0/非0を反転する BtoIオブジェクトを生成し、それを返す。
すなわち、元の関数が0を返すときに1、それ以外はすべて0を返す関数を作る。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
nullを返す。

```cpp
BtoI BtoI::operator<<(const BtoI& fv) const
```

自分自身を fv ビットだけ左シフトした値を表すBtoIオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null または
fv がnullのときは nullを返す。

```cpp
BtoI BtoI::operator>>(const BtoI& fv) const
```

自分自身を fv ビットだけ右シフトした値を表すBtoIオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null または
fv がnullのときは nullを返す。

```cpp
BtoI BtoI::UpperBound(void) const
```

自分自身が取りうる最大値を計算するために、すべての入力変数が
上限のときに最大となる関数を表すBtoIオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が nullのときは
nullを返す。

```cpp
BtoI BtoI::UpperBound(const BDD& f) const
```

自分自身が取りうる最大値を計算するために、f=1の入力に対してのみ、
すべての入力変数が上限のときに最大となる関数を表すBtoIオブジェクトを生成し、
それを返す。記憶あふれの場合は、null を表すオブジェクトを返す。自分自身または
fが nullのときは nullを返す。

```cpp
BtoI BtoI::LowerBound(void) const
```

自分自身が取りうる最小値を計算するために、すべての入力変数が
下限のときに最小となる関数を表すBtoIオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が nullのときは
nullを返す。

```cpp
BtoI BtoI::LowerBound(const BDD& f) const
```

自分自身が取りうる最小値を計算するために、f=1の入力に対してのみ、
すべての入力変数が下限のときに最小となる関数を表すBtoIオブジェクトを生成し、
それを返す。記憶あふれの場合は、null を表すオブジェクトを返す。自分自身または
fが nullのときは nullを返す。

```cpp
BtoI BtoI::At0(int v) const
```

自分自身の関数の入力変数のうち、var番目の変数を0に固定したときの
論理関数を表すBtoIオブジェクトを生成し、それを返す。記憶あふれの場合は、
null を表すオブジェクトを返す。自分自身が null のときは、nullを返す。

```cpp
BtoI BtoI::At1(int v) const
```

自分自身の関数の入力変数のうち、var番目の変数を1に固定したときの
論理関数を表すBtoIオブジェクトを生成し、それを返す。記憶あふれの場合は、
null を表すオブジェクトを返す。自分自身が null のときは、nullを返す。

```cpp
BtoI BtoI::Cofact(const BtoI& g) const
```

自分自身のグラフに対して、g = 0 の部分を don't care とみなして簡単化を
行った論理関数を表すBtoIオブジェクトを生成し、それを返す。記憶あふれの場合
は、null を表すオブジェクトを返す。自分自身が null のとき、および g が
nullのときは、null を返す。

```cpp
BtoI BtoI::Spread(int k) const
```

自分自身を表す関数について、すべての非負定数kに対して同じ出力を返す関数
（すなわちk番目以降のビットをすべて反復する関数）を表すBtoIオブジェクトを
生成し、それを返す。記憶あふれの場合は、null を表すオブジェクトを返す。
自分自身が null のときは、nullを返す。

```cpp
int BtoI::Top(void) const
```

自分自身のグラフに関係する入力変数の中で、最上位の変数番号を返す。
自分自身が定数関数のとき、及びnullの場合は0を返す。

```cpp
BDD BtoI::GetSignBDD(void) const
```

自分自身から最上位ビットを表すBDDを抽出する。最上位ビットが1のとき
負の値であり、0のとき非負値を表す。記憶あふれの場合は、null を表す
オブジェクトを返す。自分自身が null のときは、nullを返す。

```cpp
BDD BtoI::GetBDD(int ix) const
```

自分自身からix番目のビットを表すBDDを抽出する。記憶あふれの場合は、
null を表すオブジェクトを返す。自分自身が null のときは、nullを返す。

```cpp
int BtoI::Len(void) const
```

自分自身のビット長を返す。nullに対しては0を返す。

```cpp
BDDV BtoI::GetBDDV(void) const
```

内部表現で使っている BDDV を複製して返す。nullに対してはnullを返す。

```cpp
bddword BtoI::Size(void) const
```

自分自身が使用するグラフ節点数を返す。nullに対しては 0 を返す。

```cpp
void BtoI::Print(void) const
```

自分自身のグラフ情報を標準出力に出力する。

---

