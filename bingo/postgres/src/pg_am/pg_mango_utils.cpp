#include "bingo_postgres.h"
#include "bingo_pg_text.h"
#include "bingo_core_c.h"
#include "bingo_pg_common.h"

CEXPORT {
#include "postgres.h"
#include "fmgr.h"
}

CEXPORT {
PG_FUNCTION_INFO_V1(smiles);
Datum smiles(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(cansmiles);
Datum cansmiles(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(molfile);
Datum molfile(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(cml);
Datum cml(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(checkmolecule);
Datum checkmolecule(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(gross);
Datum gross(PG_FUNCTION_ARGS);
}


Datum smiles(PG_FUNCTION_ARGS) {
   Datum mol_datum = PG_GETARG_DATUM(0);

   BingoPgCommon::BingoSessionHandler bingo_handler(fcinfo->flinfo->fn_oid, false);
   bingo_handler.setFunctionName("smiles");

   BingoPgText mol_text(mol_datum);
   int buf_size;
   const char* mol_buf = mol_text.getText(buf_size);
   char* result = BingoPgCommon::releaseString(mangoSMILES(mol_buf, buf_size, 0));

   if(result == 0 || bingo_handler.error_raised)
      PG_RETURN_NULL();
   
   PG_RETURN_CSTRING(result);
}

Datum cansmiles(PG_FUNCTION_ARGS) {
   Datum mol_datum = PG_GETARG_DATUM(0);

   BingoPgCommon::BingoSessionHandler bingo_handler(fcinfo->flinfo->fn_oid, false);
   bingo_handler.setFunctionName("cansmiles");

   BingoPgText mol_text(mol_datum);
   int buf_size;
   const char* mol_buf = mol_text.getText(buf_size);
   char* result = BingoPgCommon::releaseString(mangoSMILES(mol_buf, buf_size, 1));

   if(result == 0 || bingo_handler.error_raised)
      PG_RETURN_NULL();
   
   PG_RETURN_CSTRING(result);
}

Datum molfile(PG_FUNCTION_ARGS) {
   Datum mol_datum = PG_GETARG_DATUM(0);

   BingoPgCommon::BingoSessionHandler bingo_handler(fcinfo->flinfo->fn_oid, false);
   bingo_handler.setFunctionName("molfile");

   BingoPgText mol_text(mol_datum);
   int buf_size;
   const char* mol_buf = mol_text.getText(buf_size);
   char* result = BingoPgCommon::releaseString(mangoMolfile(mol_buf, buf_size));

   if(result == 0 || bingo_handler.error_raised)
      PG_RETURN_NULL();
   
   PG_RETURN_CSTRING(result);
}

Datum cml(PG_FUNCTION_ARGS) {
   Datum mol_datum = PG_GETARG_DATUM(0);

   BingoPgCommon::BingoSessionHandler bingo_handler(fcinfo->flinfo->fn_oid, false);
   bingo_handler.setFunctionName("cml");

   BingoPgText mol_text(mol_datum);
   int buf_size;
   const char* mol_buf = mol_text.getText(buf_size);
   char* result = BingoPgCommon::releaseString(mangoCML(mol_buf, buf_size));

   if(result == 0 || bingo_handler.error_raised)
      PG_RETURN_NULL();
   
   PG_RETURN_CSTRING(result);
}

Datum checkmolecule(PG_FUNCTION_ARGS) {
   Datum mol_datum = PG_GETARG_DATUM(0);

   BingoPgCommon::BingoSessionHandler bingo_handler(fcinfo->flinfo->fn_oid, false);
   bingo_handler.setFunctionName("checkmolecule");

   BingoPgText mol_text(mol_datum);
   int buf_size;
   const char* mol_buf = mol_text.getText(buf_size);
   
   char* result = BingoPgCommon::releaseString(mangoCheckMolecule(mol_buf, buf_size));

   if(result == 0 || bingo_handler.error_raised)
      PG_RETURN_NULL();

   PG_RETURN_CSTRING(result);
}

Datum gross(PG_FUNCTION_ARGS) {
   Datum mol_datum = PG_GETARG_DATUM(0);

   BingoPgCommon::BingoSessionHandler bingo_handler(fcinfo->flinfo->fn_oid, false);
   bingo_handler.setFunctionName("gross");

   BingoPgText mol_text(mol_datum);
   int buf_size;
   const char* mol_buf = mol_text.getText(buf_size);
   
   char* result = BingoPgCommon::releaseString(mangoGross(mol_buf, buf_size));

   if(result == 0 || bingo_handler.error_raised)
      PG_RETURN_NULL();

   PG_RETURN_CSTRING(result);
}


