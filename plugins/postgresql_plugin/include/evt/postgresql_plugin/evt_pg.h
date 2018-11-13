/* Processed by ecpg (11.1) */

#line 1 "include/evt/postgresql_plugin/evt_db.pgh"
/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <stddef.h>

void* evtdb_new_conn(const char* name, const char* target, const char* user, const char* pwd);

int evtdb_close_conn(const char* name);
int evtdb_prepare_tables(const char*);
int evtdb_test();

/* exec sql begin declare section */

  
     
     
 typedef struct { 
#line 17 "include/evt/postgresql_plugin/evt_db.pgh"
 int a ;
 
#line 18 "include/evt/postgresql_plugin/evt_db.pgh"
 int b ;
 }  tmp_info ;

#line 19 "include/evt/postgresql_plugin/evt_db.pgh"


/* exec sql end declare section */
#line 21 "include/evt/postgresql_plugin/evt_db.pgh"

