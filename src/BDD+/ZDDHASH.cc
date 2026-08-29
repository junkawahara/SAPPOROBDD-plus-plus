/****************************************
 * ZDD+ Manipulator (SAPPORO-1.82)      *
 * (Hash table methods)                 *
 * (C) Shin-ichi MINATO (Mar. 20, 2017) *
 ****************************************/

#include <new>

#include "ZDD.h"

namespace sapporobdd {

/* Open addressing with linear probing.  Every slot is in one of three
   states: Empty (never used, the sentinel that ends every probe chain),
   Occupied, or Deleted (a tombstone: the slot once held an entry that was
   removed, so a probe chain must run through it, but its key is gone).

   The states used to be encoded in the key and pointer values themselves
   (_key == -1 meant empty, _ptr == 0 meant deleted), which had four
   consequences: a deleted entry kept a reference to its key ZDD, pinning
   the whole key against bddgc() until the slot happened to be reused;
   deleting keys never made a slot empty again, so a workload of inserts
   and deletes of distinct keys filled the table with tombstones and the
   probe loop, whose only exit is an empty slot, ran forever; deleting a
   key that was never entered consumed a slot; and the error value ZDD(-1)
   could not be used as a key at all.  An explicit state byte removes all
   four problems at once. */

ZDD_Hash::ZDD_Hash()
{
  _hashSize = 16;
  _wheel = new(std::nothrow) ZDD_Entry[_hashSize];
  if(_wheel == 0)
    BDDerr("ZDD_Hash::ZDD_Hash: Memory allocation failed.",
           ExceptionType::OutOfMemory);
  _amount = 0;
  _tombstone = 0;
}

ZDD_Hash::~ZDD_Hash() { delete[] _wheel; }

void ZDD_Hash::Clear()
{
  /* The fresh table is allocated before the old one is released, so that a
     failed allocation leaves the table as it was instead of leaving _wheel
     dangling for the destructor to free again. */
  ZDD_Entry* newWheel = new(std::nothrow) ZDD_Entry[16];
  if(newWheel == 0)
    BDDerr("ZDD_Hash::Clear: Memory allocation failed.",
           ExceptionType::OutOfMemory);
  delete[] _wheel;
  _wheel = newWheel;
  _hashSize = 16;
  _amount = 0;
  _tombstone = 0;
}

ZDD_Hash::ZDD_Entry* ZDD_Hash::GetEntry(const ZDD& key) const
/* Returns the entry holding the key if it is in the table; otherwise the
   slot where the key would be inserted (the first tombstone on its probe
   chain, or the empty slot that ends the chain).  The state byte tells the
   caller which of the two it got.  A single pass suffices: the first
   tombstone is remembered while the scan continues to the sentinel. */
{
  bddword id = key.GetID();
  bddword i = (id+(id>>10)+(id>>20)) & (_hashSize - 1);
  ZDD_Entry* tomb = 0;
  while(_wheel[i]._state != ZDD_Entry::Empty)
  {
    if(_wheel[i]._state == ZDD_Entry::Occupied)
    {
      if(_wheel[i]._key.GetID() == id) return & _wheel[i];
    }
    else if(tomb == 0) tomb = & _wheel[i];
    i++;
    i &= (_hashSize - 1);
  }
  return tomb? tomb: & _wheel[i];
}

void ZDD_Hash::Rehash(bddword newSize)
/* Moves the occupied entries into a fresh table of newSize slots, dropping
   every tombstone.  Throws BDDOutOfMemoryException and leaves the table
   unchanged when the new table cannot be allocated; the exit(1) that used
   to sit here (guarding a plain new, which never returns null) has been
   retired with the rest of the library's exit() calls. */
{
  ZDD_Entry* newWheel = new(std::nothrow) ZDD_Entry[newSize];
  if(newWheel == 0)
    BDDerr("ZDD_Hash::Enter: Memory allocation failed.",
           ExceptionType::OutOfMemory);
  for(bddword j=0; j<_hashSize; j++)
  {
    if(_wheel[j]._state != ZDD_Entry::Occupied) continue;
    bddword id = _wheel[j]._key.GetID();
    bddword i = (id+(id>>10)+(id>>20)) & (newSize - 1);
    while(newWheel[i]._state != ZDD_Entry::Empty)
    {
      i++;
      i &= (newSize - 1);
    }
    newWheel[i]._state = ZDD_Entry::Occupied;
    newWheel[i]._key = _wheel[j]._key;
    newWheel[i]._ptr = _wheel[j]._ptr;
  }
  delete[] _wheel;
  _wheel = newWheel;
  _hashSize = newSize;
  _tombstone = 0;
}

void ZDD_Hash::Enter(const ZDD& key, void* ptr)
// ptr = 0 means deleting.
{
  if(ptr == 0)
  {
    ZDD_Entry* ent = GetEntry(key);
    /* Deleting a key that is not in the table is a no-op; it used to write
       the key into an empty slot and consume it forever. */
    if(ent -> _state != ZDD_Entry::Occupied) return;
    ent -> _state = ZDD_Entry::Deleted;
    ent -> _key = ZDD();  /* release the reference, so bddgc() can collect */
    ent -> _ptr = 0;
    _amount--;
    _tombstone++;
    return;
  }

  /* Both the live entries and the tombstones burden the probe chains, so
     both count against the load bound.  Growing (or, when the table is
     mostly tombstones, rebuilding at the same size) before the insertion
     keeps the bound "amount + tombstones < half the table" invariant, which
     is what guarantees that the probe loop in GetEntry() terminates. */
  if(_amount + _tombstone + 1U >= (_hashSize>>1))
  {
    bddword newSize = _hashSize;
    while(_amount >= (newSize>>2)) newSize <<= 1;
    Rehash(newSize);
  }
  ZDD_Entry* ent = GetEntry(key);
  if(ent -> _state == ZDD_Entry::Occupied) { ent -> _ptr = ptr; return; }
  if(ent -> _state == ZDD_Entry::Deleted) _tombstone--;
  ent -> _state = ZDD_Entry::Occupied;
  ent -> _key = key;
  ent -> _ptr = ptr;
  _amount++;
}

void* ZDD_Hash::Refer(const ZDD& key) const
// returns 0 if not found.
{
  ZDD_Entry* ent = GetEntry(key);
  if(ent -> _state != ZDD_Entry::Occupied) return 0;
  return ent -> _ptr;
}

bddword ZDD_Hash::Amount() const { return _amount; }

} // namespace sapporobdd
