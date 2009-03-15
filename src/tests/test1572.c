/* -*- mode: C; c-basic-offset: 4 -*- */
#ident "Copyright (c) 2007 Tokutek Inc.  All rights reserved."
#include "test.h"

/* Is it feasible to run 4 billion transactions in one test in the regression tests? */
#include <db.h>
#include <sys/stat.h>

static void
four_billion_subtransactions (int do_something_in_children) {
    DB_ENV *env;
    DB *db;
    DB_TXN *xparent;

    system("rm -rf " ENVDIR);
    toku_os_mkdir(ENVDIR, S_IRWXU+S_IRWXG+S_IRWXO);

    int r;
    r = db_env_create(&env, 0);                                           CKERR(r);

    r = env->open(env, ENVDIR, DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_MPOOL|DB_INIT_TXN|DB_CREATE|DB_PRIVATE, S_IRWXU+S_IRWXG+S_IRWXO); CKERR(r);
    r = db_create(&db, env, 0);                                           CKERR(r);

    {
	DB_TXN *txn;
	r=env->txn_begin(env, 0, &txn, 0); assert(r==0);
	r=db->open(db, txn, "foo.db", 0, DB_BTREE, DB_CREATE, S_IRWXU+S_IRWXG+S_IRWXO); CKERR(r);
	r=txn->commit(txn, 0);    assert(r==0);
    }

    r=env->txn_begin(env, 0, &xparent, 0);  CKERR(r);
    long long i;
    //long long const fourbillion = 1ll << 32;
    long long const fourbillion = 500000;
    printf("Doing %d\n", do_something_in_children);
    for (i=0; i < fourbillion + 100; i++) {
	DB_TXN *xchild;
	r=env->txn_begin(env, xparent, &xchild, 0); CKERR(r);
	if (do_something_in_children) {
	    char hello[30], there[30];
	    snprintf(hello, sizeof(hello), "hello%lld", i);
	    snprintf(there, sizeof(there), "there%lld", i);
	    DBT key, val;
	    r=db->put(db, xchild,
		      dbt_init(&key, hello, strlen(hello)+1),
		      dbt_init(&val, there, strlen(there)+1),
		      DB_YESOVERWRITE);
	    CKERR(r);
	}
	r=xchild->commit(xchild, 0); CKERR(r);
	if (i%1000000==0) { printf("."); fflush(stdout); }

    }
    r=xparent->commit(xparent, 0); CKERR(r);

    r=db->close(db, 0); CKERR(r);
    r=env->close(env, 0); CKERR(r);
}

int
test_main (int argc, const char *argv[])
{
    parse_args(argc, argv);
    four_billion_subtransactions(0);
    four_billion_subtransactions(1);
    return 0;
}

