/****************************************************************************
 * Copyright (C) 2009-2010 GGA Software Services LLC
 * 
 * This file is part of Indigo toolkit.
 * 
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ***************************************************************************/

#include "base_cpp/output.h"
#include "oracle/ringo_shadow_fetch.h"
#include "core/ringo_matchers.h"

#include "oracle/ringo_fetch_context.h"

RingoShadowFetch::RingoShadowFetch (RingoFetchContext &context) :
_context(context)
{
   _total_count = -1;
   _table_name.push(0);

   ArrayOutput output(_table_name);

   output.printf("SHADOW_%d", context.context_id);
   output.writeChar(0);

   _executed = false;
   _fetch_type = 0;
   _processed_rows = 0;
   _end = false;
}

RingoShadowFetch::~RingoShadowFetch ()
{
}

int RingoShadowFetch::getTotalCount (OracleEnv &env)
{
   if (_total_count < 0)
   {
      if (!OracleStatement::executeSingleInt(_total_count, env,
         "SELECT COUNT(*) FROM %s", _table_name.ptr()))
         throw Error("getTotalCount() error");
   }

   return _total_count;
}

int RingoShadowFetch::countOracleBlocks (OracleEnv &env)
{
   int res;

   env.dbgPrintf("countOracleBlocks\n");

   if (!OracleStatement::executeSingleInt(res, env, "select blocks from user_tables where "
              "table_name = upper('%s')", _table_name.ptr()))
      return 0;

   return res;
}

int RingoShadowFetch::getIOCost (OracleEnv &env, float selectivity)
{
   return (int)(countOracleBlocks(env) * selectivity);
}

bool RingoShadowFetch::end ()
{
   return _end;
}

float RingoShadowFetch::calcSelectivity (OracleEnv &env, int total_count)
{
   int nrows_select_total;

   if (_counting_select.size() > 0)
   {
      _counting_select.push(0);
      if (!OracleStatement::executeSingleInt(nrows_select_total, env, _counting_select.ptr()))
         throw Error("selectivity: cannot count rows");
   }
   else
      nrows_select_total = total_count;

   if (_processed_rows == 0)
   {
      if (nrows_select_total == 0)
         return 0;
      throw Error("selectivity: no processed rows");
   }
   
   return (float)nrows_select_total * matched.size() / (total_count * _processed_rows);
}


void RingoShadowFetch::prepareNonSubstructure (OracleEnv &env)
{
   env.dbgPrintf("preparing shadow table for non-substructure match\n");

   _fetch_type = _NON_SUBSTRUCTURE;

   _env.reset(new OracleEnv(env.ctx(), env.logger()));
   _statement.reset(new OracleStatement(_env.ref()));

   _lob_cmf.reset(new OracleLOB(_env.ref()));
   _statement->append("SELECT rid, crf FROM %s", _table_name.ptr());
   _statement->prepare();
   _statement->defineStringByPos(1, _rowid.ptr(), sizeof(_rowid));
   _statement->defineBlobByPos(2, _lob_cmf.ref());

   _counting_select.clear();
}

void RingoShadowFetch::fetch (OracleEnv &env, int maxrows)
{
   matched.clear();

   if (_statement.get() == 0)
      return;

   if (maxrows < 1 || _end)
      return;

   env.dbgPrintf("fetching up to %d rows using shadowtable... ", maxrows);

   while (matched.size() < maxrows)
   {
      bool fetch_res;

      if (!_executed)
      {
         fetch_res = _statement->executeAllowNoData();
         _executed = true;
      }
      else
         fetch_res = _statement->fetch();

      if (!fetch_res)
      {
         _end = true;
         break;
      }

      bool have_match = false;

      if (_fetch_type == _NON_SUBSTRUCTURE)
      {
         RingoSubstructure &instance = _context.substructure;
         QS_DEF(Array<char>, crf);

         _lob_cmf->readAll(crf, false);
         
         if (!instance.matchBinary(crf))
            have_match = true;
      }
      else
         throw Error("unexpected fetch type %d", _fetch_type);

      if (have_match)
         matched.add(_rowid);
      _processed_rows++;
   } 

   env.dbgPrintf("fetched %d\n", matched.size());

   return;
}

