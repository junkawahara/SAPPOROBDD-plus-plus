# Known issues

Findings that a code review raised, that are real, and that the code
deliberately still has. Each entry says what the behaviour is, why it was left
alone, and what changing it would cost, so that the decision can be revisited
rather than rediscovered.

Issues that were found and fixed are not listed here; they are in the git
history.

## 1. `ZDD::CardMP16(char*)` cannot check the buffer it writes into

- `include/ZDD.h` (`CardMP16`), `src/BDDc/bddc_operations.cc`
  (`bddcardmp16`), `man/classes/ZDD.md` (CardMP16)

The method takes a `char*` and no length. It writes every digit of the count
and the terminating null unconditionally, so a caller that provides a shorter
buffer gets a heap or stack overrun that neither the library nor the caller
can detect.

**Why it is still there.** A safe way to call it already exists: `CardMP16(0)`
allocates a buffer of exactly the size the result needs and returns it (the
caller frees it). For the `char*` form the required size is documented and
pinned down by a test: 16 words of hexadecimal digits plus the null, i.e. 257
bytes in a 64bit build (the default and `B_EXTEND`) and 129 bytes with `B_32`.
`tests/test_mp_oom.cpp` builds the widest representable cardinality, checks
that it fits in exactly that many bytes and that nothing is written past them,
so the documented number cannot silently become wrong.

**What a fix would cost.** A bounded overload — `char* CardMP16(char* s,
size_t n) const` throwing `BDDOutOfRangeException` when `n` is too small — is
easy to add on top of the allocating form, but it is an addition to the public
API, and making the old form safe would mean deprecating a method that
existing code calls. That is an interface decision, not a defect fix.

## 2. A non-zero shift of a constant ZDD is a range error

- `src/BDDc/bddc_operations.cc` (`bddlshift`, `bddrshift`)

The shift amount is checked against the number of variables in use before the
operand is examined, so `ZDD(0) << 1` and `ZDD(1) >> 1` throw
`BDDOutOfRangeException` when no variable has been declared yet, even though a
constant has no variable to move and the result would be the constant itself.

**Why it is still there.** The C++ operators take an `int` and pass it on as an
unsigned `bddvar`, so a negative shift arrives as a very large positive one.
The range check is what turns `f << -1` into an exception. Testing `B_CST(f)`
first would remove that check for constants: `ZDD(1) << -1` would then quietly
answer `ZDD(1)`, and a caller with a sign bug would never hear about it. The
error that the review objects to and the error that catches real bugs are the
same check.

`shift == 0` is already handled before the range check, because the identity
has to work when no variable exists. The contract — the shift amount is
bounded by the variable count whatever the operand is — is written down in
`man/classes/ZDD.md`.

## 3. `rand() % 100` in `ZDD_Random()` and `BDD_Random()` has a modulo bias

- `src/BDD+/ZDD.cc` (`ZDD_Random`), `src/BDD+/BDD.cc` (`BDD_Random`)

`std::rand()` returns a value in `[0, RAND_MAX]`, and `RAND_MAX + 1` is not a
multiple of 100, so the residues are not uniform and the acceptance
probability is not exactly `density` percent.

**Why it is still there.** With the glibc `RAND_MAX` of 2^31-1 the residues
0..47 occur 21474837 times out of 2^31 and 48..99 occur 21474836 times: a
relative bias of about 5e-8. Rejection sampling would replace a sequence that
callers can reproduce from a seed today with a different one, for an error far
below anything a caller of a randomised diagram generator can observe.

This is not the same as the sampling defect fixed in `BDDCT::AllocRand()`,
where the assembled value could not reach half of the requested range at all.
If the bias ever matters — a platform with `RAND_MAX == 32767` reaches about
0.3% — the fix is rejection sampling, or an API that takes a `<random>` engine
so that the seed and the distribution are the caller's.

## 4. `XPrint()` / `XPrint0()` are declared even in a build without X11

- `include/ZDD.h`, `include/BDD.h`, `src/BDD+/ZDDX11.cc`,
  `src/BDD+/BDDX11.cc`, `src/INSTALL`, `src/INSTALL32`

The install script skips `src/BDDXc` when it cannot find `X11/Xlib.h` and says
so ("XPrint() will not be available"), but the headers declare the drawing
methods unconditionally. Code that calls one of them compiles and fails at
link time with an undefined `bddgraph`/`bddgraph0`.

**Why it is still there.** A feature macro would have to be agreed on by two
builds that do not talk to each other: the library is compiled by
`src/INSTALL`, and the application is compiled later against the installed
headers, which carry no record of how the library was built. Doing this
properly means generating a configuration header at install time and shipping
it with the rest — a change to the build layout, not to the ZDD class. The
alternative of a stub that always exists and throws would need a translation
unit that is compiled in both configurations, which the current makefile does
not have.

The link-time consequence is documented in `man/classes/ZDD.md` under
`XPrint()` and `XPrint0()`.
