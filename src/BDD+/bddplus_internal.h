/*****************************************
*  BDD+ Manipulator  - Internal Header   *
*  (helpers shared by the BDD+ sources)  *
******************************************/

#ifndef BDDPLUS_INTERNAL_H
#define BDDPLUS_INTERNAL_H

#include <cctype>
#include <climits>
#include <cstdio>
#include <string>

namespace sapporobdd {

/* Reads one whitespace-delimited token from strm into s.  Returns EOF when the
   stream ends before a token starts, 0 otherwise.

   The importers read their tokens with fscanf(strm, "%s", s) into a char
   s[256].  Without a field width that smashes the stack on any longer token,
   and a "%255s" would not do either: a PLA product term is a single token as
   long as the input count, so a legitimate file with 256 or more inputs would
   be silently truncated and then rejected as a format error.  Letting the
   string grow removes both failure modes at once. */
inline int ReadToken(FILE *strm, std::string& s)
{
  int c;

  s.erase();
  while((c = fgetc(strm)) != EOF && isspace(c))
    ; /* skip the separators in front of the token */
  if(c == EOF) return EOF;
  do s += (char)c;
  while((c = fgetc(strm)) != EOF && !isspace(c));
  return 0;
}

/* Parses one decimal token read from an imported file.  Stores the value and
   returns 0 when the whole token is a decimal number in [0, limit], and
   returns 1 otherwise (empty token, a sign, junk characters, or too large).

   The importers used to hand the header counts straight to strtol()/strtoll()
   without looking at what came back, so a corrupt or hostile file could make
   them create every variable up to the manager's limit one at a time, keep a
   count that overflowed the int it was stored in, wrap the "hashsize < n_nd<<1"
   computation down to a one-entry table whose linear probing never terminates,
   or ask for an allocation so large that operator new threw std::bad_alloc
   straight past every BDDException handler.  Checking the token against a
   sensible bound up front turns all of those into an ordinary format error. */
inline int ReadDecimal(const std::string& s, unsigned long long limit,
                       unsigned long long& val)
{
  unsigned long long v = 0;

  if(s.empty()) return 1;
  for(std::string::size_type i=0; i<s.size(); i++)
  {
    unsigned long long d;
    if(!isdigit((unsigned char)s[i])) return 1;
    d = (unsigned long long)(s[i] - '0');
    if(v > (ULLONG_MAX - d) / 10ULL) return 1;
    v = v * 10ULL + d;
    if(v > limit) return 1;
  }
  val = v;
  return 0;
}

/* Discards the rest of the current line, up to and including the newline.
   The PLA reader uses it for comments and for directives that take an
   arbitrary number of arguments: both end at the end of their line, not
   after one token. */
inline void SkipLine(FILE *strm)
{
  int c;
  while((c = fgetc(strm)) != EOF && c != '\n')
    ; /* empty */
}

} // namespace sapporobdd

#endif /* BDDPLUS_INTERNAL_H */
