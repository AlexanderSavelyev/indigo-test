#include "bingo_postgres.h"
#include "bingo_pg_common.h"
#include "pg_bingo_context.h"
#include "bingo_core_c.h"
#include "bingo_pg_config.h"
#include "bingo_pg_text.h"
#include "bingo_pg_buffer.h"
#include "bingo_pg_config.h"
#include "base_cpp/scanner.h"


CEXPORT {
#include "postgres.h"
#include "fmgr.h"
#include "utils/relcache.h"
#include "storage/lock.h"
#include "access/heapam.h"
#include "storage/bufmgr.h"
#include "catalog/pg_type.h"
#include "parser/parse_func.h"
#include "catalog/namespace.h"
#include "utils/lsyscache.h"
}

CEXPORT {
PG_FUNCTION_INFO_V1(bingo_test);
Datum bingo_test(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(bingo_test_tid);
Datum bingo_test_tid(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(bingo_test_select);
Datum bingo_test_select(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(bingo_markpos);
Datum bingo_markpos(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(bingo_restrpos);
Datum bingo_restrpos(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(getindexstructurescount);
Datum getindexstructurescount(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(getweight);
Datum getweight(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(getmass);
Datum getmass(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_match_mass_less);
Datum _match_mass_less(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_match_mass_great);
Datum _match_mass_great(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_mass_in);
Datum _mass_in(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_mass_out);
Datum _mass_out(PG_FUNCTION_ARGS);

}

CEXPORT {
#ifdef PG_MODULE_MAGIC
   PG_MODULE_MAGIC;
#endif

}

using namespace indigo;

static void bingoPgMassErrorHandler(const char *message, void *context) {
   elog(ERROR, "error while bingo calculating mass: %s", message);
}

Datum getindexstructurescount(PG_FUNCTION_ARGS){
   Oid relOid = PG_GETARG_OID(0);

   int result = 0;
   Relation rel;

   rel = relation_open(relOid, AccessShareLock);

   BingoPgBuffer meta_buffer;
   meta_buffer.readBuffer(rel, BINGO_METAPAGE, BINGO_PG_READ);
   BingoMetaPage meta_page = BingoPageGetMeta(BufferGetPage(meta_buffer.getBuffer()));

   result = meta_page->n_molecules;

//   elog(INFO, "attrs num = %d", rel->rd_att->natts);

   relation_close(rel, AccessShareLock);


   PG_RETURN_INT32(result);
}

Datum getweight(PG_FUNCTION_ARGS){
   Datum mol_datum = PG_GETARG_DATUM(0);
   Datum options_datum = PG_GETARG_DATUM(1);

   BingoPgText mol_text(mol_datum);
   BingoPgText mol_options(options_datum);

   float result = 0;

   qword session_id = bingoAllocateSessionID();
   bingoSetSessionID(session_id);
   bingoSetContext(0);
   bingoSetErrorHandler(bingoPgMassErrorHandler, 0);

   const char* schema_name = get_namespace_name(get_func_namespace(fcinfo->flinfo->fn_oid));
   
   BingoPgConfig bingo_config;
   bingo_config.readDefaultConfig(schema_name);
   bingo_config.setUpBingoConfiguration();

   int buf_len;
   const char* buf = mol_text.getText(buf_len);

   mangoMass(buf, buf_len, mol_options.getString(), &result);

   bingoReleaseSessionID(session_id);

   PG_RETURN_FLOAT4(result);
}
Datum getmass(PG_FUNCTION_ARGS){
   Datum mol_datum = PG_GETARG_DATUM(0);

   BingoPgText mol_text(mol_datum);

   float result = 0;

   qword session_id = bingoAllocateSessionID();
   bingoSetSessionID(session_id);
   bingoSetContext(0);
   bingoSetErrorHandler(bingoPgMassErrorHandler, 0);

   const char* schema_name = get_namespace_name(get_func_namespace(fcinfo->flinfo->fn_oid));
   
   BingoPgConfig bingo_config;
   bingo_config.readDefaultConfig(schema_name);
   bingo_config.setUpBingoConfiguration();

   int buf_len;
   const char* buf = mol_text.getText(buf_len);

   mangoMass(buf, buf_len, 0, &result);

   bingoReleaseSessionID(session_id);

   PG_RETURN_FLOAT4(result);
}
Datum _match_mass_less(PG_FUNCTION_ARGS){
   Datum mol_datum = PG_GETARG_DATUM(0);
   char* mass_datum = PG_GETARG_CSTRING(1);

   BufferScanner scanner(mass_datum);
   float usr_mass = scanner.readFloat();

   BingoPgText mol_text(mol_datum);

   float mol_mass = 0;

   qword session_id = bingoAllocateSessionID();
   bingoSetSessionID(session_id);
   bingoSetContext(0);
   bingoSetErrorHandler(bingoPgMassErrorHandler, 0);

   const char* schema_name = get_namespace_name(get_func_namespace(fcinfo->flinfo->fn_oid));

   BingoPgConfig bingo_config;
   bingo_config.readDefaultConfig(schema_name);
   bingo_config.setUpBingoConfiguration();

   int buf_len;
   const char* buf = mol_text.getText(buf_len);

   mangoMass(buf, buf_len, 0, &mol_mass);

   bingoReleaseSessionID(session_id);

   bool result = mol_mass < usr_mass;

   PG_RETURN_BOOL(result);
}
Datum _match_mass_great(PG_FUNCTION_ARGS){
   Datum mol_datum = PG_GETARG_DATUM(0);
   char* mass_datum = PG_GETARG_CSTRING(1);

   BufferScanner scanner(mass_datum);
   float usr_mass = scanner.readFloat();

   BingoPgText mol_text(mol_datum);

   float mol_mass = 0;

   qword session_id = bingoAllocateSessionID();
   bingoSetSessionID(session_id);
   bingoSetContext(0);
   bingoSetErrorHandler(bingoPgMassErrorHandler, 0);

   const char* schema_name = get_namespace_name(get_func_namespace(fcinfo->flinfo->fn_oid));

   BingoPgConfig bingo_config;
   bingo_config.readDefaultConfig(schema_name);
   bingo_config.setUpBingoConfiguration();

   int buf_len;
   const char* buf = mol_text.getText(buf_len);

   mangoMass(buf, buf_len, 0, &mol_mass);

   bingoReleaseSessionID(session_id);

   bool result = mol_mass > usr_mass;

   PG_RETURN_BOOL(result);
}

Datum _mass_in(PG_FUNCTION_ARGS) {
   char *str = PG_GETARG_CSTRING(0);
   int size = strlen(str) + 1;
   char* result = (char*)palloc(size);
   memcpy(result, str, size);
   PG_RETURN_POINTER(result);
}

Datum _mass_out(PG_FUNCTION_ARGS) {
   char *str = PG_GETARG_CSTRING(0);
   int size = strlen(str) + 1;
   char* result = (char*)palloc(size);
   memcpy(result, str, size);
   PG_RETURN_CSTRING(result);
}

Datum bingo_test(PG_FUNCTION_ARGS) {
   elog(INFO, "start test function 3");
   PG_RETURN_VOID();
}

Datum bingo_test_tid(PG_FUNCTION_ARGS) {
   elog(INFO, "start test function tid");

   ItemPointer pp = (ItemPointer) palloc0(sizeof(ItemPointerData));

   ItemPointerSet(pp, 1, 2);

   PG_RETURN_POINTER(pp);
}

//static Oid getFunc(const char* name, Array<Oid>& types) {
//   Array<char> fname;
//   fname.readString(name, true);
//   Value* func_name = makeString(fname.ptr());
//
//   List* func_list = list_make1(func_name);
//   Oid func_oid = LookupFuncName(func_list, types.size(), types.ptr(), false);
//
//   if(func_oid == InvalidOid)
//      elog(ERROR, "can not find the function %s", name);
//
//   list_free(func_list);
//   return func_oid;
//}

//Datum bingo_test_select(PG_FUNCTION_ARGS) {
//   elog(INFO, "start test select");
//
//   Array<Oid> func_type;
//   func_type.push(TEXTOID);
//
//   Oid func_begin_oid = getFunc("bingo_test_cur_begin", func_type);
//
//   FmgrInfo f_begin_info;
//   fmgr_info(func_begin_oid, &f_begin_info);
//
//   func_type.clear();
//   func_type.push(REFCURSOROID);
//
//   Oid func_next_oid = getFunc("bingo_test_cur_next", func_type);
//
//   FmgrInfo f_next_info;
//   fmgr_info(func_next_oid, &f_next_info);
//
//   elog(INFO, "func = %d", func_begin_oid);
//
//   BingoPgText test_select;
//   test_select.initFromString("btest_shadow");
//
//   Datum cursor_ref = FunctionCall1(&f_begin_info, PointerGetDatum(test_select.ptr()));
//
//   BingoPgText res_text(cursor_ref);
//   elog(INFO, "res text = %s", res_text.getString());
//
//
//   Datum record;
//   ItemPointer tup;
//   for (int i = 0; i < 5; ++i) {
//      record = FunctionCall1(&f_next_info, cursor_ref);
//      if(record == 0) {
//         elog(INFO, "Rec is null");
//         continue;
//      }
//      tup = (ItemPointer) DatumGetPointer(record);
//      elog(INFO, "block = %d off = %d", ItemPointerGetBlockNumber(tup), ItemPointerGetOffsetNumber(tup));
//   }
//
//   PG_RETURN_VOID();
//}


/*
 * Save current scan position
 */
Datum
bingo_markpos(PG_FUNCTION_ARGS) {
   elog(ERROR, "bingo does not support mark/restore");
   PG_RETURN_VOID();
}

/*
 * Restore scan to last saved position
 */
Datum
bingo_restrpos(PG_FUNCTION_ARGS) {
   elog(ERROR, "bingo does not support mark/restore");
   PG_RETURN_VOID();
}

void
bingo_redo(XLogRecPtr lsn, XLogRecord *record) {
   elog(PANIC, "bingo_redo: unimplemented");
}

void
bingo_desc(StringInfo buf, uint8 xl_info, char *rec) {
}

