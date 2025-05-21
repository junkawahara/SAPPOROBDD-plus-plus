# BDDCT --- BDD/ZBDDでコスト制約付き変数を扱うためのクラス

ヘッダーファイル名: "BDDCT.h"  
ソースファイル名: BDDCT.cc  
内部から呼び出しているクラス: BDD, ZBDD, (FILE)

各変数（BDDの変数やZBDDの要素）にコスト値が付けられている場合に、
変数の選び方に対してコスト制約や最適化などを扱えるようにするクラスである。
整数のコスト値は正または負の値を持ちうる（bddcost型; int型と等価）。

### 関連する外部関数

### ZBDDCT_ImportRQE

```cpp
ZBDD ZBDDCT_ImportRQE(BDDCT& bddct, FILE *strm = stdin)
```

strmで指定するファイルから、リレーショナル代数クエリ記述を読み込み、
指定した問合せの結果を表すZBDDオブジェクトを生成して、それを返す。
またbddctオブジェクトには、関連するラベル情報やリレーショナル変数の
情報などが保持される。ファイル文法について詳しくは別途マニュアルを
参照せよ。ファイルに構文上の誤りがあった場合等、異常終了時にはnullを返す。

### 公開クラスメソッド

### BDDCT

```cpp
BDDCT::BDDCT(void)
```

基本constructer。テーブルサイズ0のBDDCTを生成する。中身は空である。

### BDDCT

```cpp
BDDCT::BDDCT(const BDDCT& bddct)
```

引数 bddct を複製する constructer。

### BDDCT

```cpp
BDDCT::BDDCT(int n)
```

テーブルサイズ n のBDDCTを生成する。中身は空である。

### ~BDDCT

```cpp
BDDCT::~BDDCT(void)
```

destructer。

### AllocEntry

```cpp
int BDDCT::AllocEntry(bddcost cost = 0)
```

エントリを空き領域に確保する。確保した領域番号を返す。
引数でコスト値を与えることもできるが、あとで CostRule() や SetCost() 等のメソッド
で変更することも可能なので省略してもよい。領域がなくなった場合は -1 を返す。

### GetCost

```cpp
bddcost BDDCT::GetCost(const int var) const
```

変数VarIDに対応するコスト値を返す。

### SetCost

```cpp
void BDDCT::SetCost(const int var, const bddcost cost)
```

変数VarIDに対応するコスト値を変更する。

### GetLabel

```cpp
char *BDDCT::GetLabel(const int var) const
```

変数VarIDに対応するラベル文字列を返す。

### SetLabel

```cpp
void BDDCT::SetLabel(const int var, const char *name)
```

変数VarIDに対応するラベル文字列を設定（変更）する。

### CostRule

```cpp
int BDDCT::CostRule(const char* fname = NULL, const int dir = 0) 
```

コスト値指定ルールファイルを受け付ける。受付時にコストを書き換える。
fname が NULL のときは標準入力、それ以外は指定されたファイル名からデータを
読み込む。dir が 0 ならば昇順（レベル値小→大）の読み込み、dir が 0 より
大きければ昇順の読み込み、dir が 0 より小さければ降順（レベル値大→小）に
読み込みながらラベルを設定し、変数(VarID)のソートを行う。
ソートなしなら変数(VarID)とレベル値の一致を保つ。VarIDを１から順に割り当てる。
（特に、ZBDDの分配テーブル用には昇順、つまりレベル値の小さな方から割り振る。
レベル値は視覚的な問題で大きいほど上位、つまり値の影響が大きいとする。）
ファイルから正しく読み込めれば 0 を返し、エラーだと 1 を返す。

### GetVarByName

```cpp
int BDDCT::GetVarByName(const char *name) const
```

与えられた名前(name)にマッチするラベルを見つけ、その変数(VarID)を返す。
見つからなければ -1 を返す。

### GetVarID

```cpp
int BDDCT::GetVarID(const int var) const
```

内部ID(var)から外部ID(VarID)を取り出す。見つからなければ -1 を返す。

### Weight

```cpp
bddcost BDDCT::Weight(const ZBDD& zbdd) const
```

ZBDDの各組合せ要素にコスト値を乗じた重み付き和を計算し、それを返す。
ただし、組合せの個数は高々 ZBDDCT_CARDS_SUM 個（通常 10000）とする。
ZBDDCT_CARDS_SUM を超えた場合は、ZBDD_NILを返す。

### AllocRand

```cpp
int BDDCT::AllocRand(const int n, const bddcost min, const bddcost max)
```

fpで指定したファイルからデータを読み込んで、必要なサイズの表を確保してコスト値やラベルを記録する。正常終了の場合は0を返し、異常終了の場合は1を返す。以下にファイルフォーマットの例を示す。1行目は変数の個数を表す。2行目以降は各変数のコスト値を表す。各行の#より後、次の空白または改行まではラベル文字列を表す。ラベル文字列は省略してもよい。

```
#n 5
123 #lev5
456 #lev4
789 #lev3
-987 #lev2
-321 #lev1
```
