extern "C" {
#include "postgres.h"
#include "fmgr.h"
}

#ifdef qsort
#undef qsort
#endif

#include "bingo_postgres.h"
#include "bingo_pg_common.h"

#include "pg_bingo_context.h"
#include "bingo_core_c.h"
#include "bingo_pg_config.h"
#include "bingo_pg_text.h"


extern "C" {
PG_FUNCTION_INFO_V1(_sub_internal);
PGDLLEXPORT Datum _sub_internal(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_smarts_internal);
PGDLLEXPORT Datum _smarts_internal(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_exact_internal);
PGDLLEXPORT Datum _exact_internal(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(getsimilarity);
PGDLLEXPORT Datum getsimilarity(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_gross_internal);
PGDLLEXPORT Datum _gross_internal(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_sim_internal);
PGDLLEXPORT Datum _sim_internal(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_match_mass_less);
PGDLLEXPORT Datum _match_mass_less(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_match_mass_great);
PGDLLEXPORT Datum _match_mass_great(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_mass_in);
PGDLLEXPORT Datum _mass_in(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_mass_out);
PGDLLEXPORT Datum _mass_out(PG_FUNCTION_ARGS);

}

using namespace indigo;

/*
 * Helper class for searching setup and perfoming
 */
class _MangoContextHandler: public BingoPgCommon::BingoSessionHandler {
public:
   _MangoContextHandler(int type, unsigned int func_oid) : BingoSessionHandler(func_oid, true), _type(type) {
      BingoPgCommon::getSearchTypeString(_type, _typeStr, true);
      setFunctionName(_typeStr.ptr());
   }

   virtual ~_MangoContextHandler() {
   }


   /*
    * Match method
    * Returns true if matching is successfull
    * Throws an error if query can not be loaded
    */
   int matchInternal(Datum query_datum, Datum target_datum, Datum options_datum) {
      BingoPgText query_text(query_datum);
      BingoPgText target_text(target_datum);
      BingoPgText options_text(options_datum);

      /*
       * Set up match parameters
       */
      int res = mangoSetupMatch(_typeStr.ptr(), query_text.getString(), options_text.getString());
      
      if (res < 0)
         throw BingoPgError("Error while bingo%s loading molecule: %s", _typeStr.ptr(), bingoGetError());

      int target_size;
      const char* target_data = target_text.getText(target_size);
      
      QS_DEF(Array<char>, buffer_warn);
      if(_type == BingoPgCommon::MOL_GROSS) {
         buffer_warn.readString(_typeStr.ptr(), true);
         const char* mol_name = bingoGetNameCore(target_data, target_size);
         if(mol_name != 0 && strlen(mol_name) > 0) {
            buffer_warn.appendString(" molecule with name='", true);
            buffer_warn.appendString(mol_name, true);
            buffer_warn.appendString("'", true);
         }
         
         setFunctionName(buffer_warn.ptr());
         raise_error = false;
         target_data = mangoGross(target_data, target_size);
         if(error_raised)
            return -1;
      }
      
      res = mangoMatchTarget(target_data, target_size);

      if (res < 0) {
         buffer_warn.readString(bingoGetWarning(), true);
         const char* mol_name = bingoGetNameCore(target_data, target_size);
         if(mol_name != 0 && strlen(mol_name) > 0)
            elog(WARNING, "warning while bingo%s loading molecule with name ='%s': %s", _typeStr.ptr(), mol_name, buffer_warn.ptr());
         else
            elog(WARNING, "warning while bingo%s loading molecule: %s", _typeStr.ptr(), buffer_warn.ptr());
      }

      return res;
   }
private:
   _MangoContextHandler(const _MangoContextHandler&);//no implicit copy
   int _type;
   indigo::Array<char> _typeStr;
};

Datum _sub_internal(PG_FUNCTION_ARGS) {
   Datum query_datum = PG_GETARG_DATUM(0);
   Datum target_datum = PG_GETARG_DATUM(1);
   Datum options_datum = PG_GETARG_DATUM(2);
   int result = 0;
   PG_BINGO_BEGIN
   {
      _MangoContextHandler bingo_context(BingoPgCommon::MOL_SUB, fcinfo->flinfo->fn_oid);
      result = bingo_context.matchInternal(query_datum, target_datum, options_datum);
      if(result < 0)
         PG_RETURN_NULL();
   }
   PG_BINGO_END

   PG_RETURN_BOOL(result>0);
}

Datum _smarts_internal(PG_FUNCTION_ARGS) {
   Datum query_datum = PG_GETARG_DATUM(0);
   Datum target_datum = PG_GETARG_DATUM(1);
   Datum options_datum = PG_GETARG_DATUM(2);
   int result = 0;
   PG_BINGO_BEGIN
   {
      _MangoContextHandler bingo_context(BingoPgCommon::MOL_SMARTS, fcinfo->flinfo->fn_oid);
      result = bingo_context.matchInternal(query_datum, target_datum, options_datum);
      if(result < 0)
         PG_RETURN_NULL();
   }
   PG_BINGO_END
   PG_RETURN_BOOL(result>0);
}

Datum _exact_internal(PG_FUNCTION_ARGS) {
   Datum query_datum = PG_GETARG_DATUM(0);
   Datum target_datum = PG_GETARG_DATUM(1);
   Datum options_datum = PG_GETARG_DATUM(2);
   int result = 0;
   PG_BINGO_BEGIN
   {
      _MangoContextHandler bingo_context(BingoPgCommon::MOL_EXACT, fcinfo->flinfo->fn_oid);
      result = bingo_context.matchInternal(query_datum, target_datum, options_datum);
      if(result < 0)
         PG_RETURN_NULL();
   }
   PG_BINGO_END
   PG_RETURN_BOOL(result>0);
}

Datum getsimilarity(PG_FUNCTION_ARGS) {
   Datum target_datum = PG_GETARG_DATUM(0);
   Datum query_datum = PG_GETARG_DATUM(1);
   Datum options_datum = PG_GETARG_DATUM(2);

   float res = 0;
   PG_BINGO_BEGIN
   {
      int result = 0;
      _MangoContextHandler bingo_context(BingoPgCommon::MOL_SIM, fcinfo->flinfo->fn_oid);
      result = bingo_context.matchInternal(query_datum, target_datum, options_datum);
      if(result < 0)
         PG_RETURN_NULL();
      
      if (result > 0)
         mangoSimilarityGetScore(&res);
   }
   PG_BINGO_END
   PG_RETURN_FLOAT4(res);
}

Datum _gross_internal(PG_FUNCTION_ARGS) {
   Datum query_sign = PG_GETARG_DATUM(0);
   Datum query_datum = PG_GETARG_DATUM(1);
   Datum target_datum = PG_GETARG_DATUM(2);

   int result = 0;
   PG_BINGO_BEGIN
   {
      BingoPgText query_text(query_datum);
      BingoPgText sign_text(query_sign);
      QS_DEF(indigo::Array<char>, bingo_query);
      bingo_query.readString(sign_text.getString(), false);
      bingo_query.appendString(" ", false);
      bingo_query.appendString(query_text.getString(), false);

      query_text.initFromArray(bingo_query);

      _MangoContextHandler bingo_context(BingoPgCommon::MOL_GROSS, fcinfo->flinfo->fn_oid);

      result = bingo_context.matchInternal(query_text.getDatum(), target_datum, 0);
      
      if(result < 0)
         PG_RETURN_NULL();
   }
   PG_BINGO_END

   PG_RETURN_BOOL(result>0);
}

Datum _sim_internal(PG_FUNCTION_ARGS) {
   float min_bound = PG_GETARG_FLOAT4(0);
   float max_bound = PG_GETARG_FLOAT4(1);
   Datum query_datum = PG_GETARG_DATUM(2);
   Datum target_datum = PG_GETARG_DATUM(3);
   Datum options_datum = PG_GETARG_DATUM(4);
   int result = 0;
   bool res_bool = false;
   PG_BINGO_BEGIN
   {
      _MangoContextHandler bingo_context(BingoPgCommon::MOL_SIM, fcinfo->flinfo->fn_oid);
      float mol_sim = 0;
      result = bingo_context.matchInternal(query_datum, target_datum, options_datum);

      if(result < 0)
         PG_RETURN_NULL();
      
      if (result > 0)
         mangoSimilarityGetScore(&mol_sim);

      res_bool = (mol_sim <= max_bound) && (mol_sim >= min_bound);
   }
   PG_BINGO_END
   PG_RETURN_BOOL(res_bool);
}

Datum _match_mass_less(PG_FUNCTION_ARGS) {
   Datum mol_datum = PG_GETARG_DATUM(0);
   char* mass_datum = PG_GETARG_CSTRING(1);

   bool result = false;
   PG_BINGO_BEGIN
   {
      BingoPgCommon::BingoSessionHandler bingo_handler(fcinfo->flinfo->fn_oid, true);
      bingo_handler.setFunctionName("mass less");

      BufferScanner scanner(mass_datum);
      float usr_mass = scanner.readFloat();

      BingoPgText mol_text(mol_datum);

      float mol_mass = 0;

      int buf_len;
      const char* buf = mol_text.getText(buf_len);

      mangoMass(buf, buf_len, 0, &mol_mass);

      result = mol_mass < usr_mass;
   }
   PG_BINGO_END

   PG_RETURN_BOOL(result);
}

Datum _match_mass_great(PG_FUNCTION_ARGS) {
   Datum mol_datum = PG_GETARG_DATUM(0);
   char* mass_datum = PG_GETARG_CSTRING(1);

   bool result = false;
   PG_BINGO_BEGIN
   {
      BingoPgCommon::BingoSessionHandler bingo_handler(fcinfo->flinfo->fn_oid, true);
      bingo_handler.setFunctionName("mass great");

      BufferScanner scanner(mass_datum);
      float usr_mass = scanner.readFloat();

      BingoPgText mol_text(mol_datum);

      float mol_mass = 0;

      int buf_len;
      const char* buf = mol_text.getText(buf_len);

      mangoMass(buf, buf_len, 0, &mol_mass);

      result = mol_mass > usr_mass;
   }
   PG_BINGO_END

   PG_RETURN_BOOL(result);
}

Datum _mass_in(PG_FUNCTION_ARGS) {
   char *str = PG_GETARG_CSTRING(0);
   int size = strlen(str) + 1;
   char* result = (char*) palloc(size);
   memcpy(result, str, size);
   PG_RETURN_POINTER(result);
}

Datum _mass_out(PG_FUNCTION_ARGS) {
   char *str = PG_GETARG_CSTRING(0);
   int size = strlen(str) + 1;
   char* result = (char*) palloc(size);
   memcpy(result, str, size);
   PG_RETURN_CSTRING(result);
}
