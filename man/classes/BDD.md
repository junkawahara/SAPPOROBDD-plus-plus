# BDD --- BDDで表現された個々の論理関数を指すクラス

ヘッダーファイル名: "BDD.h"  
ソースファイル名: BDD.cc  
内部から呼び出しているクラス: （無し）

### 関連する定数値

```cpp
extern const bddword BDD_MaxNode 
```

１ワードで区別できる節点数の最大値。32ビット用にコンパイルされている場合では2の30乗に、
64ビット用にコンパイルされている場合では２の38乗にセットされているが、メモリ容量の限界が
あるので、実際にはもっと少ない個数しか扱うことはできない。
現実の最大節点数は、BDD_Init()の引数で指定する。

```cpp
extern const int BDD_MaxVar 
```

入力変数番号の最大値。通常 65535。

### 関連する外部関数

### BDD_Init

```cpp
int BDD_Init(bddword init=256, bddword limit=BDD_MaxNode)
```

処理系を初期化しメモリの確保を行う。bddword は unsigned int の別名である。
引数initで、最初にメモリを確保するBDD節点数を指定する。以後、演算中
にメモリを使い切った場合は、自動的にメモリの再確保が行われる。再確保毎に
節点数は 4倍に拡大される。拡大の上限は、引数limitによって指定できる。
使用節点数がlimitに達したときは、メモリの再確保はそれ以上行われず、
ガベジコレクションが起動され、空き節点が自動的に回収される。initは、
256より小さい値を指定した場合は強制的に256に設定される。initを下回る
値をlimitに指定した場合は、強制的にlimitはinitと同じ値に設定される。
適切なlimit値は計算機のメモリ容量に依存する。（32ビット用にコンパイルされている場合では
1節点当たり約25バイト、64ビット用にコンパイルされている場合では約35バイト必要とする。）
initやlimitの指定を省略した場合は、initは256に、limitは原理的な
最大値（メモリ容量の限界まで）がデフォルト値となる。 BDD_Init()による
初期化が正常に行われた場合には、関数の値として0を返し、メモリ確保に
失敗した場合1を返す。BDD_Init()を複数回実行すると、前回の内容がクリア
され、再度初期化される。BDD_Init()を一度も実行せずに演算を開始した場合は、
initとlimitには自動的にデフォルト値が設定される。

### BDD_NewVar

```cpp
int BDD_NewVar(void)
```

新しい入力変数を１つ生成し、その変数番号(通称VarID)を返す。VarIDは1から
始まる整数で、BDD_NewVar()またはBDD_NewVarOfLev()を1回実行するごとに
1ずつ大きな値が返る。生成した変数のBDD展開順位(通称level)は、VarIDと
同様に下位から順番に割り当てられる。変数の個数が最大値BDD_MaxVarを
超えるとエラーを出力して異常終了する。
なお、最初にBDDV_Init()で初期化した場合（BDDVクラスを扱う場合）には、
最初にシステム用に変数が使われるので、VarIDは (BDDV_SysVarTop + 1)から
開始し、順に1ずつ大きな値となる。

### BDD_NewVarOfLev

```cpp
int BDD_NewVarOfLev(int lev)
```

新しい入力変数を１つ生成し、その変数番号(通称VarID)を返す。VarIDは1から
始まる整数で、BDD_NewVar()またはBDD_NewVarOfLev()を1回実行するごとに
1ずつ大きな値が返る。生成した変数のBDD展開順位(通称level)は、引数levで
指定した値となる。実行時に順位levの変数がすでに存在していた場合は、
lev以上の変数を１つずつ上にずらして（levelを１ずつ増加させて）、空いた
ところに新しい変数を挿入する。引数levは1以上かつ「関数実行直前の変数の
個数＋１」以下でなければならない。そうでなければエラーを出力して異常終了する。
変数の個数が最大値BDD_MaxVarを超えるとエラーを出力して異常終了する。

### BDD_LevOfVar

```cpp
int BDD_LevOfVar(int v) 
```

引数vで指定した変数番号(通称VarID）のBDD展開順位(通称level)を返す。
引数vは1以上かつ「現在の変数の個数」以下でなければならない。
そうでなければエラーを出力して異常終了する。

### BDD_VarOfLev

```cpp
int BDD_VarOfLev(int lev)
```

引数levで指定したBDD展開順位(通称level)を持つ変数番号(通称VarID)を返す。
引数levは1以上かつ「現在の変数の個数」以下でなければならない。
そうでなければエラーを出力して異常終了する。

### BDD_VarUsed

```cpp
int BDD_VarUsed(void)
```

現在までに宣言済みの入力変数の個数を返す。
（BDDV用に自動的に宣言された特殊変数の個数も含む）

### BDD_TopLev

```cpp
int BDD_TopLev(void)
```

現在までにユーザが宣言した入力変数の最上位変数の順位(Level)を返す。
最初にBDDV_Init()で初期化した場合（BDDVクラスを扱う場合）には、
BDD_VarUsed() - BDDV_SysVarTop に等しい。
BDD_Init()で初期化した場合は、BDD_VarUsed() と等しい。

### BDD_Used

```cpp
bddword BDD_Used(void)
```

現在使用中の総節点数を返す。使用済みで再利用可能な節点も、実際に
回収されるまでは使用中として数えるため、正確な節点数を知るには、
直前に BDD_GC()を実行（ガベジコレクション起動）する必要がある。

### BDD_GC

```cpp
void BDD_GC(void)
```

強制的にガベジコレクション（不要な節点の回収）を行う。BDD_GC()を陽に
起動しなくても、記憶が足りなくなった場合には自動的に起動される。
ガベジコレクションで空き節点が回収された場合は 0 を返し、空き節点が
１個も見つからなかった場合は 1 を返す。

### BDD_CacheInt

```cpp
bddword BDD_CacheInt(unsigned char op, bddword f, bddword g)
```

f と g の演算結果が非負整数値のとき、演算結果を演算キャッシュから参照する。
引数op は演算の種類を表す番号で、20 以上の値を入れる。演算結果が
登録されている場合はその数値を返し、見つからなかった場合は、
nullに相当する数値（BDD_MaxNodeよりも約2倍大きな数値で
BDD(-1).GetID()で得られる値）を返す。f, g が BDD 型の演算の場合は、
GetID()で bddword 型に変換して与える。

### BDD_CacheBDD

```cpp
BDD BDD_CacheBDD(unsigned char op, bddword f, bddword g)
```

f と g の演算結果が BDD 型のとき、演算結果を演算キャッシュから参照する。
op は演算の種類を表す番号で、20 以上の値を入れる。演算結果が登録されて
いる場合はその BDD を返し、見つからなかった場合は、null を表すオブジェ
クトを返す。f, g が BDD 型の演算の場合は、GetID()で bddword 型に変換して与える。

### BDD_CacheEnt

```cpp
void BDD_CacheEnt(unsigned char op, bddword f, bddword g, bddword h)
```

f と g の演算結果 h を演算キャッシュに登録する。op は演算の種類を表す番
号で、20 以上の値を入れる。被演算子や演算結果が数値のときは、そのま
ま与える。BDD 型の演算の場合は、GetID()で bddword 型に変換して与える。

### BDDvar

```cpp
BDD BDDvar(int var)
```

入力変数番号var の変数そのもの（恒等関数）を表すBDDオブジェクトを
生成し、それを返す。記憶あふれの場合は、null を表すオブジェクトを返す。

### operator&

```cpp
BDD operator&(const BDD& f, const BDD& g)
```

f と g の論理積を表すBDDオブジェクトを生成し、それを返す。記憶あふれの
場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。

### operator|

```cpp
BDD operator|(const BDD& f, const BDD& g)
```

f と g の論理和を表すBDDオブジェクトを生成し、それを返す。記憶あふれの
場合は、null を表すオブジェクトを返す。引数にnullを与えた場合には
nullを返す。

### operator^

```cpp
BDD operator^(const BDD&, const BDD&)
```

f と g の排他的論理和を表すBDDオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた場合
にはnullを返す。

### operator==

```cpp
int operator==(const BDD& f, const BDD& g)
```

f と g が同じ論理関数かどうかの真偽(1/0)を返す。

### operator!=

```cpp
int operator!=(const BDD& f, const BDD& g)
```

f と g が異なる論理関数かどうかの真偽(1/0)を返す。

### BDD_Imply

```cpp
int BDD_Imply(const BDD& f, const BDD& g)
```

f と g の包含性判定を行う。すなわち、(~f | g) が恒真かどうかを調べる。
(~f | g) のBDDオブジェクトを生成せずに判定だけを行うので、(~f | g)の
演算を実行するよりも高速である。引数にnullを与えた場合には0を返す。

### BDD_Import

```cpp
BDD BDD_Import(FILE *strm = stdin)
```

strmで指定するファイルからBDDの構造を読み込み、BDDオブジェクトを生成して、それを返す。ただし、ファイルに書かれているデータが多出力であった場合は、最初の出力の論理関数のみ読み込む。ファイルに文法誤りが合った場合等、異常終了時はnullを返す。

### BDD_Random

```cpp
BDD BDD_Random(int dim, int density = 50)
```

入力数 dim (変数のlevelが1からdimまで）の乱数論理関数（真理値表が
乱数表である論理関数）を表すBDDオブジェクトを生成し、それを返す。
引数density によって、真理値表濃度（１の出現確率％）を指定することが
できる。記憶あふれの場合は、null を表すオブジェクトを返す。

### BDDerr

```cpp
void BDDerr(char *msg)
void BDDerr(char *msg, bddword key)
void BDDerr(char *msg, char *name)
```

引数として与えた文字列や数値をエラー出力に表示し、異常終了する。
通常、内部で回復不可能なエラーが起きたときに自動的に呼び出されるが、
何らかの理由で処理を途中終了したいときに陽に用いても良い。

### 公開クラスメソッド

### BDD

```cpp
BDD::BDD(void)
```

基本constructer。初期値として恒偽関数を表すBDDオブジェクトを生成する。

### BDD

```cpp
BDD::BDD(int val)
```

定数論理関数のオブジェクトを作り出す constructer。val == 0 ならば恒偽関数、
val > 0 ならば恒真関数、val < 0 ならば null を表すBDDオブジェクトを生成する。

### BDD

```cpp
BDD::BDD(const BDD& f)
```

引数 f を複製する constructer。

### ~BDD

```cpp
BDD::~BDD(void)
```

destructer。内部のBDD節点の記憶管理は自動化されており、使用済みの節点は適当なタイミングで回収され、再利用される。

### operator=

```cpp
BDD& BDD::operator=(const BDD& f)
```

自分自身に f を代入し、fを関数値として返す。

### operator&=

```cpp
BDD BDD::operator&=(const BDD& f)
```

自分自身と f との論理積を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは何もしない。f が null のときは、null を代入する。

### operator|=

```cpp
BDD BDD::operator|=(const BDD& f)
```

自分自身と f との論理和を求め、自分自身に代入する。演算結果を関数値として返す。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは何もしない。f が null のときは、null を代入する。

### operator^=

```cpp
BDD BDD::operator^=(const BDD& f)
```

自分自身と f との排他的論理和を求め、自分自身に代入する。演算結果を関数値
として返す。記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が
null のときは何もしない。f が null のときは、null を代入する。

### operator<<=

```cpp
BDD BDD::operator<<=(int s)
```

自分自身のグラフに対して、関係する全ての入力変数を展開順位(level)がsずつ
大きい（上位にある）変数の変数番号(VarID)にそれぞれ書き換えてBDDを複製した
論理関数を、自分自身に代入する。また演算結果を関数値として返す。
実行結果において未定義の入力変数が必要になるようなsを与えてはならない。
必要な入力変数はあらかじめ宣言しておくこと。記憶あふれの場合は、null を表す
オブジェクトを返す。自分自身が null のときは何もしない。sに負の値を指定する
ことはできない。

### operator>>=

```cpp
BDD BDD::operator>>=(int s)
```

自分自身のグラフに対して、関係する全ての入力変数を展開順位(level)がsずつ
小さい（下位にある）変数の変数番号(VarID)にそれぞれ書き換えてBDDを複製した
論理関数を、自分自身に代入する。また演算結果を関数値として返す。
実行結果において未定義の入力変数が必要になるようなsを与えてはならない。
したがって、fに関係しない入力変数が下位レベルにあらかじめ用意されていなければ
ならない。記憶あふれの場合は、null を表すオブジェクトを返す。自分自身がnullの
ときは何もしない。sに負の値を指定することはできない。

### operator~

```cpp
BDD BDD::operator~(void) const
```

自分自身の否定の論理関数を表すBDDオブジェクトを生成し、それを返す。
自分自身が null のときは、null を返す。

### operator<<

```cpp
BDD BDD::operator<<(int s) const 
```

自分自身のグラフに対して、関係する全ての入力変数を展開順位(level)がsずつ
大きい（上位にある）変数の変数番号(VarID)にそれぞれ書き換えてBDDを複製した
オブジェクトを生成し、それを返す。実行結果において未定義の入力変数が必要に
なるようなsを与えてはならない。必要な入力変数はあらかじめ宣言しておくこと。
記憶あふれの場合は、nullを表すオブジェクトを返す。自分自身がnullのときは
何もしない。sに負の値を指定することはできない。

### operator>>

```cpp
BDD BDD::operator>>(int s) const 
```

自分自身のグラフに対して、関係する全ての入力変数を展開順位(level)がsずつ
小さい（下位にある）変数の変数番号(VarID)にそれぞれ書き換えてBDDを複製した
オブジェクトを生成し、それを返す。実行結果において未定義の入力変数が必要
になるようなsを与えてはならない。したがって、fに関係しない入力変数が
下位レベルにあらかじめ用意されていなければならない。記憶あふれの場合は、
nullを表すオブジェクトを返す。自分自身がnullのときは何もしない。
sに負の値を指定することはできない。

### At0

```cpp
BDD BDD::At0(int var) const 
```

自分自身のグラフに対して、番号varの入力変数を0に固定したときの
論理関数（射影）を表すBDDオブジェクトを生成し、それを返す。記憶あふれ
の場合は、null を表すオブジェクトを返す。自分自身がnullのときは、
nullを返す。

### At1

```cpp
BDD BDD::At1(int var) const 
```

自分自身のグラフに対して、番号varの入力変数を1に固定したときの
論理関数（射影）を表すBDDオブジェクトを生成し、それを返す。記憶あふれ
の場合は、null を表すオブジェクトを返す。自分自身がnullのときは、
nullを返す。

### Cofact

```cpp
BDD BDD::Cofact(BDD f) const 
```

自分自身のグラフに対して、f = 0 の部分を don't care とみなして簡単化を
行った論理関数を表すBDDオブジェクトを生成し、それを返す。記憶あふれの場合
は、null を表すオブジェクトを返す。自分自身が null のとき、および f が
nullのときは、null を返す。

### Univ

```cpp
BDD BDD::Univ(BDD f) const 
```

全称作用演算(universal quantification)。f で指定した入力変数の部分集合
に0,1の定数を代入したときに、どのような0,1の組合せを代入しても常に自分
自身が1となる場合には1を返し、それ以外は0を返すような論理関数を表すオ
ブジェクトを生成し、それを返す。入力変数の部分集合の指定は、それらの変
数すべての論理和を表す論理関数を作って与える。記憶あふれの場合は、null 
を表すオブジェクトを返す。自分自身が null のとき、および f がnullのと
きは、null を返す。

### Exist

```cpp
BDD BDD::Exist(BDD f) const 
```

存在作用演算(universal quantification)。f で指定した入力変数の部分集合
に0,1の定数を代入したときに、どのような0,1の組合せを代入しても常に自分
自身が0となる場合には0を返し、それ以外は1を返すような論理関数を表すオ
ブジェクトを生成し、それを返す。入力変数の部分集合の指定は、それらの変
数すべての論理和を表す論理関数を作って与える。記憶あふれの場合は、null 
を表すオブジェクトを返す。自分自身が null のとき、および f がnullのと
きは、null を返す。

### Support

```cpp
BDD BDD::Support(void) const 
```

自分自身の論理関数の値に影響を与える入力変数の集合を抽出し、それらの変
数すべての論理和を表すオブジェクトを作りそれを返す。記憶あふれの場合は、
null を表すオブジェクトを返す。自分自身が null のとき、および f がnull
のときは、null を返す。

### Top

```cpp
int BDD::Top(void) const 
```

自分自身のグラフに対して、最上位の入力変数の番号を返す。定数関数または
null のときは、0 を返す。

### Size

```cpp
bddword BDD::Size(void) const 
```

自分自身のグラフの節点数を返す。null に対しては 0を返す。

### Export

```cpp
void BDD::Export(FILE *strm = stdout) const 
```

BDDの内部データ構造を、strmで指定するファイルに出力する。

### XPrint0

```cpp
void BDD::XPrint0(void) const 
```

自分自身のグラフを、X-Window に描画する。（否定エッジなし）

### XPrint

```cpp
void BDD::XPrint(void) const 
```

自分自身のグラフを、X-Window に描画する。（否定エッジなし）

### GetID

```cpp
bddword BDD::GetID(void) const 
```

論理関数を一意に表現する 1-word の識別番号（内部インデックス値）を返す。

### Print

```cpp
void BDD::Print(void) const 
```

BDDの内部インデックス値、最上位の変数番号、節点数の情報を標準出力に出力する。

### Swap

```cpp
BDD BDD::Swap(int var1, int var2) const 
```

自分自身のグラフに対して、変数番号var1とvar2の入力変数を
入れ換えたときの論理関数を表すオブジェクトを生成し、それを返す。
引数はlevelではなく、変数番号で与えることに注意。記憶あふれの場合は、
nullを表すオブジェクトを返す。自分自身がnullのときは、nullを返す。

### Smooth

```cpp
BDD BDD::Smooth(int var) const 
```

自分自身のグラフに対して、指定した変数番号varの順位よりも低い順位を持つ
全ての入力変数に、あらゆる0,1の組合せを代入したときに、関数の値が真になる
組合せが１つでもあるか否かを表すBDDを生成し、それを返す。計算結果のBDD
にはvarおよびそれ以下の順位を持つ変数の節点は含まれない。記憶あふれの
場合は、nullを表すオブジェクトを返す。自分自身がnullのときは、nullを返す。

---

