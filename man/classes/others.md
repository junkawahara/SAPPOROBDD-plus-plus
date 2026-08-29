Please see the [original SAPPORBDD manual](https://github.com/Shin-ichi-Minato/SAPPOROBDD/tree/main/man) for classes except for [ZDD.md](ZDD.md) and [BDDCT.md](BDDCT.md).

## BDD_Hash / ZDD_Hash

BDD（ZDD）をキーとし、`void*` を値とするハッシュ表である。
`BDD.h`（`ZDD.h`）で宣言され、SAPPOROBDD の同名クラスに相当する。
`ZBDD_Hash` は `ZDD_Hash` の別名である。

```cpp
BDD_Hash(void)
~BDD_Hash(void)
void Clear(void)
void Enter(const BDD& key, void* ptr)
void* Refer(const BDD& key) const
bddword Amount(void) const
```

- `Enter(key, ptr)` はエントリを登録または更新する。**`ptr` に 0 を渡すと
  そのキーの削除を意味する**（`Refer()` が「見つからない」場合に 0 を
  返すため、値としての null ポインタは格納できない）。
  未登録キーの削除は何もしない。
- `Refer(key)` は登録された値を返す。見つからない場合は 0 を返す。
- `Amount()` は登録中のエントリ数を返す。
- `Clear()` は全エントリを解放し、表を初期サイズに戻す。
- 表はエントリのキーへの参照を保持するため、登録中のキーのノードは
  ガベージコレクション（bddgc）で回収されない。削除または `Clear()` で
  参照は解放される。
- コピーおよび代入は禁止されている。
- 記憶確保に失敗した場合は BDDOutOfMemoryException 例外を投げる。
