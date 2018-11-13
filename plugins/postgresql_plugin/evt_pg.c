/* Processed by ecpg (11.1) */
/* These include files are added by the preprocessor */
#include <ecpglib.h>
#include <ecpgerrno.h>
#include <sqlca.h>
/* End of automatic include section */

#line 1 "evt_pg.pgc"

#line 1 "./include/evt/postgresql_plugin/evt_pg.pgh"
/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <stddef.h>

#define PG_OK   0
#define PG_FAIL 1

void* pg_new_conn(const char* name, const char* target, const char* user, const char* pwd);

int pg_close_conn(const char* name);
int pg_create_db(const char* name);
int pg_drop_db(const char* name);
int pg_exists_db(const char* name, int* /* out */);
int pg_prepare_tables();

/* exec sql begin declare section */

  
     
     
 typedef struct { 
#line 22 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int a ;
 
#line 23 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int b ;
 }  pg_block_t ;

#line 24 "./include/evt/postgresql_plugin/evt_pg.pgh"


  
     
     
 typedef struct { 
#line 27 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int a ;
 
#line 28 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int b ;
 }  pg_trx_t ;

#line 29 "./include/evt/postgresql_plugin/evt_pg.pgh"


  
     
     
 typedef struct { 
#line 32 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int a ;
 
#line 33 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int b ;
 }  pg_action_t ;

#line 34 "./include/evt/postgresql_plugin/evt_pg.pgh"


  
     
     
 typedef struct { 
#line 37 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int a ;
 
#line 38 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int b ;
 }  pg_domain_t ;

#line 39 "./include/evt/postgresql_plugin/evt_pg.pgh"


  
     
     
 typedef struct { 
#line 42 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int a ;
 
#line 43 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int b ;
 }  pg_token_t ;

#line 44 "./include/evt/postgresql_plugin/evt_pg.pgh"


  
     
     
 typedef struct { 
#line 47 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int a ;
 
#line 48 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int b ;
 }  pg_group_t ;

#line 49 "./include/evt/postgresql_plugin/evt_pg.pgh"


  
     
     
 typedef struct { 
#line 52 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int a ;
 
#line 53 "./include/evt/postgresql_plugin/evt_pg.pgh"
 int b ;
 }  pg_fungible_t ;

#line 54 "./include/evt/postgresql_plugin/evt_pg.pgh"


/* exec sql end declare section */
#line 56 "./include/evt/postgresql_plugin/evt_pg.pgh"


int pg_is_table_empty(const char* name, int* /* out */);

int pg_add_block(pg_block_t*);
int pg_get_block(const char* block_id, pg_block_t** /* out */);
int pg_get_latest_block_id(char** /* out */);
int pg_set_block_irreversible(const char* block_id);

int pg_add_trx(pg_trx_t*);
int pg_get_block(pg_trx_t** /* out */);

int pg_add_action(pg_action_t*);
int pg_get_action(pg_action_t** /* out */);

int pg_add_domain(pg_domain_t*);
int pg_get_domain(pg_domain_t** /* out */);

int pg_add_token(pg_token_t*);
int pg_get_token(pg_token_t** /* out */);

int pg_add_group(pg_group_t*);
int pg_get_group(pg_group_t** /* out */);

int pg_add_fungible(pg_fungible_t*);
int pg_get_fungible(pg_fungible_t** /* out */);

#line 1 "evt_pg.pgc"


void*
pg_new_conn(const char* name, const char* target, const char* user, const char* pwd) {
    /* exec sql whenever sqlerror  goto  err ; */
#line 5 "evt_pg.pgc"


    /* exec sql begin declare section */
          
        
          
           
    
#line 8 "evt_pg.pgc"
 const char * name = name ;
 
#line 9 "evt_pg.pgc"
 const char * target = target ;
 
#line 10 "evt_pg.pgc"
 const char * user = user ;
 
#line 11 "evt_pg.pgc"
 const char * pwd = pwd ;
/* exec sql end declare section */
#line 12 "evt_pg.pgc"


    { ECPGconnect(__LINE__, 0, target , user , pwd , name, 0); 
#line 14 "evt_pg.pgc"

if (sqlca.sqlcode < 0) goto err;}
#line 14 "evt_pg.pgc"


err:
    return NULL;
}

int
pg_close_conn(const char* name) {
    /* exec sql whenever sqlerror  goto  err ; */
#line 22 "evt_pg.pgc"


    /* exec sql begin declare section */
        
    
#line 25 "evt_pg.pgc"
 const char * name = name ;
/* exec sql end declare section */
#line 26 "evt_pg.pgc"


    { ECPGdisconnect(__LINE__, name);
#line 28 "evt_pg.pgc"

if (sqlca.sqlcode < 0) goto err;}
#line 28 "evt_pg.pgc"

    return PG_OK;

err:
    return PG_FAIL;
}

int
pg_create_db(const char* name) {
    /* exec sql whenever sqlerror  goto  err ; */
#line 37 "evt_pg.pgc"


    /* exec sql begin declare section */
        
        
    
#line 40 "evt_pg.pgc"
 const char * name = name ;
 
#line 41 "evt_pg.pgc"
 const char * stmt = "CREATE DATABASE ?
                        WITH
                        ENCODING = 'UTF8'
                        LC_COLLATE = 'C'
                        LC_CTYPE = 'C'
                        CONNECTION LIMIT = -1;" ;
/* exec sql end declare section */
#line 47 "evt_pg.pgc"


    { ECPGprepare(__LINE__, NULL, 0, "create_db", stmt);
#line 49 "evt_pg.pgc"

if (sqlca.sqlcode < 0) goto err;}
#line 49 "evt_pg.pgc"

    { ECPGdo(__LINE__, 0, 1, NULL, 0, ECPGst_execute, "create_db", 
	ECPGt_char,&(name),(long)0,(long)1,(1)*sizeof(char), 
	ECPGt_NO_INDICATOR, NULL , 0L, 0L, 0L, ECPGt_EOIT, ECPGt_EORT);
#line 50 "evt_pg.pgc"

if (sqlca.sqlcode < 0) goto err;}
#line 50 "evt_pg.pgc"

    { ECPGdeallocate(__LINE__, 0, NULL, "create_db");
#line 51 "evt_pg.pgc"

if (sqlca.sqlcode < 0) goto err;}
#line 51 "evt_pg.pgc"


err:
    return PG_FAIL;
}

int
pg_drop_db(const char* name) {
    /* exec sql whenever sqlerror  goto  err ; */
#line 59 "evt_pg.pgc"


    /* exec sql begin declare section */
        
        
    
#line 62 "evt_pg.pgc"
 const char * name = name ;
 
#line 63 "evt_pg.pgc"
 const char * stmt = "DROP DATABASE ?;" ;
/* exec sql end declare section */
#line 64 "evt_pg.pgc"


    { ECPGprepare(__LINE__, NULL, 0, "drop_db", stmt);
#line 66 "evt_pg.pgc"

if (sqlca.sqlcode < 0) goto err;}
#line 66 "evt_pg.pgc"

    { ECPGdo(__LINE__, 0, 1, NULL, 0, ECPGst_execute, "drop_db", 
	ECPGt_char,&(name),(long)0,(long)1,(1)*sizeof(char), 
	ECPGt_NO_INDICATOR, NULL , 0L, 0L, 0L, ECPGt_EOIT, ECPGt_EORT);
#line 67 "evt_pg.pgc"

if (sqlca.sqlcode < 0) goto err;}
#line 67 "evt_pg.pgc"

    { ECPGdeallocate(__LINE__, 0, NULL, "drop_db");
#line 68 "evt_pg.pgc"

if (sqlca.sqlcode < 0) goto err;}
#line 68 "evt_pg.pgc"


err:
    return PG_FAIL;
}

int
pg_exists_db(const char* name, int* r /* out */) {
    /* exec sql whenever sqlerror  goto  err ; */
#line 76 "evt_pg.pgc"


    /* exec sql begin declare section */
        
        
            
    
#line 79 "evt_pg.pgc"
 const char * name = name ;
 
#line 80 "evt_pg.pgc"
 const char * stmt = "SELECT EXISTS(
                            SELECT datname
                            FROM pg_catalog.pg_database WHERE datname = ?
                        );" ;
 
#line 84 "evt_pg.pgc"
 bool existed ;
/* exec sql end declare section */
#line 85 "evt_pg.pgc"


    { ECPGprepare(__LINE__, NULL, 0, "exists_db", stmt);
#line 87 "evt_pg.pgc"

if (sqlca.sqlcode < 0) goto err;}
#line 87 "evt_pg.pgc"

    { ECPGdo(__LINE__, 0, 1, NULL, 0, ECPGst_execute, "exists_db", 
	ECPGt_char,&(name),(long)0,(long)1,(1)*sizeof(char), 
	ECPGt_NO_INDICATOR, NULL , 0L, 0L, 0L, ECPGt_EOIT, 
	ECPGt_bool,&(existed),(long)1,(long)1,sizeof(bool), 
	ECPGt_NO_INDICATOR, NULL , 0L, 0L, 0L, ECPGt_EORT);
#line 88 "evt_pg.pgc"

if (sqlca.sqlcode < 0) goto err;}
#line 88 "evt_pg.pgc"

    { ECPGdeallocate(__LINE__, 0, NULL, "exists_db");
#line 89 "evt_pg.pgc"

if (sqlca.sqlcode < 0) goto err;}
#line 89 "evt_pg.pgc"


    *r = existed;

err:
    return PG_FAIL;
}

int
pg_prepare_tables() {
    /* exec sql whenever sqlerror  goto  err ; */
#line 99 "evt_pg.pgc"
\

    { ECPGdo(__LINE__, 0, 1, NULL, 0, ECPGst_normal, "create table if not exists public . blocks ( block_id character ( 64 ) not null , block_num integer not null , prev_block_id character ( 64 ) not null , \"timestamp\" timestamp with time zone not null , trx_merkle_root character ( 64 ) not null , trx_count integer not null , producer character varying ( 21 ) not null , pending boolean not null , created_at timestamp with time zone not null ) partition by range ( ( ( block_num / 1000000 ) ) ) with ( oids = false ) tablespace pg_default", ECPGt_EOIT, ECPGt_EORT);
#line 116 "evt_pg.pgc"

if (sqlca.sqlcode < 0) goto err;}
#line 116 "evt_pg.pgc"

 
             CREATE INDEX IF NOT EXISTS block_id_index
                 ON public.blocks USING btree
                 (block_id)
                 TABLESPACE pg_default;

err:
    return PG_FAIL;
}

