# CtoI  --- 整数値組合せ集合（整係数ユネイト論理式）を表すクラス

ヘッダーファイル名: "CtoI.h"  
ソースファイル名: CtoI.cc  
内部から呼び出しているクラス: ZBDD, ZBDDV

ZBDDを用いて整数値組合せ集合（整係数ユネイト論理式）を表すクラス。
アイテム集合に対応する整数値の集合を表現する。整数値は符号付きの一般の整数
（C言語のint型）である。組合せ集合の各要素のことをベクトルとも呼び、
また整数値のことを重みまたは係数とも呼ぶ。内部表現はZBDDV であり、
配列要素0が値0に対応するベクトルの集合（まとめたZBDD）、
配列要素1が値1に対応するベクトルの集合、
配列要素2が値2に対応するベクトルの集合、...
配列要素-1が値-1に対応するベクトルの集合、
配列要素-2が値-2に対応するベクトルの集合、...
のような形で保持している。
値の絶対値の上限は（コンパイル時の設定にもよるが）通常 1000000000である。
記憶あふれの場合は null を返す。

（使用例）
```cpp
int v1 = BDD_NewVar();
int v2 = BDD_NewVar();
int v3 = BDD_NewVar();
CtoI f1 = CtoI(1) * ZBDD(v1);
CtoI f2 = CtoI(2) * ZBDD(v2) * ZBDD(v3);
CtoI f3 = f1 + f2;
f1.Print();
f2.Print();
f3.Print();
```

### 関連する外部関数

### operator+

```cpp
CtoI operator+(const CtoI& f, const CtoI& g)
```

f と g のベクトル同士の係数加算した結果を表す CtoI オブジェクト
を生成して、それを返す。この演算は集合族の和のような演算である。
記憶あふれの場合はnull を表すオブジェクトを返す。引数にnullを与えた場合にはnullを返す。

### operator-

```cpp
CtoI operator-(const CtoI& f, const CtoI& g)
```

f から g のベクトル同士の係数を減算した結果を表す CtoI オブジェクト
を生成して、それを返す。この演算はベクトル係数の符号を反転する演算と
和の演算の組合せとして定義される。
記憶あふれの場合はnull を表すオブジェクトを返す。引数にnullを与えた場合にはnullを返す。

### operator*

```cpp
CtoI operator*(const CtoI& f, const CtoI& g)
```

f と g の両方に現れるベクトル同士の係数を積算した結果を表す 
CtoI オブジェクトを生成して、それを返す。この演算は集合族の交わりの
ような演算である。
記憶あふれの場合はnull を表すオブジェクトを返す。引数にnullを与えた場合にはnullを返す。

### operator*

```cpp
CtoI operator*(int n, const CtoI& f)
```

f の全てのベクトルの係数を n 倍した結果を表すCtoI オブジェクト
を生成して、それを返す。この演算はスカラー倍を表す。
記憶あふれの場合はnull を表すオブジェクトを返す。引数にnullを与えた場合にはnullを返す。

### operator*

```cpp
CtoI operator*(const CtoI& f, int n)
```

f の全てのベクトルの係数を n 倍した結果を表すCtoI オブジェクト
を生成して、それを返す。この演算はスカラー倍を表す。
記憶あふれの場合はnull を表すオブジェクトを返す。引数にnullを与えた場合にはnullを返す。

### operator*

```cpp
CtoI operator*(const ZBDD& f, int n)
```

ZBDDで表された組合せ集合fの各ベクトルに係数nを乗じた結果を表す
CtoI オブジェクトを生成して、それを返す。この演算はスカラー倍を表す。
記憶あふれの場合はnull を表すオブジェクトを返す。引数にnullを与えた場合にはnullを返す。

### operator*

```cpp
CtoI operator*(int n, const ZBDD& f)
```

ZBDDで表された組合せ集合fの各ベクトルに係数nを乗じた結果を表す
CtoI オブジェクトを生成して、それを返す。この演算はスカラー倍を表す。
記憶あふれの場合はnull を表すオブジェクトを返す。引数にnullを与えた場合にはnullを返す。

### operator*

```cpp
CtoI operator*(const CtoI& f, const ZBDD& g)
```

f の各ベクトルと g の各ベクトルの共通アイテムだけを保持した結果を表す
CtoI オブジェクトを生成して、それを返す。この演算は積演算を表す。
記憶あふれの場合はnull を表すオブジェクトを返す。引数にnullを与えた場合にはnullを返す。

### operator*

```cpp
CtoI operator*(const ZBDD& f, const CtoI& g)
```

f の各ベクトルと g の各ベクトルの共通アイテムだけを保持した結果を表す
CtoI オブジェクトを生成して、それを返す。この演算は積演算を表す。
記憶あふれの場合はnull を表すオブジェクトを返す。引数にnullを与えた場合にはnullを返す。

### operator/

```cpp
CtoI operator/(const CtoI& f, int n)
```

f の各ベクトルの係数を n で割って得られる商を係数とした結果を表す
CtoI オブジェクトを生成して、それを返す。この演算はスカラー商を表す。
記憶あふれの場合はnull を表すオブジェクトを返す。引数にnullを与えた場合にはnullを返す。

### CtoI_LcmC

```cpp
CtoI CtoI_LcmC(char *fname1, char *fname2, int th)
```

fname1で指定する名前のファイルからFIMIベンチマークフォーマットのトランザクションデータベースを読み込み、LCMアルゴリズムを用いて、閾値 th 回以上出現する飽和頻出アイテム集合を表す CtoIオブジェクトを生成し、それを返す。fname2にNULL以外が指定された場合は、変数順序ファイルを読み込んで、その順序でLCMを実行する。記憶あふれの場合は null を返す。ファイル読み込みに失敗した場合はエラーメッセージを出力する。（LCM関連のメソッドは別途インストールする必要あり）

### CtoI_LcmM

```cpp
CtoI CtoI_LcmM(char *fname1, char *fname2, int th)
```

fname1で指定する名前のファイルからFIMIベンチマークフォーマットのトランザクションデータベースを読み込み、LCMアルゴリズムを用いて、閾値 th 回以上出現する極大頻出アイテム集合を表す CtoIオブジェクトを生成し、それを返す。fname2にNULL以外が指定された場合は、変数順序ファイルを読み込んで、その順序でLCMを実行する。記憶あふれの場合は null を返す。ファイル読み込みに失敗した場合はエラーメッセージを出力する。（LCM関連のメソッドは別途インストールする必要あり）

### CtoI_LcmCV

```cpp
CtoI CtoI_LcmCV(char *fname1, char *fname2, int th)
```

CtoI_LcmCとほとんど同じだが、それぞれのアイテム組合せの出現回数を保持したCtoIオブジェクトを生成し、それを返す。（LCM関連のメソッドは別途インストールする必要あり）

### CtoI_LcmMV

```cpp
CtoI CtoI_LcmMV(char *fname1, char *fname2, int th)
```

CtoI_LcmMとほとんど同じだが、それぞれのアイテム組合せの出現回数を保持したCtoIオブジェクトを生成し、それを返す。（LCM関連のメソッドは別途インストールする必要あり）

### CtoI_Lcm

```cpp
CtoI CtoI_Lcm(char *fname1, char *fname2, int th)
```

fname1で指定する名前のファイルからFIMIベンチマークフォーマットのトランザクションデータベースを読み込み、LCMアルゴリズムを用いて、閾値 th 回以上出現する頻出アイテム集合を表す CtoIオブジェクトを生成し、それを返す。fname2にNULL以外が指定された場合は、変数順序ファイルを読み込んで、その順序でLCMを実行する。記憶あふれの場合は null を返す。ファイル読み込みに失敗した場合はエラーメッセージを出力する。（LCM関連のメソッドは別途インストールする必要あり）

### 公開クラスメソッド

### CtoI

```cpp
CtoI::CtoI(void)
```

基本constructer。初期値として空の集合（何も格納されていない族）を
表す CtoI オブジェクトを生成する。

### CtoI

```cpp
CtoI::CtoI(const CtoI& f)
```

引数 f を複製する constructer。

### CtoI

```cpp
CtoI::CtoI(int n)
```

定数 n を表す CtoI オブジェクトを生成する。具体的には、空のベクトル
（何も要素を含まないアイテム集合）に係数 n を乗じたものを１つだけ
含む CtoI が生成される。n < 0 の場合は係数が負の整数の場合を表す。
n == 0 のときは空集合（何も格納されていない族）となる。

---

