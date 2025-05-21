# BDDV  --- BDDの配列（論理関数の配列）を表すクラス

ヘッダーファイル名: "BDD.h"  
ソースファイル名: BDD.cc  
内部から呼び出しているクラス: BDD

BDD の配列を表すクラスである。配列長は可変である。要素の番号は 0 か
ら始まる整数である。内部構造は、「出力選択変数」を導入して、
１個のBDDに束ねて配列を表現しているので、もし配列長が大きくても、
各要素が同じ関数であれば、メモリ使用量は１個のBDDと変わらない。
また、配列の複製が配列長に関わらず定数時間で行える。
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

```cpp
extern const bddword BDD_MaxNode 
extern const int BDD_MaxVar 
```

### 関連する外部関数

### BDDV_Init

```cpp
int BDDV_Init(bddword init=256, bddword limit=BDD_NodeMax)
```

BDD_Init()と同様に、処理系を初期化しメモリの確保を行う。BDD_Init()との
相違点は、出力選択変数の確保を行う点である。入力変数番号1から
BDDV_SysVarTop(通常20）までの変数が、出力選択変数としてシステムに
確保され、ユーザが論理演算に用いる入力変数はその次の番号(通常21)から
始まる。出力選択変数はユーザ変数よりも必ず上位になるように自動的に
配置される。BDDVを用いる場合は、必ず最初にBDDV_Init()を実行しなければ
ならない。

### BDDV_UserTopLev

```cpp
int BDDV_UserTopLev(void)
```

（このメソッドは旧版で用いていた。なくても困らないはずである）
現在までにユーザが宣言した論理変数の個数。出力選択変数は含まれない。
この数値は、現在までにユーザが宣言した論理変数の順位(level)の最上位の
順位番号と等しい。出力選択変数の順位番号(level)は、そのすぐ上の範囲に
あり、(BDDV_UserTopLev + 1)から(BDDV_UserTopLev + BDDV_SysVarTop)までの
間である。

### BDDV_NewVar

```cpp
int BDDV_NewVar(void)
```

（このメソッドは旧版で用いていた。なくても困らないはずである）
BDD_NewVar()と同様に、新しい入力変数を１つ生成し、その変数番号(通称VarID)を
返す。BDD_NewVar()との違いは、VarIDが1から始まるのではなく、
(BDDV_SysVarTop + 1)から始まる点である。ただし変数の順位(通称level)は1から
スタートする。出力選択変数のlevelは1ずつ上位にシフトしていく。

### BDDV_NewVarOfLev

```cpp
int BDDV_NewVarOfLev(int lev)
```

（このメソッドは旧版で用いていた。なくても困らないはずである）
BDD_NewVarOfLev()と同様に、順位（通称level）を指定して新しい入力変数を
１つ生成し、その変数番号(通称VarID)を返す。BDD_NewVar()との違いは、
VarIDが1から始まるのではなく、(BDDV_SysVarTop + 1)から始まる点である。
ただし指定できる変数の順位(通称level)は、1以上かつ「これまでユーザが宣言
した変数の個数 +1」までである。出力選択変数のlevelは1ずつ上位にシフトしていく。

### operator&

```cpp
BDDV operator&(const BDDV& fv, const BDDV& gv)
```

fvとgvの各要素同士の論理積を表すBDDVオブジェクトを生成し、それを返す。
配列長が一致していなければエラー（異常終了）。記憶あふれの場合は 長さ1のnullを
返す。引数にnullが含まれていた場合には、長さ1のnullを返す。

### operator|

```cpp
BDDV operator|(const BDDV& fv, const BDDV& gv)
```

fvとgvの各要素同士の論理和を表すBDDVオブジェクトを生成し、それを返す。
配列長が一致していなければエラー（異常終了）。記憶あふれの場合は 長さ1のnullを
返す。引数にnullが含まれていた場合には、長さ1のnullを返す。

### operator^

```cpp
BDDV operator^(const BDDV& fv, const BDDV& gv)
```

fvとgvの各要素同士の排他的論理和を表すBDDVオブジェクトを生成し、それを返す。
配列長が一致していなければエラー（異常終了）。記憶あふれの場合は 長さ1のnullを
返す。引数にnullが含まれていた場合には、長さ1のnullを返す。

### operator==

```cpp
int operator==(const BDDV& fv, const BDDV& gv)
```

fvとgvの対応する要素が全て同じ論理関数かどうかの真偽(1/0)を返す。
配列長が一致していなければエラー（異常終了）。

### operator!=

```cpp
int operator!=(const BDDV& fv, const BDDV& gv)
```

fvとgvの対応する要素の少なくとも1組が異なる論理関数かどうかの
真偽(1/0)を返す。配列長が一致していなければエラー（異常終了）。

### BDDV_Imply

```cpp
int BDDV_Imply(BDDV fv, BDDV gv)
```

fv と gv の包含性判定を行う。すなわち、(~fv | gv) が全ての要素について
恒真かどうかを調べる。(~fv | gv) のBDDVオブジェクトを生成せずに
判定だけを行うので、(~fv | gv)の演算を実行するよりも高速である。
引数にnullを与えた場合には0を返す。

### BDDV_Import

```cpp
BDDV BDDV_Import(FILE *strm = stdin)
```

strmで指定するファイルからBDDVの構造を読み込み、BDDVオブジェクトを生成して、それを返す。ファイルに文法誤りが合った場合等、異常終了時はnullを返す。

### BDDV_ImportPla

```cpp
BDDV BDDV_ImportPla(FILE *strm = stdin, int sopf = 0)
```

strmで指定するファイルからESPRESSOフォーマットのPLAデータを読み込み、BDDVオブジェクトを生成して、それを返す。ESPRESSOフォーマットでは、.i .o .type .e のキーワードのみサポートする。生成されるBDDVオブジェクトの要素数は、入力されたPLAデータの出力数のちょうど2倍になっており、前半部分がonset関数、後半部分がdcset関数を表現している。sopfフラグが0以外の場合は、SOPクラスとデータ変換がしやすいように、偶数番号のVarIDを使用する。ファイルに文法誤りが合った場合等、異常終了時はnullを返す。

### operator||

```cpp
BDDV operator||(const BDDV& fv, const BDDV& gv)
```

fvの末尾にgvを連結したBDDVオブジェクトを生成し、それを返す。
fv, gv は変化しない。fv の配列長が２のべき乗数のとき、処理効率がよい。
記憶あふれの場合は 長さ1のnullを返す。引数にnullが含まれていた場合には、
長さ1のnullを返す。

### BDDV_Mask1

```cpp
BDDV BDDV_Mask1(int ix, int len)
```

ix 番目の要素だけが恒真関数で、他は恒偽関数となっているような、長さ
len の多出力定数論理関数を表すBDDVオブジェクトを生成し、それを返す。
記憶あふれの場合は 長さ1のnullを返す。引数にnullが含まれていた場合には、
長さ1のnullを返す。不当な引数を与えた場合はエラー（異常終了）。

### BDDV_Mask2

```cpp
BDDV BDDV_Mask2(int ix, int len)
```

0番目～(ix-1) 番目の要素が恒偽関数で、ix番目以降は恒真関数となっているような、
長さlen の多出力定数論理関数を表すBDDVオブジェクトを生成し、それを返す。
記憶あふれの場合は 長さ1のnullを返す。引数にnullが含まれていた場合には、
長さ1のnullを返す。不当な引数を与えた場合はエラー（異常終了）。
(再掲)
### BDD_NewVar

```cpp
int BDD_NewVar(void)
int BDD_NewVarOfLev(int lev)
int BDD_LevOfVar(int v) 
int BDD_VarOfLev(int lev)
int BDD_VarUsed(void)
int BDD_TopLev(void)
bddword BDD_Used(void)
void BDD_GC(void)
```

### 公開クラスメソッド

### BDDV

```cpp
BDDV::BDDV(void)
```

基本constructer。配列長 0 のBDDVオブジェクトを生成する。

### BDDV

```cpp
BDDV::BDDV(const BDDV& fv)
```

引数 fv を複製する constructer。

### BDDV

```cpp
BDDV::BDDV(const BDD&, int len = 1)
```

引数 fで指定したBDDがlen個続けて並んでいるBDDVオブジェクトを
生成するconstructer。f に null を与えた場合は、len指定に関わらず
長さ1となる。

### ~BDDV

```cpp
BDDV::~BDDV(void)
```

destructer。

### operator=

```cpp
BDDV& BDDV::operator=(const BDDV& fv)
```

自分自身の元のデータを消去し、fv を代入する。関数の値としてfvを返す。

### operator&=

```cpp
BDDV BDDV::operator&=(const BDDV& fv)
```

自分自身と fv の各要素同士の論理積を求め、自分自身に代入する。配列長は一致
していなければならない。記憶あふれの場合は 長さ1のnullを返す。自分自身
または引数にnullが含まれていた場合には、長さ1のnullを返す。

### operator|=

```cpp
BDDV BDDV::operator|=(const BDDV& fv)
```

自分自身と fv の各要素同士の論理和を求め、自分自身に代入する。配列長は一致
していなければならない。記憶あふれの場合は 長さ1のnullを返す。自分自身
または引数にnullが含まれていた場合には、長さ1のnullを返す。

### operator^=

```cpp
BDDV BDDV::operator^=(const BDDV& fv)
```

自分自身と fv の各要素同士の排他的論理和を求め、自分自身に代入する。配列長は
一致していなければならない。記憶あふれの場合は 長さ1のnullを返す。自分自身
または引数にnullが含まれていた場合には、長さ1のnullを返す。

### operator<<=

```cpp
BDDV BDDV::operator<<=(int s)
```

自分自身の各要素に対して、関係する全ての入力変数を展開順位(level)がsずつ
大きい（上位にある）変数の変数番号(VarID)にそれぞれ書き換えて複製した
BDDVを、自分自身に代入する。また演算結果を関数値として返す。出力選択変数には
影響はない。実行結果において未定義の入力変数が必要になるようなsを与えて
はならない。必要な入力変数はあらかじめ宣言しておくこと。記憶あふれの場合は、
null を表すオブジェクトを返す。自分自身が null のときは何もしない。sに負の値を
指定することはできない。

### operator>>=

```cpp
BDDV BDDV::operator>>=(int s)
```

自分自身の各要素に対して、関係する全ての入力変数を展開順位(level)がsずつ
小さい（下位にある）変数の変数番号(VarID)にそれぞれ書き換えて複製した
BDDVを、自分自身に代入する。また演算結果を関数値として返す。出力選択変数には
影響はない。実行結果において未定義の入力変数が必要になるようなsを与えて
はならない。必要な入力変数はあらかじめ宣言しておくこと。記憶あふれの場合は、
null を表すオブジェクトを返す。自分自身が null のときは何もしない。sに負の値を
指定することはできない。

### operator~

```cpp
BDDV BDDV::operator~(void) const 
```

自分自身の各要素の否定関数を表すBDDVオブジェクトを生成し、それを返す。
自分自身にnullが含まれていた場合には、長さ1のnullを返す。

### operator<<

```cpp
BDDV BDDV::operator<<(int s) const 
```

自分自身の各要素に対して、関係する全ての入力変数を展開順位(level)がsずつ
大きい（上位にある）変数の変数番号(VarID)にそれぞれ書き換えて複製した
BDDVを生成し、それを返す。出力選択変数には影響はない。実行結果において
未定義の入力変数が必要になるようなsを与えてはならない。必要な入力変数は
あらかじめ宣言しておくこと。記憶あふれの場合は、null を表すオブジェクトを返す。
自分自身が null のときは何もしない。sに負の値を指定することはできない。

### operator>>

```cpp
BDDV BDDV::operator>>(int s) const 
```

自分自身の各要素に対して、関係する全ての入力変数を展開順位(level)がsずつ
小さい（下位にある）変数の変数番号(VarID)にそれぞれ書き換えて複製した
BDDVを生成し、それを返す。出力選択変数には影響はない。実行結果において
未定義の入力変数が必要になるようなsを与えてはならない。必要な入力変数は
あらかじめ宣言しておくこと。記憶あふれの場合は、null を表すオブジェクトを返す。
自分自身が null のときは何もしない。sに負の値を指定することはできない。

### At0

```cpp
BDDV BDDV::At0(int var) const 
```

自分自身の各要素に対して、番号varの入力変数を0に固定したときの
論理関数（射影）の配列を表すBDDVオブジェクトを生成し、それを返す。
記憶あふれの場合は 長さ1のnullを返す。自分自身がnullのときは、
nullを返す。

### At1

```cpp
BDDV BDDV::At1(int var) const 
```

自分自身の各要素に対して、番号varの入力変数を1に固定したときの
論理関数（射影）の配列を表すBDDVオブジェクトを生成し、それを返す。
記憶あふれの場合は 長さ1のnullを返す。自分自身がnullのときは、
nullを返す。

### Cofact

```cpp
BDDV BDDV::Cofact(BDD f) const 
```

自分自身の各要素に対して、f = 0 の部分を don't care とみなして簡単化を
行った論理関数の配列を表すBDDVオブジェクトを生成し、それを返す。
記憶あふれの場合は 長さ1のnullを返す。自分自身がnullのとき、
およびfがnullのときは、nullを返す。

### Univ

```cpp
BDDV BDDV::Univ(BDD f) const 
```

自分自身の各要素に対して、全称作用演算を行い、その論理関数の配列を表す
BDDVオブジェクトを生成し、それを返す。入力変数の部分集合の指定は、
それらの変数すべての論理和を表す論理関数を作って与える。
記憶あふれの場合は 長さ1のnullを返す。自分自身がnullのとき、
およびfがnullのときは、nullを返す。

### Exist

```cpp
BDDV BDDV::Exist(BDD f) const 
```

自分自身の各要素に対して、存在作用演算を行い、その論理関数の配列を表す
BDDVオブジェクトを生成し、それを返す。入力変数の部分集合の指定は、
それらの変数すべての論理和を表す論理関数を作って与える。
記憶あふれの場合は 長さ1のnullを返す。自分自身がnullのとき、
およびfがnullのときは、nullを返す。

### Support

```cpp
BDDV BDDV::Support(void) const 
```

自分自身の各要素に含まれる変数をすべて集めた論理和のBDDを求め、
BDDVの先頭要素にして長さ1のBDDVオブジェクトを生成し、それを返す。
記憶あふれの場合は 長さ1のnullを返す。自分自身がnullのときは、
nullを返す。

### Top

```cpp
int BDDV::Top(void) const 
```

自分自身が含む任意のBDDに含まれる最上位の入力変数番号を返す。
nullのときは、0を返す。

### Elem

```cpp
int BDDV::Elem(int ix) const 
```

自分自身のix番目の要素のBDDを返す。ixが0以上配列長未満でなければ
エラー（異常終了）。

### Last

```cpp
int BDDV::Last(void) const 
```

自分自身の配列長を返す。正常な配列長は 0 以上の値である。

### Size

```cpp
bddword BDDV::Size(void) const 
```

自分自身の総節点数を返す。nullのときは、0を返す。

### Export

```cpp
void BDDV::Export(FILE *strm = stdout) const 
```

BDDVの内部データ構造を、strmで指定するファイルに出力する。

### Print

```cpp
void BDDV::Print(void) const 
```

BDDVの配列長、およびBDDクラスの Print() を全配列要素に対して実行する。

---

