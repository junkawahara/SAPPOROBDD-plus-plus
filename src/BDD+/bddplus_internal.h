/*****************************************
*  BDD+ Manipulator  - Internal Header   *
*  (helpers shared by the BDD+ sources)  *
******************************************/

#ifndef BDDPLUS_INTERNAL_H
#define BDDPLUS_INTERNAL_H

#include <cctype>
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

} // namespace sapporobdd

#endif /* BDDPLUS_INTERNAL_H */
