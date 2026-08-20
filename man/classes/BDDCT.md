# BDDCT --- BDD/ZDDでコスト制約付き変数を扱うためのクラス

ヘッダーファイル名: "BDDCT.h"  
ソースファイル名: BDDCT.cc  
内部から呼び出しているクラス: BDD, ZDD

以下では、SAPPOROBDD から SAPPOROBDD++ で変更された箇所を打消し線で示す。

BDDCT は BDD Cost Table の略で、BDD/ZDD でコスト制約付きの変数を扱うためのクラスである。
1 から n までの level の BDD 変数に対応する整数値コストの表を内部に保持し、それに基づいて
ZDD が表す組合せ集合のうちコストが閾値以下の要素だけを取り出す演算や、コストの最小値・
最大値を求める演算を提供する。

エラーが発生した場合は ~~処理を中断して戻り値で通知する~~
BDDException の子クラスの例外を投げる（後述の「例外」を参照）。

（使用例）

```cpp
#include "BDDCT.h"
using namespace sapporobdd;

int v1 = BDD_NewVar();   // level 1
int v2 = BDD_NewVar();   // level 2
int v3 = BDD_NewVar();   // level 3

BDDCT ct;
ct.Alloc(3);             // level 1..3 の表を確保（初期コストは 1）
ct.SetCostOfLev(1, 10);
ct.SetCostOfLev(2, 20);
ct.SetCostOfLev(3, 30);

ZDD f = ZDD(1).Change(v1) + ZDD(1).Change(v2) * ZDD(1).Change(v3);
                         // { {v1}, {v2,v3} }: コストは 10 と 50
ZDD g = ct.ZDD_CostLE(f, 30);   // { {v1} }
bddcost mn = ct.MinCost(f);     // 10
bddcost mx = ct.MaxCost(f);     // 50
```

## コストと変数レベルの対応

表の大きさを n とすると、第 ix 番目の要素は level (n - ix) の変数に対応する。
すなわち添字 0 が最上位 level n の変数、添字 n-1 が level 1 の変数である。
`CostOfLev()` / `SetCostOfLev()` / `LabelOfLev()` / `SetLabelOfLev()` は
この対応を内部で行うので、通常はこちらを用いる方が分かりやすい。

**表が扱う level の範囲は 1 以上 n 以下である。** これを外れる level の変数を含む ZDD を
`ZDD_CostLE()`、`ZDD_CostLE0()`、`MinCost()`、`MaxCost()` に与えた場合は
BDDOutOfRangeException を投げる。
~~表より上位の level（負の添字）の変数はコスト 1 として扱われる。~~
（旧版では表に無い変数が黙ってコスト 1 で計算され、`Alloc()` に与える n を間違えても
気付けなかった。）

## 関連する型と定数値

```cpp
typedef int bddcost;
const bddcost bddcost_null = 0x7FFFFFFF;
const int CT_STRLEN = 15;
```

- `bddcost` はコストを表す型で、符号付き 32 ビット整数である。
- `bddcost_null` は「値が無い」ことを表す番兵値である。表の範囲外の添字に対する
  `Cost()` の戻り値、空集合に対する `MinCost()` / `MaxCost()` の戻り値、
  演算キャッシュの空きスロットの印として用いる。
  したがって **コストとして格納できる値は
  -(bddcost_null - 1) 以上 bddcost_null - 1 以下**（= -2147483646 以上 2147483646 以下）
  であり、これを外れる値は `SetCost()` / `Alloc()` / `AllocRand()` が拒否する。
- `CT_STRLEN` はラベル文字列の最大長である。
- ~~この 2 つはマクロ (#define) である。~~
  SAPPOROBDD++ では名前空間 sapporobdd 内の定数であり、グローバルなマクロ空間を汚さない。

## コピーの禁止

```cpp
BDDCT(const BDDCT&) = delete;
BDDCT& operator=(const BDDCT&) = delete;
```

BDDCT はコスト表・ラベル・2 つのキャッシュを生ポインタで所有するため、
~~暗黙のコピーコンストラクタ・代入演算子が使える。~~
コピーと代入は禁止されている（コピーすると同じ領域を二重に解放してしまうため）。
関数への受け渡しは参照またはポインタで行う。

## 公開メソッド

### BDDCT::BDDCT

```cpp
BDDCT::BDDCT(void)
```

空の BDDCT オブジェクトを生成する。表の大きさは 0 である。

### BDDCT::~BDDCT

```cpp
BDDCT::~BDDCT(void)
```

確保した記憶領域をすべて解放する。キャッシュが保持していた ZDD 節点もここで解放される。

### BDDCT::Size

```cpp
int Size(void) const
```

表の大きさ（記録している変数 level の最大値）n を返す。

### BDDCT::Cost, BDDCT::CostOfLev

```cpp
bddcost Cost(const int ix) const
bddcost CostOfLev(const int lev) const
```

第 ix 番目の変数、または level lev の変数のコストを返す。
**表の範囲外（ix < 0 または ix >= n、lev <= 0 または lev > n）の場合は
いずれも bddcost_null を返す。**
~~ix が負の場合（lev が n より大きい場合）は 1 を返す。~~

### BDDCT::Label, BDDCT::LabelOfLev

```cpp
const char* Label(const int ix) const
const char* LabelOfLev(const int lev) const
```

第 ix 番目の変数、または level lev の変数のラベル文字列を返す。
表の範囲外の場合は 0 を返す。
~~戻り値は char* である。~~
戻り値は表が所有するバッファへの読み取り専用ポインタであり、
次の `Alloc()` / `Import()` / `AllocRand()` およびデストラクタで解放される。
それ以降も保持したい場合は呼び出し側でコピーすること。

### BDDCT::SetCost, BDDCT::SetCostOfLev

```cpp
int SetCost(const int ix, const bddcost cost)
int SetCostOfLev(const int lev, const bddcost cost)
```

第 ix 番目の変数、または level lev の変数にコスト値 cost を代入する。
正常終了時は 0 を返す。**戻り値 1 は「添字（level）が範囲外」または
「cost が格納可能な範囲外」のいずれかを意味する。**
コストを変更すると、それまでの計算結果は無効になるため、両方の演算キャッシュを破棄する
（この破棄は記憶領域の確保を伴わないため失敗しない）。

### BDDCT::SetLabel, BDDCT::SetLabelOfLev

```cpp
int SetLabel(const int ix, const char* label)
int SetLabelOfLev(const int lev, const char* label)
```

第 ix 番目の変数、または level lev の変数にラベル文字列 label を代入する。
正常終了時は 0 を返し、添字（level）が範囲外の場合は 1 を返す。
**文字列長が CT_STRLEN を超える場合は 1 を返し、既存のラベルは変更しない。**
~~文字列長が CT_STRLEN を超える場合は先頭の CT_STRLEN 文字のみを代入する。~~

### BDDCT::Alloc

```cpp
int Alloc(const int n, const bddcost cost = 1)
```

n 個の変数のコストを記録する領域を確保し、すべてのコストを cost に、
すべてのラベルを空文字列に初期化する。両方の演算キャッシュを破棄する。
n が負の場合は大きさ 0 の表になる。
正常終了時は 0 を返し、cost が格納可能な範囲外の場合は 1 を返す
（このとき表は変更されない）。
~~領域確保に失敗した場合は 1 を返す。~~
記憶領域の確保に失敗した場合は、空の表を残して BDDOutOfMemoryException を投げる。

### BDDCT::AllocRand

```cpp
int AllocRand(const int n, const bddcost min, const bddcost max)
```

n 個の変数のコストを記録する領域を確保し、各変数に min 以上 max 以下の
乱数コストを設定する。正常終了時は 0 を返す。
**min > max の場合、または min・max が格納可能な範囲外の場合は、表を変更せずに 1 を返す。**
~~min > max の場合はすべての変数のコストが min になる。~~

乱数は C 標準ライブラリの `rand()` で生成する。したがって `srand()` を呼ばない
プログラムは毎回同じ表を得る。実行ごとに異なる表が必要な場合は、呼び出し前に
`srand()` でシードを設定すること。

### BDDCT::Import

```cpp
int Import(FILE* fp = stdin)
```

fp で指定したファイルから表を読み込む。正常終了時は 0 を返し、
フォーマットが不正な場合は 1 を返す。
**読み込みに失敗した場合、表は必ず空（大きさ 0）になる**
（古い表や途中まで読み込んだ表が残ることはない）。
記憶領域の確保に失敗した場合は BDDOutOfMemoryException を投げる。

~~トークンは strtol() で解釈する。~~
数値トークンは検証され、数字以外を含むもの、int の範囲を超えるもの、
負の要素数はいずれもフォーマットエラーになる。
CT_STRLEN より長いラベルもフォーマットエラーである。

### BDDCT::Export

```cpp
void Export(void) const
```

自分自身の内容を標準出力に出力する。フォーマットは `Import()` と同じである。

### ファイルフォーマット

```
#n 5
123 #lev5
456 #lev4
789
-987 #lev2
-321 #lev1
```

- 空白・改行で区切られたトークンの列である。最初のトークンが変数の個数 n、
  続く n 個のトークンが各変数のコストで、上位 level から順に並ぶ。
- `#` で始まるトークンはコメントである。ただしコストの直後にある場合は、
  その変数のラベルとして扱われる（`#` の次の文字から次の空白までがラベル）。
  1 行目の `#n` も単なるコメントトークンである。
- **コメントはトークン単位であり、行単位ではない。**
  1 行目の要素数がコメントトークン `#n` と同じ行に続く以上、行単位にはできない。
  そのため `# 要修正` のような空白を含むコメントを書くと、2 語目が数値として
  読まれてフォーマットエラーになる。空白を含まない 1 トークン（例 `#要修正`）
  として書けば読み飛ばされる。
- 大きさ 0 の表（`#n 0` だけのファイル）も正当である。

## 演算メソッド

### BDDCT::ZDD_CostLE

```cpp
ZDD ZDD_CostLE(const ZDD& f, const bddcost bound)
ZDD ZDD_CostLE(const ZDD& f, const bddcost bound, bddcost& acc_worst, bddcost& rej_best)
```

ZDD f が表す組合せ集合のうち、コストの合計が bound 以下である要素だけを集めた
組合せ集合を返す。4 引数の形式では、受理された要素のコストの最大値を acc_worst に、
拒絶された要素のコストの最小値を rej_best に代入する（該当する要素が無い場合は
それぞれ bddcost_null）。

~~記憶あふれの場合は ZDD(-1) を返す。~~
記憶あふれの場合は BDDOutOfMemoryException を投げる。
f が ZDD(-1) の場合は BDDInvalidBDDValueException を投げる。

互換のため `ZBDD_CostLE()` という名前でも呼び出せる。

### BDDCT::ZDD_CostLE0

```cpp
ZDD ZDD_CostLE0(const ZDD& f, const bddcost bound)
```

`ZDD_CostLE()` と同じ結果を返す旧版のアルゴリズム。
各部分 ZDD のコスト最小値・最大値による枝刈りを行い、その値を
`MinCost()` / `MaxCost()` と共有するキャッシュに残す。
互換のため `ZBDD_CostLE0()` という名前でも呼び出せる。

### BDDCT::MinCost, BDDCT::MaxCost

```cpp
bddcost MinCost(const ZDD& f)
bddcost MaxCost(const ZDD& f)
```

ZDD f が表す組合せ集合の要素のうち、コストの最小値・最大値を返す。
f が空集合の場合は bddcost_null を返す。単位元集合（`ZDD(1)`）の場合は 0 を返す。
f が ZDD(-1) の場合は BDDInvalidBDDValueException を投げる。

### BDDCT::CallCount

```cpp
bddword CallCount(void) const
```

**【SAPPOROBDD++のみ】**
直前に実行した `ZDD_CostLE()`、`ZDD_CostLE0()`、`MinCost()`、`MaxCost()` が
行った再帰呼び出しの回数を返す。4 つの演算はいずれも開始時にこの計数を 0 に戻すので、
値は常に直前の 1 回の演算を表す。
~~この値は公開データメンバ _call である。~~
（データメンバはすべて private である。）

## 演算キャッシュ

`ZDD_CostLE()` 用のキャッシュと、`MinCost()` / `MaxCost()` / `ZDD_CostLE0()` 用の
キャッシュ（Cache0 系）の 2 つを保持する。

```cpp
int CacheClear(void)
int CacheEnlarge(void)
ZDD CacheRef(const ZDD& f, const bddcost bound, bddcost& acc_worst, bddcost& rej_best)
int CacheEnt(const ZDD& f, const ZDD& h, const bddcost acc_worst, const bddcost rej_best)

int Cache0Clear(void)
int Cache0Enlarge(void)
bddcost Cache0Ref(const unsigned char op, const ZDD& f) const
int Cache0Ent(const unsigned char op, const ZDD& f, const bddcost b)
```

- `CacheRef()` はヒットしなければ ZDD(-1) を、`Cache0Ref()` はヒットしなければ
  bddcost_null を返す。
- `Cache0Ent()` は b が bddcost_null の場合、登録せずに 1 を返す
  （bddcost_null は空きスロットの印であるため）。
- `CacheClear()` / `Cache0Clear()` は常に 0 を返す。エントリを解放するだけで、
  次の登録時まで表を確保しない。
- `CacheEnlarge()` / `Cache0Enlarge()` および `CacheEnt()` / `Cache0Ent()` は、
  記憶領域を確保できない場合に 1 を返す（キャッシュは最適化にすぎないので
  例外は投げず、計算はそのまま続行できる）。
- ~~Cache0Ref() / Cache0Ent() のキーは bddword の ID である。~~
  キーは ZDD である。

### キャッシュとガベジコレクション

**【SAPPOROBDD++のみ】**
両キャッシュはキーの ZDD をエントリ内に保持する。これは、キーとして節点 ID だけを
覚えると、その節点が解放・回収された後で同じ ID を得た無関係な関数に対して
古い結果を返してしまうためである。その代わり、キャッシュに載った節点は
`CacheClear()` / `Cache0Clear()`（および両者を呼ぶ `SetCost()` / `Alloc()`）で
キャッシュを解放するまでガベジコレクションで回収されない。
長時間動作するプログラムで節点を解放したい場合は、明示的にキャッシュを破棄すること。

### キャッシュと変数順序の変更

**【SAPPOROBDD++のみ】**
コスト演算は呼び出し時点の変数の level でコストを引くため、キャッシュエントリは
1 つの変数順序に対してのみ有効である。`BDD_NewVarOfLev()` で先頭以外の level に
変数を挿入すると、挿入位置より上の既存変数の level がすべて 1 つ上がる。
BDDCT はキャッシュを埋めた時点の「表が扱う各 level にどの変数が載っていたか」を
表ごとに記録しており、次の演算・キャッシュ登録・`CacheRef()` の呼び出し時に
現在の順序と照合し、表が扱う level の変数が入れ替わっていたら両キャッシュを
自動的に破棄する。以前は破棄されず、level 挿入後も 4 つのコスト演算すべてが
旧 level のコストによる結果を例外なしで返し続けた。
表より上の level への挿入や、先頭 level への挿入（`BDD_NewVar()` を含む）は
表が扱う level を動かさないため、キャッシュはそのまま保持される。
const メンバの `Cache0Ref()` だけは照合の代わりに、スナップショット以降に
変数が 1 つでも作られていたらミスとして bddcost_null を返す（保守的だが安全で、
次の非 const のキャッシュ呼び出しがスナップショットを取り直す）。

## 例外

| 例外クラス | 発生条件 |
| --- | --- |
| BDDInvalidBDDValueException | 演算メソッドに ZDD(-1) を与えた |
| BDDOutOfRangeException | 表が持たない level の変数を含む ZDD を与えた、コストや閾値の加減算が bddcost の範囲を超えた |
| BDDOutOfMemoryException | コスト表・ラベルの確保に失敗した、ZDD 演算が記憶あふれを起こした |
| BDDInternalErrorException | 再帰の深さが BDD_RecurLimit（8192）を超えた |

## 注意事項

- 本ライブラリ全体と同様、BDDCT も単一スレッドでの使用を前提とする。
  ~~4 つの演算はファイルスタティック変数で状態を受け渡す。~~
  ただし各インスタンスは独立しており、あるインスタンスの演算が他のインスタンスの
  状態を壊すことはない。
- コストの合計は bddcost の範囲内でなければならない。大きなコストと多数の変数の
  組合せでは合計が範囲を超えることがあり、その場合は BDDOutOfRangeException を投げる
  （黙ってオーバーフローした値を返すことはない）。
