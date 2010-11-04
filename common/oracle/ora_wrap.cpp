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

#include <stdio.h>
#include <oci.h>
#include <string.h>
#include <time.h>

// oci.h on Solaris has typedef-ed dword.
// We define it in order to avoid conflict with base_c/defs.h
#define dword unsigned int

#include "oracle/ora_wrap.h"
#include "oracle/ora_logger.h"
#include "base_c/defs.h"

OracleStatement::OracleStatement (OracleEnv &env) : _env(env)
{                                                 
   dvoid *res = 0;

   _statement = 0;

   // can throw here...
   env.callOCI(OCIHandleAlloc(env.envhp(), &res, OCI_HTYPE_STMT, 0, 0));

   // OCI call OK, save result
   _statement = (OCIStmt *)res;
   _query_len = 0;
}

OracleStatement::~OracleStatement ()
{
   if (_statement != 0)
      OCIHandleFree(_statement, OCI_HTYPE_STMT);
}


OracleLOB::OracleLOB (OracleEnv &env) : _env(env)
{
   dvoid *res = 0;

   _lob = 0;
   _delete_tmp = false;
   _delete_desc = false;
   _is_buffered = false;
   _opened = false;

   // can throw here...
   env.callOCI(OCIDescriptorAlloc(env.envhp(), &res, (ub4)OCI_DTYPE_LOB, 0, 0));

   // OCI call OK, save variables
   _lob = (OCILobLocator *)res;
   _delete_desc = true;
}

OracleLOB::OracleLOB (OracleEnv &env, OCILobLocator *lob) : _env(env)
{
   _lob = lob;
   _delete_tmp = false;
   _delete_desc = false;
   _is_buffered = false;
   _flushed = true;
   _opened = false;
   _buffered_read_start_pos = -1;
}

void OracleLOB::createTemporaryBLOB ()
{
   // can throw here...
   _env.callOCI(OCILobCreateTemporary(_env.svchp(), _env.errhp(), _lob,
      (ub2)OCI_DEFAULT, (ub1)OCI_DEFAULT,
      OCI_TEMP_BLOB, TRUE, OCI_DURATION_SESSION));

   // not thrown => temporary LOB created, so set flag that we have to destroy him later
   _delete_tmp = true;
}

void OracleLOB::createTemporaryCLOB ()
{
   // can throw here...
   _env.callOCI(OCILobCreateTemporary(_env.svchp(), _env.errhp(), _lob,
      (ub2)OCI_DEFAULT, (ub1)OCI_DEFAULT,
      OCI_TEMP_CLOB, TRUE, OCI_DURATION_SESSION));

   // not thrown => temporary LOB created, so set flag that we have to destroy him later
   _delete_tmp = true;
}

void OracleLOB::trim (int length)
{
   _env.callOCI(OCILobTrim(_env.svchp(), _env.errhp(), _lob, length));
}

OracleLOB::~OracleLOB ()
{
   if (_delete_desc && _lob != 0)
   {
      if (_is_buffered)
         flush();
      if (_opened)
         _env.callOCI(OCILobClose(_env.svchp(), _env.errhp(), _lob));
      if (_delete_tmp)
         OCILobFreeTemporary(_env.svchp(), _env.errhp(), _lob);
      OCIDescriptorFree(_lob, OCI_DTYPE_LOB);
   }
}

int OracleLOB::getLength ()
{
   ub4 total_length = 0;

   _env.callOCI(OCILobGetLength(_env.svchp(), _env.errhp(), _lob, &total_length));

   return total_length;
}

void OracleLOB::enableBuffering ()
{
   if (!_is_buffered)
   {
      _env.callOCI(OCILobEnableBuffering(_env.svchp(), _env.errhp(), _lob));
      _is_buffered = true;
      _flushed = true;
   }
}

void OracleLOB::disableBuffering ()
{
   if (_is_buffered)
   {
      flush();
      _env.callOCI(OCILobDisableBuffering(_env.svchp(), _env.errhp(), _lob));
      _is_buffered = false;
   }
}

void OracleLOB::readAll (Array<char> &arr, bool add_zero)
{
   int len = getLength();

   arr.clear_resize(len);

   read(0, arr.ptr(), len);

   if (add_zero)
      arr.push(0);
}

void OracleLOB::read (int start, char *buffer, int buffer_size)
{
   ub4 offset = start;

   while (buffer_size > 0)
   {
      ub4 size = buffer_size;
      // ub4 bufl = buffer_size; 
      ub4 bufl = buffer_size + 4; // to avoid bad behavior on Oracle 11 

      sword rc = OCILobRead(_env.svchp(), _env.errhp(), _lob, &size, offset + 1,
         &buffer[offset - start], bufl, 0, 0, 0, SQLCS_IMPLICIT);

      if (rc != OCI_SUCCESS && rc != OCI_NEED_DATA)
         throw OracleError(_env.errhp(), rc, "OCI lob read error", -1);

      if (size == 0)
         throw OracleError(-1, "0 bytes read from LOB");

      offset += size;
      buffer_size -= size;

      if (rc == OCI_SUCCESS)
      {
         if (buffer_size != 0)
            throw OracleError(-1, "read(): got OCI_SUCCESS but size != 0");
         break;
      }
   }
}

void OracleLOB::flush ()
{
   if (!_flushed)
   {
      OCILobFlushBuffer(_env.svchp(), _env.errhp(), _lob, OCI_LOB_BUFFER_NOFREE);
      //_env.callOCI(OCILobFlushBuffer(_env.svchp(), _env.errhp(), _lob, OCI_LOB_BUFFER_NOFREE));
      _flushed = true;
   }
}

void OracleLOB::write (int start, const Array<char> &data)
{
   write(start, data.ptr(), data.size());
}

void OracleLOB::write (int start, const char *buffer, int bytes)
{
   bool was_buffered = _is_buffered;
   while (bytes > 0)
   {
      ub4 n = bytes;
      int tries = 5;
      sword rc = OCI_ERROR;

      while (tries-- > 0)
      {
         rc = OCILobWrite(_env.svchp(), _env.errhp(), _lob, &n, start + 1,
               (void *)buffer, bytes, OCI_ONE_PIECE, 0, 0, 0, SQLCS_IMPLICIT);

         if (rc == OCI_ERROR)
         {
            int oracle_rc = 0;
            text errbuf[512];

            OCIErrorGet(_env.errhp(), 1, NULL, &oracle_rc, errbuf, (ub4)sizeof(errbuf), OCI_HTYPE_ERROR);
            if (oracle_rc == 22280 && _is_buffered)
            {
               disableBuffering();
               continue;
            }
            else
            {
               _env.callOCI(rc);
               break;
            }
         }

         if (rc == OCI_NEED_DATA || rc == OCI_SUCCESS)
            break;

         _env.callOCI(rc);
      }
      if (rc == OCI_ERROR)
         _env.callOCI(rc);

      if (n == 0)
         throw OracleError(-1, "0 bytes written to LOB");

      bytes -= n;
      buffer += n;
      start += n;
   }
   if (was_buffered)
      enableBuffering();
   if (_is_buffered)
      _flushed = false;
}

void OracleLOB::openReadonly ()
{
   _env.callOCI(OCILobOpen(_env.svchp(), _env.errhp(), _lob, OCI_LOB_READONLY));
   _opened = true;
}

void OracleLOB::openReadWrite ()
{
   _env.callOCI(OCILobOpen(_env.svchp(), _env.errhp(), _lob, OCI_LOB_READWRITE));
   _opened = true;
}


OracleEnv::OracleEnv (OCIExtProcContext *ctx, OracleLogger &logger) : _logger(logger)
{
   _envhp = 0;
   _svchp = 0;
   _errhp = 0;
   _ctx = 0;

   // Get the OCI environment, service context, and error handles
   int rc = OCIExtProcGetEnv(ctx, &_envhp, &_svchp, &_errhp);
          
   if (rc != OCI_SUCCESS)
      throw OracleError((OCIError *)0, rc, "Error getting OCI environment", -1);

   _ctx = ctx;
}

OracleEnv::OracleEnv (const char* name, const char* password, const char* base, OracleLogger &logger) : _logger(logger)
{     
   _envhp = 0;
   _svchp = 0;
   _errhp = 0;
   _ctx = 0;
   
   callOCI(OCIEnvCreate( &_envhp, (ub4)OCI_DEFAULT, (dvoid *)0, 
                         0, 0, 0, (size_t)0, (dvoid **)0));
   dvoid *errhp = 0;

   callOCI(OCIHandleAlloc((CONST dvoid *)_envhp, &errhp, OCI_HTYPE_ERROR, (size_t)0, (dvoid **)0));

   _errhp = (OCIError *)errhp;
   
   callOCI(OCILogon(_envhp, _errhp, &_svchp, (const OraText*)name, strlen(name),
                    (const OraText*)password, strlen(password), (const OraText*)base, strlen(base)));
}

OracleEnv::~OracleEnv ()
{
}

void OracleEnv::callOCI (int rc)
{
   if (rc == OCI_SUCCESS || rc == OCI_SUCCESS_WITH_INFO)
      return;

   throw OracleError(_errhp, rc, "OCI call error", -1);
}

void OracleEnv::dbgPrintf (const char *format, ... )
{
   va_list args;
  
   va_start(args, format);
   _logger.dbgPrintfV(format, args);
   va_end(args);
}

void OracleEnv::dbgPrintfTS (const char *format, ... )
{
   va_list args;

   time_t tm = time(NULL);
   const struct tm *lt = localtime(&tm);

   _logger.dbgPrintf("[%02d.%02d.%4d %02d:%02d:%02d] ",
           lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900, lt->tm_hour, lt->tm_min, lt->tm_sec);

   va_start(args, format);
   _logger.dbgPrintfV(format, args);
   va_end(args);
}

void OracleStatement::prepare ()
{
   //_env.dbgPrintf("preparing %.*s\n", _query_len, _query);
   _env.callOCI(OCIStmtPrepare(_statement, _env.errhp(), (text *)_query,
         (ub4)_query_len, (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT));
}

void OracleStatement::defineIntByPos (int pos, int *value)
{
   OCIDefine *defnp = (OCIDefine *) 0;

   _env.callOCI(OCIDefineByPos(_statement, &defnp, _env.errhp(), pos, value,
         (sword)sizeof(int), SQLT_INT, 0, 0, 0, OCI_DEFAULT));
}

void OracleStatement::defineFloatByPos (int pos, float *value)
{
   OCIDefine *defnp = (OCIDefine *) 0;

   _env.callOCI(OCIDefineByPos(_statement, &defnp, _env.errhp(), pos, value,
      (sword)sizeof(float), SQLT_FLT, 0, 0, 0, OCI_DEFAULT));
}

void OracleStatement::defineStringByPos (int pos, char *string, int max_len)
{
   OCIDefine *defnp = (OCIDefine *) 0;

   _indicators[pos] = 0;

   _env.callOCI(OCIDefineByPos(_statement, &defnp, _env.errhp(), pos, (dvoid *)string, max_len, SQLT_STR, &_indicators[pos], 0, 0, OCI_DEFAULT));
}

void OracleStatement::defineRowidByPos (int pos, OracleRowID &rowid)
{
   OCIDefine *defnp = (OCIDefine *) 0;

   _env.callOCI(OCIDefineByPos(_statement, &defnp, _env.errhp(), pos, rowid.getRef(), (sword)sizeof(OCIRowid *), SQLT_RDD, 0, 0, 0, OCI_DEFAULT));
}

void OracleStatement::defineRawByPos (int pos, OracleRaw &raw)
{
   OCIDefine *defnp = (OCIDefine *) 0;

   _env.callOCI(OCIDefineByPos(_statement, &defnp, _env.errhp(), pos, raw.get(), (sword)sizeof(OCIRaw *), SQLT_LVB, 0, 0, 0, OCI_DEFAULT));
}

void OracleStatement::defineBlobByPos (int pos, OracleLOB &lob)
{
   OCIDefine *defnp = (OCIDefine *) 0;

   if (pos >= NELEM(_indicators))
      throw OracleError(-1, "pos too big");

   _indicators[pos] = 0;
   
   _env.callOCI(OCIDefineByPos(_statement, &defnp, _env.errhp(), pos, lob.getRef(), (sword)sizeof(OCILobLocator *), SQLT_BLOB, &_indicators[pos], 0, 0, OCI_DEFAULT));
}

void OracleStatement::defineClobByPos (int pos, OracleLOB &lob)
{
   OCIDefine *defnp = (OCIDefine *) 0;

   if (pos >= NELEM(_indicators))
      throw OracleError(-1, "pos too big");

   _indicators[pos] = 0;

   _env.callOCI(OCIDefineByPos(_statement, &defnp, _env.errhp(), pos, lob.getRef(), (sword)sizeof(OCILobLocator *), SQLT_CLOB, &_indicators[pos], 0, 0, OCI_DEFAULT));
}

void OracleStatement::execute ()
{
   sword rc = OCIStmtExecute(_env.svchp(), _statement, _env.errhp(), 1, 0, NULL, NULL, OCI_DEFAULT);

   _env.callOCI(rc);
}

bool OracleStatement::executeAllowNoData ()
{
   sword rc = OCIStmtExecute(_env.svchp(), _statement, _env.errhp(), 1, 0, NULL, NULL, OCI_DEFAULT);

   if (rc != OCI_NO_DATA)
   {
      if (rc == OCI_ERROR)
      {
         text errbuf[512];
         int code;

         OCIErrorGet(_env.errhp(), 1, NULL, &code, errbuf, (ub4)sizeof(errbuf), OCI_HTYPE_ERROR);
         if (code == 1405) // ORA-01405: fetched column value is NULL
            return false;
      }
      _env.callOCI(rc);
   }

   return (rc == OCI_SUCCESS);
}

bool OracleStatement::fetch ()
{
   sword rc = OCIStmtFetch2(_statement, _env.errhp(), 1, OCI_FETCH_NEXT, 1, OCI_DEFAULT);
   return (rc == OCI_SUCCESS);
}

bool OracleStatement::gotNull (int pos)
{
   return _indicators[pos] == OCI_IND_NULL;
}

void OracleStatement::getRowID (OracleRowID &rowid)
{
   _env.callOCI(OCIAttrGet(_statement, OCI_HTYPE_STMT, rowid.get(), 0,
      OCI_ATTR_ROWID, _env.errhp()));
}

void OracleStatement::bindIntByName (const char *name, int *value)
{
   OCIBind *bndp = (OCIBind *)0;

   _env.callOCI(OCIBindByName(_statement, &bndp, _env.errhp(), (text *)name, -1,
      (dvoid *)value, sizeof(int), SQLT_INT, 0, 0, 0, 0, 0, OCI_DEFAULT));
}

void OracleStatement::bindClobByName (const char *name, OracleLOB &lob)
{
   OCIBind *bndp = (OCIBind *)0;

   _env.callOCI(OCIBindByName(_statement, &bndp, _env.errhp(), (text *)name, -1,
      (dvoid *)lob.getRef(), 0, SQLT_CLOB, 0, 0, 0, 0, 0, OCI_DEFAULT));
}

void OracleStatement::bindBlobByName (const char *name, OracleLOB &lob)
{
   OCIBind *bndp = (OCIBind *)0;

   _env.callOCI(OCIBindByName(_statement, &bndp, _env.errhp(), (text *)name, -1,
      (dvoid *)lob.getRef(), 0, SQLT_BLOB, 0, 0, 0, 0, 0, OCI_DEFAULT));
}

void OracleStatement::bindRawByName (const char *name, OracleRaw &raw)
{
   OCIBind *bndp = (OCIBind *)0;

   _env.callOCI(OCIBindByName(_statement, &bndp, _env.errhp(), (text *)name, -1,
      (dvoid *)raw.get(), 0, SQLT_LVB, 0, 0, 0, 0, 0, OCI_DEFAULT));
}

void OracleStatement::bindStringByName (const char *name, const char *string, int max_len)
{
   OCIBind *bndp = (OCIBind *) 0;

   _env.callOCI(OCIBindByName(_statement, &bndp, _env.errhp(), (text *)name, -1,
      (dvoid *)string, max_len, SQLT_STR, 0, 0, 0, 0, 0, OCI_DEFAULT));
}

void OracleStatement::append_v (const char *format, va_list args)
{
   _query_len += vsnprintf(&_query[_query_len], sizeof(_query) - _query_len, format, args);

   if (_query_len >= (int)sizeof(_query))
      throw OracleError(-2, "query too long");
}

void OracleStatement::append (const char *format, ...)
{
   va_list args;
  
   va_start(args, format);
   append_v(format, args);
   va_end(args);
}

void OracleStatement::executeSingle (OracleEnv &env, const char *format, ...)
{
   OracleStatement statement(env);
   va_list args;
  
   va_start(args, format);
   statement.append_v(format, args);
   statement.prepare();
   statement.execute();
   va_end(args);
}

bool OracleStatement::executeSingleInt (int &result, OracleEnv &env, const char *format, ...)
{
   OracleStatement statement(env);
   va_list args;
  
   result = 0; // to emphasize that the result can change even if we return 'false'

   va_start(args, format);
   statement.append_v(format, args);
   statement.prepare();
   statement.defineIntByPos(1, &result);
   if (!statement.executeAllowNoData())
      return false;
   va_end(args);
   return true;
}

bool OracleStatement::executeSingleFloat (float &result, OracleEnv &env, const char *format, ...)
{
   OracleStatement statement(env);
   va_list args;
  
   result = 0; // to emphasize that the result can change even if we return 'false'

   va_start(args, format);
   statement.append_v(format, args);
   statement.prepare();
   statement.defineFloatByPos(1, &result);
   if (!statement.executeAllowNoData())
      return false;
   va_end(args);
   return true;
}


bool OracleStatement::executeSingleString (Array<char> &result, OracleEnv &env, const char *format, ...)
{
   OracleStatement statement(env);
   va_list args;
  
   // clear to emphasize that the result can change even if we return 'false'
   result.clear_resize(4001); // maximum size of Oracle's VARCHAR2

   va_start(args, format);
   statement.append_v(format, args);
   statement.prepare();
   statement.defineStringByPos(1, result.ptr(), result.size());
   if (!statement.executeAllowNoData())
   {
      result.clear();
      return false;
   }
   va_end(args);

   result.resize(strlen(result.ptr()) + 1);
   return true;
}

bool OracleStatement::executeSingleBlob (Array<char> &result, OracleEnv &env, const char *format, ...)
{
   OracleStatement statement(env);
   va_list args;
  
   result.clear(); // to emphasize that the result can change even if we return 'false'

   va_start(args, format);
   statement.append_v(format, args);
   statement.prepare();

   OracleLOB lob(env);

   statement.defineBlobByPos(1, lob);
   if (!statement.executeAllowNoData())
      return false;

   va_end(args);

   if (statement.gotNull(1)) // null LOB?
      return false;

   lob.readAll(result, false);
   return true;
}

bool OracleStatement::executeSingleClob (Array<char> &result, OracleEnv &env, const char *format, ...)
{
   OracleStatement statement(env);
   va_list args;
  
   result.clear(); // to emphasize that the result can change even if we return 'false'

   va_start(args, format);
   statement.append_v(format, args);
   statement.prepare();

   OracleLOB lob(env);

   statement.defineClobByPos(1, lob);
   if (!statement.executeAllowNoData())
      return false;

   if (statement.gotNull(1)) // null LOB?
      return false;

   va_end(args);
   lob.readAll(result, false);
   return true;
}

OracleRowID::OracleRowID (OracleEnv &env) : _env(env)
{
   _rowid = 0;

   dvoid *rowid = 0;
   env.callOCI(OCIDescriptorAlloc(env.envhp(), &rowid, OCI_DTYPE_ROWID, 0, (dvoid **)0));
   _rowid = (OCIRowid *)rowid;
}

OracleRowID::~OracleRowID ()
{
   OCIDescriptorFree(_rowid, OCI_DTYPE_ROWID);
}

OracleRaw::OracleRaw (OracleEnv &env) : _env(env)
{
   _raw = 0;
}

OracleRaw::~OracleRaw ()
{
}

void OracleRaw::assignBytes (char *buffer, int length)
{
   _env.callOCI(OCIRawAssignBytes(_env.envhp(), _env.errhp(), (ub1 *)buffer, (ub4)length, &_raw));
}

char *OracleRaw::getDataPtr ()
{
   return (char *)OCIRawPtr(_env.envhp(), _raw);
}

void OracleRaw::resize (int new_size)
{
   _env.callOCI(OCIRawResize(_env.envhp(), _env.errhp(), (ub2)new_size, &_raw));
}

OraRowidText::OraRowidText ()
{
   memset(t, 0, sizeof(t));
}

int OraRowidText::length ()
{
   return 18;
}

OCINumber * OracleExtproc::createInt (OracleEnv &env, int value)
{
   OCINumber *result = (OCINumber *)OCIExtProcAllocCallMemory(env.ctx(), sizeof(OCINumber));

   if (result == NULL)
      throw OracleError(-1, "can't allocate memory for number");

   env.callOCI(OCINumberFromInt(env.errhp(), &value, sizeof(int), OCI_NUMBER_SIGNED, result));

   return result;
}

OCINumber * OracleExtproc::createDouble (OracleEnv &env, double value)
{
   OCINumber *result = (OCINumber *)OCIExtProcAllocCallMemory(env.ctx(), sizeof(OCINumber));

   if (result == NULL)
      throw OracleError(-1, "can't allocate memory for number");

   env.callOCI(OCINumberFromReal(env.errhp(), &value, sizeof(value), result));

   return result;
}

int OracleUtil::numberToInt (OracleEnv &env, OCINumber *number)
{
   int res;

   env.callOCI(OCINumberToInt(env.errhp(), number, sizeof(res), OCI_NUMBER_SIGNED, &res));

   return res;
}

float OracleUtil::numberToFloat (OracleEnv &env, OCINumber *number)
{
   double res;

   env.callOCI(OCINumberToReal(env.errhp(), number, sizeof(res), &res));

   return (float)res;
}

double OracleUtil::numberToDouble (OracleEnv &env, OCINumber *number)
{
   double res;

   env.callOCI(OCINumberToReal(env.errhp(), number, sizeof(res), &res));

   return res;
}
