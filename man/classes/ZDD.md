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
ZBDD x = ZBDD(1).Change(v1);
ZBDD y = ZBDD(1).Change(v2);
ZBDD f = x + y;
ZBDD g = f.Change(v3) + f.Change(v4);
f.Print();
g.Print();
```

## 関連する定数値

```cpp
extern const bddword BDD_MaxNode
extern const int BDD_MaxVar
```

## BDD.h で定義された関数

以下の関数は BDD.h で定義されているが、ZDD クラスでも用いる。

### BDD_Init

```cpp
int BDD_Init(bddword init=256, bddword limit=BDD_MaxNode)
```

処理系を初期化しメモリの確保を行う。bddword は unsigned long long
（64ビット用にコンパイルされた場合）、または unsigned int（32ビット用に
コンパイルされた場合）の別名である。
引数initで、最初にメモリを確保するBDD節点数を指定する。以後、演算中
にメモリを使い切った場合は、自動的にメモリの再確保が行われる。再確保毎に
節点数は 4倍に拡大される。拡大の上限は、引数limitによって指定できる。
使用節点数がlimitに達したときは、メモリの再確保はそれ以上行われず、
ガベジコレクションが起動され、空き節点が自動的に回収される。initは、
256より小さい値を指定した場合は強制的に256に設定される。initを下回る
値をlimitに指定した場合は、強制的にlimitはinitと同じ値に設定される。
適切なlimit値は計算機のメモリ容量に依存する。（64ビット用にコンパイル
された場合
1節点当たり約35バイト、32ビット用に
コンパイルされた場合では約25バイト必要とする。）
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

引数vで指定した変数番号(通称VarID)のBDD展開順位(通称level)を返す。
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

## 関連する外部関数

```cpp
ZBDD operator&(const ZBDD& f, const ZBDD& g)
```

f と g の交わり(intersection)を表すZBDDオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた
場合にはnullを返す。

### operator+

```cpp
ZBDD operator+(const ZBDD& f, const ZBDD& g)
```

f と g の結び(union)を表すZBDDオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた
場合にはnullを返す。

### operator-

```cpp
ZBDD operator-(const ZBDD& f, const ZBDD& g)
```

f から g を引いた差分集合を表すZBDDオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた
場合にはnullを返す。

### operator*

```cpp
ZBDD operator*(const ZBDD& f, const ZBDD& g)
```

f と g の直積集合を表すZBDDオブジェクトを生成し、それを返す。記憶
あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた
場合にはnullを返す。

### operator/

```cpp
ZBDD operator/(const ZBDD& f, const ZBDD& g)
```

f を g で割った集合(Weak-division)を表すZBDDオブジェクトを生成し、
それを返す。記憶あふれの場合は、null を表すオブジェクトを返す。
引数にnullを与えた場合にはnullを返す。

### operator%

```cpp
ZBDD operator%(const ZBDD& f, const ZBDD& g);
```

f を g で割った剰余の集合を表すZBDDオブジェクトを生成し、それを返す。
記憶あふれの場合は、null を表すオブジェクトを返す。引数にnullを与えた
場合にはnullを返す。

### operator==

```cpp
int operator==(const ZBDD& f, const ZBDD& g)
```

f と g が同じ集合かどうかの真偽(1/0)を返す。

### operator!=

```cpp
int operator!=(const ZBDD& f, const ZBDD& g)
```

f と g が異なる集合かどうかの真偽(1/0)を返す。

### BDD_CacheZBDD

```cpp
ZBDD BDD_CacheZBDD(char op, bddword f, bddword g);
```

f と g の演算結果が ZBDD 型のとき、演算結果を演算キャッシュから参照する。op 
は演算の種類を表す番号で、20 以上の値を入れる。演算結果が登録されて
いる場合はその ZBDD を返し、見つからなかった場合は、null を表すオブジェ
クトを返す。f, g が ZBDD 型の演算の場合は、GetID()で bddword 型に変換して
与える。

### ZBDD_Import

```cpp
ZBDD ZBDD_Import(FILE *strm = stdin)
```

strmで指定するファイルからZBDDの構造を読み込み、ZBDDオブジェクトを生成して、それを返す。ただし、ファイルに書かれているデータが多出力であった場合は、最初の出力の構造のみ読み込む。ファイルに文法誤りが合った場合等、異常終了時はnullを返す。

### ZBDD_Random

```cpp
ZBDD ZBDD_Random(int dim, int density = 50)
```

次数 dim の乱数集合を表すZBDDオブジェクトを生成し、それを返す。
変数順位(level)が1からdimまでの値を持つアイテム変数を使用する。
アイテム変数はあらかじめ宣言されていなければならない。density によって、
濃度（要素数／全体集合％）を指定することができる。記憶あふれの場合は
nullを返す。

### ZBDD_Meet

```cpp
ZBDD_Meet(const ZBDD& f, const ZBDD& g)
```

f と g のMeet演算（Knuth本4巻1分冊141頁: 演習問題203参照）により得られる
集合を表すZBDDオブジェクトを生成し、それを返す。記憶あふれの場合は、null を
表すオブジェクトを返す。引数にnullを与えた場合にはnullを返す。

## 公開クラスメソッド

### ZDD

```cpp
ZDD::ZDD(void)
ZDD::ZDD(int v)
ZDD::ZDD(const ZDD& f)
ZDD::ZDD(const BDD& bdd, int offset)
```

基本constructer。引数を与えない場合は、初期値として空集合を表すZDDオブジェクトを生成する。
引数として `int v` を与えた場合は、定数式を作り出す constructer。v == 0 ならば空集合、v > 0 ならば単位元集合、
v < 0 ならば null を表すZDDオブジェクトを生成する。

引数として `const ZDD& f` を与えた場合は、引数 f を複製する。

引数として `const ZDD& f, int offset` を与えた場合は、BDDオブジェクトから変換する。
各変数をoffset だけシフトして
ZDD変数に割り当てる。offsetに0を指定すると、0シフト（元の変数番号のまま）
となる。記憶あふれの場合は null オブジェクトを生成する。
例外として、変数の値が0のBDDの場合は空集合のZDDとなる。

### ~ZDD

```cpp
ZDD::~ZDD(void)
```

destructer。

### operator=

```cpp
ZBDD& ZBDD::operator=(const ZBDD& f)
```

自分自身に f を代入し、そのコピーを返す。

### operator&=

```cpp
ZBDD ZBDD::operator&=(const ZBDD& f)
```

自分自身と f との交わり(intersection)を求め、自分自身に代入し、
そのコピーを返す。記憶あふれの場合は、null を表すオブジェクトを
代入する。自分自身または引数が null の場合も null となる。

### operator+=

```cpp
ZBDD ZBDD::operator+=(const ZBDD& f)
```

自分自身と f との結び(union)を求め、自分自身に代入し、そのコピーを
返す。記憶あふれの場合は、null を表すオブジェクトを代入する。
自分自身または引数が null の場合も null となる。

### operator-=

```cpp
ZBDD ZBDD::operator-=(const ZBDD& f)
```

自分自身から f を引いた差分集合を求め、自分自身に代入し、そのコピーを
返す。記憶あふれの場合は、null を表すオブジェクトを代入する。
自分自身または引数が null の場合も null となる。

### operator*=

```cpp
ZBDD ZBDD::operator*=(const ZBDD& f)
```

自分自身と f との直積集合を求め、自分自身に代入し、そのコピーを返す。
記憶あふれの場合は、null を表すオブジェクトを代入する。
自分自身または引数が null の場合も null となる。

### operator/=

```cpp
ZBDD ZBDD::operator/=(const ZBDD& f)
```

自分自身を f で割った集合(Weak division)を求め、自分自身に代入し、
そのコピーを返す。記憶あふれの場合は、null を表すオブジェクトを代入する。
自分自身または引数が null の場合も null となる。

### operator%=

```cpp
ZBDD ZBDD::operator%=(const ZBDD& f)
```

自分自身を f で割った余りの集合を求め、自分自身に代入し、そのコピーを返す。
記憶あふれの場合は、null を表すオブジェクトを代入する。
自分自身または引数が null の場合も null となる。

### operator<<=

```cpp
ZBDD ZBDD::operator<<=(int s)
```

自分自身のグラフに対して、関係する全てのアイテム変数を、展開順位(level)がsずつ
大きい（上位にある）変数の変数番号(VarID)にそれぞれ書き換えてZBDDを複製した
組合せ集合を、自分自身に代入する。また演算結果を関数値として返す。
実行結果において未定義の入力変数が必要になるようなsを与えてはならない。
必要な入力変数はあらかじめ宣言しておくこと。記憶あふれの場合は、null を表す
オブジェクトを返す。自分自身が null のときは何もしない。sに負の値を指定する
ことはできない。

### operator>>=

```cpp
ZBDD ZBDD::operator>>=(int s)
```

自分自身のグラフに対して、関係する全てのアイテム変数を、展開順位(level)がsずつ
小さい（下位にある）変数の変数番号(VarID)にそれぞれ書き換えてZBDDを複製した
組合せ集合を、自分自身に代入する。また演算結果を関数値として返す。
実行結果において未定義の入力変数が必要になるようなsを与えてはならない。
必要な入力変数はあらかじめ宣言しておくこと。記憶あふれの場合は、null を表す
オブジェクトを返す。自分自身が null のときは何もしない。sに負の値を指定する
ことはできない。

### operator<<

```cpp
ZBDD ZBDD::operator<<(int) const
```

自分自身のグラフに対して、関係する全てのアイテム変数を、展開順位(level)がsずつ
大きい（上位にある）変数の変数番号(VarID)にそれぞれ書き換えてZBDDを複製した
オブジェクトを生成し、それを返す。実行結果において未定義の入力変数が必要に
なるようなsを与えてはならない。必要な入力変数はあらかじめ宣言しておくこと。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
何もしない。sに負の値を指定することはできない。

### operator>>

```cpp
ZBDD ZBDD::operator>>(int s) const
```

自分自身のグラフに対して、関係する全てのアイテム変数を、展開順位(level)がsずつ
小さい（下位にある）変数の変数番号(VarID)にそれぞれ書き換えてZBDDを複製した
オブジェクトを生成し、それを返す。実行結果において未定義の入力変数が必要に
なるようなsを与えてはならない。必要な入力変数はあらかじめ宣言しておくこと。
記憶あふれの場合は、null を表すオブジェクトを返す。自分自身が null のときは
何もしない。sに負の値を指定することはできない。

### OffSet

```cpp
ZBDD ZBDD::OffSet(int var) const
```

自分自身のグラフに対して、変数番号varのアイテムを含まない組合せからなる
部分集合を表すZBDDオブジェクトを生成し、それを返す。記憶あふれの場合は、
nullを表すオブジェクトを返す。自分自身がnullだった場合はnullを返す。

### OnSet

```cpp
ZBDD ZBDD::OnSet(int var) const
```

自分自身のグラフに対して、変数番号varのアイテムを含む組合せからなる
部分集合を表すZBDDオブジェクトを生成し、それを返す。記憶あふれの場合は、
nullを表すオブジェクトを返す。自分自身がnullだった場合はnullを返す。

### OnSet0

```cpp
ZBDD ZBDD::OnSet0(int var) const
```

Onset(var)を実行した後、変数番号varのアイテムを除去した集合を表す
ZBDDオブジェクトを生成し、それを返す。Onset(var).Change(var)と等価。
varがグラフの最上位の変数番号の場合は、1-エッジ が指しているサブグラフ
をそのまま返すことになる。記憶あふれの場合は、null を表すオブジェクトを
返す。自分自身がnull だった場合は null を返す。

### Change

```cpp
ZBDD ZBDD::Change(int) const
```

自分自身のグラフに対して、変数番号varのアイテムの有無を反転させた集合
を表すZBDDオブジェクトを生成し、それを返す。記憶あふれの場合は、null を
表すオブジェクトを返す。自分自身がnull だった場合は null を返す。

### Swap

```cpp
ZBDD Swap(int var1, int var2) const
```

自分自身のグラフに対して、変数番号var1とvar2のアイテム変数を
入れ換えたときの論理関数を表すZBDDオブジェクトを生成し、それを返す。引数は
levelではなく、変数番号で与えることに注意。記憶あふれの場合は、nullを
表すオブジェクトを返す。自分自身がnullのときは、nullを返す。

### Restrict

```cpp
ZBDD ZBDD::Restrict(ZBDD f) const
```

自分自身の組合せ集合の要素となっている組合せの中で、fの中の少なくとも
１つの組合せを包含しているような組合せだけを抽出し、抽出した組合せ集合を表す
ZBDDオブジェクトを生成してそれを返す。記憶あふれの場合は、nullを返す。
自分自身または引数がnullの場合もnullを返す

### Permit

```cpp
ZBDD ZBDD::Permit(ZBDD f) const
```

自分自身の組合せ集合の要素となっている組合せの中で、fの中の少なくとも
１つの組合せに包含されているような組合せだけを抽出し、抽出した組合せ集合を表す
ZBDDオブジェクトを生成してそれを返す。記憶あふれの場合は、nullを返す。
自分自身または引数がnullの場合もnullを返す

### PermitSym

```cpp
ZBDD ZBDD::PermitSym(int n) const
```

自分自身の組合せ集合の要素となっている組合せの中で、アイテム個数がn個以下の
組合せだけを抽出した組合せ集合を表すZBDDオブジェクトを生成してそれを返す。
記憶あふれの場合は、nullを返す。自分自身または引数がnullの場合もnullを返す

### Support

```cpp
ZBDD ZBDD::Support(void) const
```

自分自身の集合に現れるアイテムを抽出し、それらのアイテム１個ずつを要
素とする集合を表すオブジェクトを生成し、それを返す。記憶あふれの場合は、
null を表すオブジェクトを返す。

### Always

```cpp
ZBDD ZBDD::Always(void) const
```

自分自身の集合に属する組合せ全てに共通して現れるアイテムを抽出し、
それらのアイテム１個ずつを要素とする集合を表すオブジェクトを生成し、
それを返す。記憶あふれの場合は、null を表すオブジェクトを返す。

### IsPoly

```cpp
int ZBDD::IsPoly(void) const
```

自分自身の集合に組合せが複数個含まれるかどうかの真偽を返す。

### Top

```cpp
int ZBDD::Top(void) const
```

自分自身の組合せ集合に関係するアイテム変数のうち、最上位の順位を持つ
アイテムの変数番号を返す。null に対しては 0 を返す。

### GetID

```cpp
bddword ZBDD::GetID(void) const
```

集合を一意に表現する 1-word の識別番号を返す。

### Size

```cpp
bddword ZBDD::Size(void) const
```

自分自身のグラフの節点数を返す。null に対しては 0 を返す。

### Card

```cpp
bddword ZBDD::Card(void) const
```

自分自身が表す集合の要素数(cardinality)を返す。null に対しては 0 を返す。

### CardMP16

```cpp
char* ZBDD::CardMP16(char* s) const
```

自分自身が表す集合の要素数(cardinality)を最大16ワード長までの
多倍長整数でカウントする。結果は16進数文字列としてsから始まる
記憶領域に格納する。（詳細はC言語版の bddcardmp16 を参照）

### Lit

```cpp
bddword ZBDD::Lit(void) const
```

自分自身が表す集合中の総リテラル数（各組合せのアイテム個数の総和）を返す。
null に対しては 0 を返す。

### Len

```cpp
bddword ZBDD::Len(void) const
```

自分自身が表す集合の中で、最もアイテム数を多く含む組合せを探し出して、
そのアイテム数を返す。null に対しては 0 を返す。

### Export

```cpp
void ZBDD::Export(FILE *strm = stdout) const
```

ZBDDの内部データ構造を、strmで指定するファイルに出力する。

### PrintPla

```cpp
void ZBDD::PrintPla(void) const
```

自分自身が表す集合を表形式（pla format）で標準出力に出力する。

### XPrint

```cpp
void ZBDD::XPrint(void) const
```

自分自身のグラフを、X-Window に描画する。

### XPrint0

```cpp
void ZBDD::XPrint0(void) const
```

自分自身のグラフを、X-Window に描画する。（否定エッジなし）

### Print

```cpp
void Print(void) const
```

インデックスの値、最上位のリテラル番号、節点数の情報を標準出力に出力する。

### ZLev

```cpp
bddword ZBDD::ZLev(int lev, int last = 0) const
```

自分自身のZBDDについて、最上位節点から0-枝を順にたどって行って、アイテム変数のレベルがちょうどlev となっている節点があれば、それを最上位節点とするZBDDオブジェクトをコピーして返す。変数レベルがちょうどlevとなっている節点がなければ、lastがゼロのときは、lev以下となる最初の節点を最上位節点とするZBDDオブジェクトをコピーして返す。lastが非ゼロのときは、lev以上である最後の節点を最上位節点とするZBDDオブジェクトをコピーして返す。nullに対してはnullを返す。なお、ZBDDオブジェクトに対してあらかじめSetZSkip()を1回実行しておくと、補助リンクのおかげでZLevメソッドが高速に行える。

### SetZSkip

```cpp
void ZBDD::SetZSkip(void) const
```

自分自身のZBDDについて、0-枝を高速にたどるための補助リンクを張る。それ以外には基本的に変化しない。

### Intersec

```cpp
bddword ZBDD::Intersec(ZBDD g) const
```

自分自身とgとの共通集合を表すZBDDオブジェクトを生成し、それを返す。どちらかがnullならばnullを返す。あらかじめSetZSkip()を1回実行しておくと、補助リンクのおかげで高速に実行できる。特に、自分自身のZBDDオブジェクトが含むアイテム変数の個数が非常に多く、gに出現するアイテム変数の個数が非常に少ないときに有効である。
