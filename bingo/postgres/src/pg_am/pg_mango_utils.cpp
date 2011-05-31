#include "bingo_postgres.h"
#include "bingo_pg_text.h"
#include "bingo_core_c.h"

CEXPORT {
#include "postgres.h"
#include "fmgr.h"
}

CEXPORT {
PG_FUNCTION_INFO_V1(smiles);
Datum smiles(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(cansmiles);
Datum cansmiles(PG_FUNCTION_ARGS);

}

static void bingoErrorHandler(const char *message, void *context) {
   const char* call_func = (const char*)context;
   elog(ERROR, "error while processing %s: %s", call_func, message);
}

Datum smiles(PG_FUNCTION_ARGS) {
   Datum mol_datum = PG_GETARG_DATUM(0);

   qword session_id = bingoAllocateSessionID();
   bingoSetSessionID(session_id);

   indigo::Array<char> func_name;
   func_name.readString("Molecule SMILES", true);

   bingoSetErrorHandler(bingoErrorHandler, func_name.ptr());

   BingoPgText mol_text(mol_datum);
   int buf_size;
   const char* mol_buf = mol_text.getText(buf_size);
   PG_RETURN_CSTRING(mangoSMILES(mol_buf, buf_size, 0));
}

Datum cansmiles(PG_FUNCTION_ARGS) {
   Datum mol_datum = PG_GETARG_DATUM(0);

   qword session_id = bingoAllocateSessionID();
   bingoSetSessionID(session_id);

   indigo::Array<char> func_name;
   func_name.readString("Molecule canonical SMILES", true);

   bingoSetErrorHandler(bingoErrorHandler, func_name.ptr());

   BingoPgText mol_text(mol_datum);
   int buf_size;
   const char* mol_buf = mol_text.getText(buf_size);
   PG_RETURN_CSTRING(mangoSMILES(mol_buf, buf_size, 1));
}