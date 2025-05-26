#ifndef SBDD_HELPER_TDZDD_H
#define SBDD_HELPER_TDZDD_H

#include "tdzdd/ToBDD_sb.hpp"
#include "tdzdd/ToZBDD_sb.hpp"
#include "tdzdd/SapporoBdd_sb.hpp"
#include "tdzdd/SapporoZdd_sb.hpp"

namespace sapporobdd {

inline BDD to_bdd(const tdzdd::DdStructure<2>& dd)
{
    return dd.evaluate(ToBDD_sb());
}

inline ZDD to_zdd(const tdzdd::DdStructure<2>& dd)
{
    return dd.evaluate(ToZBDD_sb());
}

inline tdzdd::DdStructure<2> to_tdzdd(const BDD& f)
{
    SapporoBdd_sb sbdd(f);
    return tdzdd::DdStructure<2>(sbdd);
}

inline tdzdd::DdStructure<2> to_tdzdd(const ZBDD& f)
{
    SapporoZdd_sb sbdd(f);
    return tdzdd::DdStructure<2>(sbdd);
}

} /* end of namespace sapporobdd */

#endif /* SBDD_HELPER_TDZDD_H */
