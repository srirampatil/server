/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
/*
COPYING CONDITIONS NOTICE:

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation, and provided that the
  following conditions are met:

      * Redistributions of source code must retain this COPYING
        CONDITIONS NOTICE, the COPYRIGHT NOTICE (below), the
        DISCLAIMER (below), the UNIVERSITY PATENT NOTICE (below), the
        PATENT MARKING NOTICE (below), and the PATENT RIGHTS
        GRANT (below).

      * Redistributions in binary form must reproduce this COPYING
        CONDITIONS NOTICE, the COPYRIGHT NOTICE (below), the
        DISCLAIMER (below), the UNIVERSITY PATENT NOTICE (below), the
        PATENT MARKING NOTICE (below), and the PATENT RIGHTS
        GRANT (below) in the documentation and/or other materials
        provided with the distribution.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

COPYRIGHT NOTICE:

  TokuDB, Tokutek Fractal Tree Indexing Library.
  Copyright (C) 2007-2013 Tokutek, Inc.

DISCLAIMER:

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

UNIVERSITY PATENT NOTICE:

  The technology is licensed by the Massachusetts Institute of
  Technology, Rutgers State University of New Jersey, and the Research
  Foundation of State University of New York at Stony Brook under
  United States of America Serial No. 11/760379 and to the patents
  and/or patent applications resulting from it.

PATENT MARKING NOTICE:

  This software is covered by US Patent No. 8,185,551.
  This software is covered by US Patent No. 8,489,638.

PATENT RIGHTS GRANT:

  "THIS IMPLEMENTATION" means the copyrightable works distributed by
  Tokutek as part of the Fractal Tree project.

  "PATENT CLAIMS" means the claims of patents that are owned or
  licensable by Tokutek, both currently or in the future; and that in
  the absence of this license would be infringed by THIS
  IMPLEMENTATION or by using or running THIS IMPLEMENTATION.

  "PATENT CHALLENGE" shall mean a challenge to the validity,
  patentability, enforceability and/or non-infringement of any of the
  PATENT CLAIMS or otherwise opposing any of the PATENT CLAIMS.

  Tokutek hereby grants to you, for the term and geographical scope of
  the PATENT CLAIMS, a non-exclusive, no-charge, royalty-free,
  irrevocable (except as stated in this section) patent license to
  make, have made, use, offer to sell, sell, import, transfer, and
  otherwise run, modify, and propagate the contents of THIS
  IMPLEMENTATION, where such license applies only to the PATENT
  CLAIMS.  This grant does not include claims that would be infringed
  only as a consequence of further modifications of THIS
  IMPLEMENTATION.  If you or your agent or licensee institute or order
  or agree to the institution of patent litigation against any entity
  (including a cross-claim or counterclaim in a lawsuit) alleging that
  THIS IMPLEMENTATION constitutes direct or contributory patent
  infringement, or inducement of patent infringement, then any rights
  granted to you under this License shall terminate as of the date
  such litigation is filed.  If you or your agent or exclusive
  licensee institute or order or agree to the institution of a PATENT
  CHALLENGE, then Tokutek may terminate any rights granted to you
  under this License.
*/

#ident "Copyright (c) 2007-2013 Tokutek Inc.  All rights reserved."
#ident "The technology is licensed by the Massachusetts Institute of Technology, Rutgers State University of New Jersey, and the Research Foundation of State University of New York at Stony Brook under United States of America Serial No. 11/760379 and to the patents and/or patent applications resulting from it."
#include "test.h"

// verify that the DB_RMW flag on cursor create grabs write locks for cursor set operations

static void test_create_rmw(DB_ENV *env, DB *db, int k, uint32_t txn1_flags, uint32_t txn2_flags, int expect_r) {
    int r;

    DB_TXN *txn1 = NULL;
    r = env->txn_begin(env, NULL, &txn1, 0); assert_zero(r);

    DB_TXN *txn2 = NULL;
    r = env->txn_begin(env, NULL, &txn2, 0); assert_zero(r);

    DBC *c1 = NULL;
    r = db->cursor(db, txn1, &c1, txn1_flags); assert_zero(r);

    DBC *c2 = NULL;
    r = db->cursor(db, txn2, &c2, txn2_flags); assert_zero(r);

    DBT key; dbt_init(&key, &k, sizeof k);
    DBT val; memset(&val, 0, sizeof val);
    r = c1->c_get(c1, &key, &val, DB_SET); assert_zero(r);

    r = c2->c_get(c2, &key, &val, DB_SET); assert(r == expect_r);

    r = c1->c_close(c1); assert_zero(r);
    r = c2->c_close(c2); assert_zero(r);

    r = txn1->commit(txn1, 0); assert_zero(r);
    r = txn2->commit(txn2, 0); assert_zero(r);
}

// verify that the DB_RMW flag to the cursor set operations grabs write locks

static void test_set_rmw(DB_ENV *env, DB *db, int k, uint32_t txn1_flags, uint32_t txn2_flags, int expect_r) {
    int r;

    DB_TXN *txn1 = NULL;
    r = env->txn_begin(env, NULL, &txn1, 0); assert_zero(r);

    DB_TXN *txn2 = NULL;
    r = env->txn_begin(env, NULL, &txn2, 0); assert_zero(r);

    DBC *c1 = NULL;
    r = db->cursor(db, txn1, &c1, 0); assert_zero(r);

    DBC *c2 = NULL;
    r = db->cursor(db, txn2, &c2, 0); assert_zero(r);

    DBT key; dbt_init(&key, &k, sizeof k);
    DBT val; memset(&val, 0, sizeof val);
    r = c1->c_get(c1, &key, &val, DB_SET + txn1_flags); assert_zero(r);

    r = c2->c_get(c2, &key, &val, DB_SET + txn2_flags); assert(r == expect_r);

    r = c1->c_close(c1); assert_zero(r);
    r = c2->c_close(c2); assert_zero(r);

    r = txn1->commit(txn1, 0); assert_zero(r);
    r = txn2->commit(txn2, 0); assert_zero(r);
}

int test_main(int argc, char * const argv[]) {
    int r;

    const char *env_dir = TOKU_TEST_FILENAME;
    const char *db_filename = "rmwtest";

    parse_args(argc, argv);

    char rm_cmd[strlen(env_dir) + strlen("rm -rf ") + 1];
    snprintf(rm_cmd, sizeof(rm_cmd), "rm -rf %s", env_dir);
    r = system(rm_cmd); assert_zero(r);

    r = toku_os_mkdir(env_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); assert_zero(r);

    DB_ENV *env = NULL;
    r = db_env_create(&env, 0); assert_zero(r);
    int env_open_flags = DB_CREATE | DB_PRIVATE | DB_INIT_MPOOL | DB_INIT_TXN | DB_INIT_LOCK | DB_INIT_LOG;
    r = env->open(env, env_dir, env_open_flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); assert_zero(r);

    // create the db
    DB *db = NULL;
    r = db_create(&db, env, 0); assert_zero(r);
    DB_TXN *create_txn = NULL;
    r = env->txn_begin(env, NULL, &create_txn, 0); assert_zero(r);
    r = db->open(db, create_txn, db_filename, NULL, DB_BTREE, DB_CREATE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); assert_zero(r);
    r = create_txn->commit(create_txn, 0); assert_zero(r);

    DB_TXN *write_txn = NULL;
    r = env->txn_begin(env, NULL, &write_txn, 0); assert_zero(r);

    int k = htonl(42); int v = 42;
    DBT key; dbt_init(&key, &k, sizeof k);
    DBT val; dbt_init(&val, &v, sizeof v);
    r = db->put(db, write_txn, &key, &val, DB_NOOVERWRITE); assert_zero(r);
    r = write_txn->commit(write_txn, 0); assert_zero(r);

#ifdef BLOCKING_ROW_LOCKS_READS_NOT_SHARED
    test_set_rmw(env, db, k, 0, 0, DB_LOCK_NOTGRANTED);
#else
    test_set_rmw(env, db, k, 0, 0, 0);
#endif
    test_set_rmw(env, db, k, 0, DB_RMW, DB_LOCK_NOTGRANTED);
    test_set_rmw(env, db, k, DB_RMW, 0, DB_LOCK_NOTGRANTED);
    test_set_rmw(env, db, k, DB_RMW, DB_RMW, DB_LOCK_NOTGRANTED);

#ifdef BLOCKING_ROW_LOCKS_READS_NOT_SHARED
    test_create_rmw(env, db, k, 0, 0, DB_LOCK_NOTGRANTED);
#else
    test_create_rmw(env, db, k, 0, 0, 0);
#endif
    test_create_rmw(env, db, k, 0, DB_RMW, DB_LOCK_NOTGRANTED);
    test_create_rmw(env, db, k, DB_RMW, 0, DB_LOCK_NOTGRANTED);
    test_create_rmw(env, db, k, DB_RMW, DB_RMW, DB_LOCK_NOTGRANTED);


    r = db->close(db, 0); assert_zero(r);

    r = env->close(env, 0); assert_zero(r);
    return 0;
}
