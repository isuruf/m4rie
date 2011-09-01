/**
 * \file test_multiplication.cc
 * \brief Test code for triangular system solving with matrices (TRSM) routines
 *
 * \author Martin Albrecht <martinralbrecht@googlemail.com>
 */

/******************************************************************************
*
*            M4RIE: Linear Algebra over GF(2^e)
*
*    Copyright (C) 2011 Martin Albrecht <martinralbrecht@googlemail.com>
*
*  Distributed under the terms of the GNU General Public License (GEL)
*  version 2 or higher.
*
*    This code is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*  The full text of the GPL is available at:
*
*                  http://www.gnu.org/licenses/
******************************************************************************/

#include "testing.h"
#include <gf2e_cxx/finite_field_givaro.h>

using namespace M4RIE;

mzed_t *random_mzed_t_lower_left(gf2e *ff, rci_t m) {
  const int bitmask = (1<<ff->degree)-1;
  mzed_t *L = random_mzed_t(ff, m, m);
  for(rci_t i=0; i<m; i++) {
    for(rci_t j=i+1; j<m; j++) {
      mzed_write_elem(L, i, j, 0);
    }
    while(mzed_read_elem(L, i, i) == 0) {
      mzed_write_elem(L, i, i, random()&bitmask) ;
    }
  }
  return L;
};

int test_trsm_lower_left(gf2e *ff, rci_t m, rci_t n) {
  int fail_ret = 0;

  mzed_t *L = random_mzed_t_lower_left(ff, m);
  mzed_t *H = mzed_copy(NULL, L);
  mzed_t *B = random_mzed_t(ff, m, n);
  mzed_t *X = mzed_copy(NULL, B);

  mzed_trsm_lower_left(L, X);

  mzed_addmul(B, L, X);

  m4rie_check(mzed_is_zero(B) == 1);
  m4rie_check(mzed_cmp(L,H) == 0);

  mzed_free(L);
  mzed_free(H);
  mzed_free(B);
  mzed_free(X);

  L = random_mzed_t(ff, m, m);
  B = random_mzed_t(ff, m, n);
  X = mzed_copy(NULL, B);

  const int bitmask = (1<<ff->degree)-1;
  for(rci_t i=0; i<m; i++) {
    while(mzed_read_elem(L, i, i) == 0) {
      mzed_write_elem(L, i, i, random()&bitmask) ;
    }
  };
  H = mzed_copy(NULL, L);

  mzed_trsm_lower_left(L, X);
  m4rie_check(mzed_cmp(L,H) == 0);

  for(rci_t i=0; i<m; i++) {
    for(rci_t j=i+1; j<m; j++) {
      mzed_write_elem(L, i, j, 0);
    }
  }
  mzed_addmul(B, L, X);

  m4rie_check(mzed_is_zero(B) == 1);

  mzed_free(L);
  mzed_free(H);
  mzed_free(B);
  mzed_free(X);

  return fail_ret;
}

int test_batch(gf2e *ff, rci_t m, rci_t n) {
  int fail_ret = 0;
  printf("trsm: k: %2d, minpoly: 0x%03x m: %5d, n: %5d ",(int)ff->degree, (unsigned int)ff->minpoly, (int)m,(int)n);

  m4rie_check(test_trsm_lower_left(ff, m, m) == 0); printf("."); fflush(0);

  if (fail_ret == 0)
    printf(" passed\n");
  else
    printf(" FAILED\n");

  return fail_ret;
}

int main(int argc, char **argv) {
  srandom(17);

  gf2e *ff[10];
  int fail_ret = 0;

  for(int k=2; k<=10; k++) {
    FiniteField *F = (FiniteField*)(new GFqDom<int>(2,k));
    ff[k] = gf2e_init_givgfq(F);
    delete F;
  }

  for(int k=2; k<=10; k++) {
    fail_ret += test_batch(ff[k],   1,   1);
    fail_ret += test_batch(ff[k],   1,   2);
    fail_ret += test_batch(ff[k],  11,  12);
    fail_ret += test_batch(ff[k],  21,  22);
    fail_ret += test_batch(ff[k],  13,   2);
    fail_ret += test_batch(ff[k],  32,  33);
    fail_ret += test_batch(ff[k],  63,  64);
    fail_ret += test_batch(ff[k], 127, 128);
    fail_ret += test_batch(ff[k], 200,  20);
  };

  for(int k=2; k<=10; k++) {
    gf2e_free(ff[k]);
  }

  return fail_ret;
}
