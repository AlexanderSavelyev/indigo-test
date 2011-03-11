﻿using System;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Data;
using Microsoft.SqlServer.Server;
using System.Data.SqlTypes;
using System.Data.SqlClient;
using System.Collections;
using System.IO;
using System.Text;
using Microsoft.Win32;
using System.Threading;
using System.Text.RegularExpressions;
using indigo.SqlAttributes;

namespace indigo
{
   public class Bingo
   {
      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString GetVersion (SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);
         return BingoCore.bingoGetVersion();
      }

      private delegate void bingoCallback ();

      private static SqlInt32 _Match (SqlString target, SqlString query, SqlString options,
         SqlString bingo_schema, string search_type,
         bingoCallback prepare_match, bingoCallback process_matched)
      {
         BingoDll.load(bingo_schema);
         using (BingoSession sessions = new BingoSession())
         {
            ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM;

            if (options.Value.Contains("TAU"))
               flags |= ContextFlags.FLAG_TAU_RULES;
            if (search_type == "SIM")
               flags |= ContextFlags.FLAG_FINGERPRINTS;

            using (SqlConnection conn = new SqlConnection("context connection=true"))
            {
               conn.Open();
               prepareContext(conn, bingo_schema.Value, 0, flags);
            }

            int res = BingoCore.mangoSetupMatch(search_type, query.Value, options.Value);
            if (res == -2)
               throw new Exception(BingoCore.bingoGetError());
            if (res == -1)
               return SqlInt32.Null;

            if (prepare_match != null)
               prepare_match();

            res = BingoCore.mangoMatchTarget(target.Value, target.Value.Length);
            if (res == -2)
               throw new Exception(BingoCore.bingoGetError());

            if (res == -1)
            {
               // can not use SqlContext.Pipe from inside the function, 
               // so just returning NULL without printing the error message
               return SqlInt32.Null;
            }

            if (res == 1 && process_matched != null)
               process_matched();

            return new SqlInt32(res);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlInt32 Sub (SqlString target, SqlString query, SqlString options, SqlString bingo_schema)
      {
         return _Match(target, query, options, bingo_schema, "SUB", null, null);
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlInt32 SMARTS (SqlString target, SqlString query, SqlString options, SqlString bingo_schema)
      {
         return _Match(target, query, options, bingo_schema, "SMARTS", null, null);
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlInt32 Exact (SqlString target, SqlString query, SqlString options, SqlString bingo_schema)
      {
         return _Match(target, query, options, bingo_schema, "EXACT", null, null);
      }

      private static string MatchWithHighlighting (SqlString target, SqlString query,
         SqlString parameters, SqlString bingo_schema, string search_type)
      {
         string highlighting = null;

         bingoCallback prepare =
            () =>
            {
               BingoCore.mangoSetHightlightingMode(1);
            };

         bingoCallback handle =
            () =>
            {
               highlighting = BingoCore.mangoGetHightlightedMolecule();
            };

         _Match(target, query, parameters, bingo_schema, search_type, prepare, handle);
         return highlighting;
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString SubHi (SqlString target, SqlString query, SqlString parameters, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);
         return MatchWithHighlighting(target, query, parameters, bingo_schema, "SUB");
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString SMARTSHi (SqlString target, SqlString query, SqlString parameters, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);
         return MatchWithHighlighting(target, query, parameters, bingo_schema, "SMARTS");
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlSingle Sim (SqlString target, SqlString query, SqlString metrics, SqlString bingo_schema)
      {
         SqlSingle score = SqlSingle.Null;
         bingoCallback handler =
            () =>
            {
               Single score_value;
               BingoCore.mangoSimilarityGetScore(out score_value);
               score = score_value;
            };

         _Match(target, query, metrics, bingo_schema, "SIM", null, handler);
         return score;
      }

      private static SqlInt32 _RMatch (SqlString target, SqlString query, SqlString options,
         SqlString bingo_schema, string search_type, bool heed_highlighting, ref string highlighting)
      {
         BingoDll.load(bingo_schema);
         using (BingoSession sessions = new BingoSession())
         {
            ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM;

            if (options.Value.Contains("TAU"))
               flags |= ContextFlags.FLAG_TAU_RULES;

            using (SqlConnection conn = new SqlConnection("context connection=true"))
            {
               conn.Open();
               prepareContext(conn, bingo_schema.Value, 0, flags);
            }

            int res = BingoCore.ringoSetupMatch(search_type, query.Value, options.Value);
            if (res == -2)
               throw new Exception(BingoCore.bingoGetError());
            if (res == -1)
               return SqlInt32.Null;

            if (heed_highlighting)
               BingoCore.ringoSetHightlightingMode(1);

            res = BingoCore.ringoMatchTarget(target.Value, target.Value.Length);
            if (res == -2)
               throw new Exception(BingoCore.bingoGetError());

            if (res == -1)
            {
               // can not use SqlContext.Pipe from inside the function, 
               // so just returning NULL without printing the error message
               return SqlInt32.Null;
            }

            if (res == 1 && heed_highlighting)
               highlighting = BingoCore.ringoGetHightlightedReaction();

            return new SqlInt32(res);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlInt32 RSub (SqlString target, SqlString query, SqlString bingo_schema)
      {
         string highlighting = null;
         return _RMatch(target, query, "", bingo_schema, "RSUB", false, ref highlighting);
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString RSubHi (SqlString target, SqlString query, SqlString bingo_schema)
      {
         string highlighting = null;
         _RMatch(target, query, "", bingo_schema, "RSUB", true, ref highlighting);
         return highlighting;
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString AAM (SqlString reaction, SqlString options, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);
         ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM;
         using (SqlConnection conn = new SqlConnection("context connection=true"))
         {
            conn.Open();
            prepareContext(conn, bingo_schema.Value, 0, flags);
         }
         return new SqlString(BingoCore.ringoAAM(reaction.Value, options.Value));
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString CheckReaction (SqlString reaction, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);
         ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM;
         using (SqlConnection conn = new SqlConnection("context connection=true"))
         {
            conn.Open();
            prepareContext(conn, bingo_schema.Value, 0, flags);
         }
         string res = BingoCore.checkReaction(reaction.Value);
         if (res == null)
            return SqlString.Null;
         return new SqlString(res);
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString CheckMolecule (SqlString molecule, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);
         ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM;
         using (SqlConnection conn = new SqlConnection("context connection=true"))
         {
            conn.Open();
            prepareContext(conn, bingo_schema.Value, 0, flags);
         }
         string res = BingoCore.checkMolecule(molecule.Value);
         if (res == null)
            return SqlString.Null;
         return new SqlString(res);
      }

      private delegate void bingoOperationDelegate
         (SqlConnection ctx_conn, SqlConnection conn, BingoIndexData index_data);
      private delegate BingoIndexData bingoGetIndexDataDelegate
         (SqlConnection ctx_conn, SqlConnection conn, SqlString bingo_schema);

      [Flags]
      enum BingoOp
      {
         NEED_STAT = 0x01,
         DROP_ON_EXCEPTION = 0x02,
         LOAD_TAU_RULES = 0x04,
         LOAD_CMF = 0x08,
         NON_CONTEXT_CONN = 0x10,
         LOCK_INDEX = 0x20
      }

      // Method for executing abstract operation with BingoIndexData
      private static void _ExecuteBingoOperation(SqlString table_name,
         SqlString bingo_schema, bingoOperationDelegate operationDelegate,
         bingoGetIndexDataDelegate getBingoDataDelegate, BingoOp op_flags)
      {
         BingoDll.load(bingo_schema);

         SqlConnection ext_conn = null;
         try
         {
            using (SqlConnection ctx_conn = new SqlConnection("context connection=true"))
            {
               ctx_conn.Open();

               SqlConnection conn = ctx_conn;
               if ((op_flags & BingoOp.NON_CONTEXT_CONN) != 0)
               {
                  ext_conn = new SqlConnection("server=" + getServername(ctx_conn) +
                     ";integrated security=true;database=" + ctx_conn.Database);
                  ext_conn.Open();
                  conn = ext_conn;
               }

               using (BingoSession session = new BingoSession())
               {
                  BingoCore.bingoProfilingReset(false);

                  BingoTimer timer = new BingoTimer("total");

                  BingoIndexData index_data = getBingoDataDelegate(ctx_conn, conn, bingo_schema);
                  if (index_data.locked)
                  {
                     BingoLog.logMessage("Attempt to get locked index for the tabel {0}", table_name.Value);
                     throw new Exception("MoleculeIndex for the table '" + table_name.Value + "' is locked");
                  }
                  if ((op_flags & BingoOp.LOCK_INDEX) != 0)
                     index_data.locked = true;

                  ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM
                                                                  | ContextFlags.FLAG_FINGERPRINTS;
                  if ((op_flags & BingoOp.LOAD_TAU_RULES) != 0)
                     flags |= ContextFlags.FLAG_TAU_RULES;
                  if ((op_flags & BingoOp.LOAD_CMF) != 0)
                     flags |= ContextFlags.FLAG_CMF;

                  prepareContext(conn, bingo_schema.Value, index_data.id.object_id, flags);
                  index_data.syncContextParameters(conn, bingo_schema.Value);

                  try
                  {
                     operationDelegate(ctx_conn, conn, index_data);
                  }
                  catch (Exception ex)
                  {
                     BingoLog.logMessage("Exception {0} in {1}:\n{2}", ex.Message, ex.Source, ex.StackTrace);

                     if ((op_flags & BingoOp.LOCK_INDEX) != 0)
                        index_data.locked = false;

                     if ((op_flags & BingoOp.DROP_ON_EXCEPTION) != 0)
                        BingoIndexData.DropIndexData(conn, bingo_schema.Value, table_name.Value);

                     throw;
                  }

                  if ((op_flags & BingoOp.LOCK_INDEX) != 0)
                     index_data.locked = false;

                  timer.end();

                  if ((op_flags & BingoOp.NEED_STAT) != 0)
                     BingoLog.logMessage("Statistics for table {0}:\n{1}\n",
                        table_name.Value,
                        BingoCore.bingoProfilingGetStatistics(false));
               }
            }
         }
         finally
         {
            if (ext_conn != null)
               ext_conn.Close();
         }
      }

      // Method for executing abstract operation with BingoIndexData
      private static void _ExecuteBingoOperationChangeIndex (SqlString table_name,
         SqlString bingo_schema, bingoOperationDelegate operationDelegate,
         bingoGetIndexDataDelegate getBingoDataDelegate, BingoOp flags)
      {
         bingoOperationDelegate opWithIndex =
            (ctx_conn, conn, index_data) =>
            {
               int ret;
               if (index_data.getIndexType() == BingoIndexData.IndexType.Molecule)
                  ret = BingoCore.mangoIndexBegin();
               else
                  ret = BingoCore.ringoIndexBegin();
               if (ret != 1)
                  throw new Exception(BingoCore.bingoGetError());

               try
               {
                  index_data.fingerprints.init(conn);
                  index_data.storage.validate(conn);

                  operationDelegate(ctx_conn, conn, index_data);

                  ContextFlags save_flags = ContextFlags.FLAG_CMF;
                  saveContext(conn, bingo_schema.Value, index_data.id.object_id, save_flags);
               }
               finally
               {
                  if (index_data.getIndexType() == BingoIndexData.IndexType.Molecule)
                     BingoCore.mangoIndexEnd();
                  else
                     BingoCore.ringoIndexEnd();
               }
            };

         _ExecuteBingoOperation(table_name, bingo_schema,
            opWithIndex, getBingoDataDelegate, flags);
      }


      private static void CreateIndex (SqlString table, SqlString id_column, 
         SqlString data_column, SqlString bingo_schema, bool reaction)
      {
         bingoOperationDelegate opWithIndex =
            (ctx_conn, conn, data) =>
            {
               // Process each molecule
               using (BingoSqlCursor cursor = new BingoSqlCursor(conn,
                  "SELECT {0}, {1} FROM {2}", data.id_column, data.data_column, table))
               {
                  int counter = 0;
                  while (cursor.read())
                  {
                     counter++;

                     if (cursor[0] == DBNull.Value)
                     {
                        string message =
                           String.Format("Record with id=null was skipped.");
                        SqlContext.Pipe.Send(message);
                        BingoLog.logMessage(message);
                        continue;
                     }

                     int id = Convert.ToInt32(cursor[0]);
                     if (counter % 1000 == 0)
                     {
                        BingoLog.logMessage("Preparing record #{0} with id = {1}",
                           counter, id);
                     }

                     try
                     {
                        if (cursor[1] == DBNull.Value)
                        {
                           string message =
                              String.Format("Record with id={0} has null data. Skipped.", id);
                           SqlContext.Pipe.Send(message);
                           BingoLog.logMessage(message);
                           continue;
                        }
                        string record_data = (string)cursor[1];

                        BingoTimer add_timer = new BingoTimer("index.add_to_index");
                        if (data.getIndexType() == BingoIndexData.IndexType.Molecule)
                           _AddMoleculeToIndex(conn, data, id, record_data);
                        else
                           _AddReactionToIndex(conn, data, id, record_data);
                        add_timer.end();
                     }
                     catch (Exception)
                     {
                        BingoLog.logMessage("Failed on id = {0}", id);
                        throw;
                     }
                  }
               }

               data.flush(conn);

               BingoTimer indices_timer = new BingoTimer("index.create_indices");
               data.createIndices(conn);
               indices_timer.end();

               BingoTimer fp_indices_timer = new BingoTimer("index.create_fp_indices");
               data.fingerprints.createIndices(conn);
               fp_indices_timer.end();
            };


         bingoGetIndexDataDelegate createDataDelegate =
            (ctx_conn, conn, schema) => BingoIndexData.CreateIndexData(getSPID(ctx_conn),
                     conn, schema.Value, table.Value, id_column.Value, data_column.Value, reaction);

         _ExecuteBingoOperationChangeIndex(table, bingo_schema, opWithIndex,
            createDataDelegate, BingoOp.DROP_ON_EXCEPTION | BingoOp.NEED_STAT |
            BingoOp.NON_CONTEXT_CONN | BingoOp.LOCK_INDEX);
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static void CreateMoleculeIndex (SqlString table, SqlString id_column, 
         SqlString data_column, SqlString bingo_schema)
      {
         CreateIndex(table, id_column, data_column, bingo_schema, false);
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static void SetKeepCache (SqlString table, SqlBoolean value, SqlString bingo_schema)
      {
         BingoLog.logMessage("SetKeepCache for {0} table", table.Value);

         bingoGetIndexDataDelegate getDataDelegate =
            (ctx_conn, conn, schema) =>
               BingoIndexData.GetIndexData(conn, bingo_schema.Value, table.Value, getSPID(ctx_conn));

         bingoOperationDelegate setOperation =
            (ctx_conn, conn, index_data) =>
            {
               index_data.setKeepCache(conn, bingo_schema.Value, value.Value);
            };


         _ExecuteBingoOperation(table, bingo_schema, setOperation, getDataDelegate, 0);
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static void CreateReactionIndex (SqlString table, SqlString id_column, 
         SqlString data_column, SqlString bingo_schema)
      {
         CreateIndex(table, id_column, data_column, bingo_schema, true);
      }

      private static bool _AddMoleculeToIndex (SqlConnection conn, BingoIndexData bingo_data, 
         int id, string molfile)
      {
         MangoIndexData data = (MangoIndexData)bingo_data;

         MangoIndex index = new MangoIndex();
         if (!index.prepare(molfile))
         {
            string message =
               String.Format("Molecule with ID={0} wasn't added to the index: {1}",
                  id, BingoCore.bingoGetWarning());
            SqlContext.Pipe.Send(message);
            BingoLog.logMessage(message);
            return false;
         }
         // 4 bytes for Id, 2 bytes for similarity fingerprints bits counts
         byte[] new_data = new byte[index.cmf.Length + 4 + 2];
         index.cmf.CopyTo(new_data, 6);

         MemoryStream stream = new MemoryStream(new_data, 0, 6, true);
         BinaryWriter writer = new BinaryWriter(stream);
         writer.Write(id);
         writer.Write((short)index.sim_fp_bits_count);

         int storage_id = data.storage.add(new_data, conn);
         data.fingerprints.addFingerprint(conn, index.fingerprint, storage_id);

         data.addToShadowTable(conn, index, id, storage_id);
         return true;
      }

      private static bool _AddReactionToIndex (SqlConnection conn, BingoIndexData bingo_data, 
         int id, string reaction)
      {
         RingoIndexData data = (RingoIndexData)bingo_data;

         RingoIndex index = new RingoIndex();
         if (!index.prepare(reaction))
         {
            string message =
               String.Format("Reaction with ID={0} wasn't added to the index: {1}",
                  id, BingoCore.bingoGetWarning());
            SqlContext.Pipe.Send(message);
            BingoLog.logMessage(message);
            return false;
         }

         byte[] new_data = new byte[index.crf.Length + 4];
         index.crf.CopyTo(new_data, 4);

         MemoryStream stream = new MemoryStream(new_data, 0, 4, true);
         BinaryWriter writer = new BinaryWriter(stream);
         writer.Write(id);

         int storage_id = data.storage.add(new_data, conn);
         data.fingerprints.addFingerprint(conn, index.fingerprint, storage_id);

         data.addToShadowTable(conn, index, id, storage_id);
         return false;
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static void DropIndex (SqlString table, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (SqlConnection conn = new SqlConnection("context connection=true"))
         {
            conn.Open();        
            BingoIndexData.DropIndexData(conn, bingo_schema.Value, table.Value);
         }
      }

      public static void FillRowInt (Object obj, out SqlInt32 id)
      {
         FetchedData data = (FetchedData)obj;
         id = new SqlInt32((int)data.id);
      }

      public static void FillRowIntString (Object obj, out SqlInt32 id, out SqlString str)
      {
         FetchedData data = (FetchedData)obj;
         id = new SqlInt32(data.id);
         str = new SqlString(data.subhi);
      }
      public static void FillRowIntFloat (Object obj, out SqlInt32 id, out SqlSingle value)
      {
         FetchedData data = (FetchedData)obj;
         id = new SqlInt32(data.id);
         value = new SqlSingle(data.value);
      }

      private static Dictionary<string, string> ParseOptions (ref string options)
      {
         string specific_options = null;
         Dictionary<string, string> result = new Dictionary<string,string>();

         string[] allowed_parameters = { "TOP", "NEXT", "START" };

         foreach (string part in options.Split(';'))
         {
            string part_trimmed = part.TrimStart();

            bool found = false;
            foreach (string p in allowed_parameters)
               if (part_trimmed.StartsWith(p, StringComparison.OrdinalIgnoreCase))
               {
                  if (result.ContainsKey(p))
                     throw new Exception(String.Format("Only one specification of '{0}' option is allowed", p));
                  result.Add(p, part_trimmed.Substring(p.Length).Trim());
                  found = true;
                  break;
               }
            if (!found)
            {
               if (specific_options != null)
                  throw new Exception(String.Format("Search-specific option was specified twice: '{0}' and '{1}'",
                     specific_options, part_trimmed));

               specific_options = part_trimmed;
            }
         }

         options = specific_options;
         return result;
      }

      private static IEnumerable _MakeSearch (SqlString query, SqlString table,
                 SqlString options,    SqlString bingo_schema, string search_type,
                  bool highlighting, params object[] ext_options)
      {
         BingoLog.logMessage("{0} query started for {1} table", search_type, table.Value);

         ArrayList res_list = new ArrayList();

         BingoOp op_flags = BingoOp.NEED_STAT | BingoOp.LOAD_CMF;
         if (search_type != "SIM")
         {
            if (options.Value.Contains("TAU"))
               op_flags |= BingoOp.LOAD_TAU_RULES;
         }

         bingoGetIndexDataDelegate getDataDelegate = 
            (ctx_conn, conn, schema) => 
               BingoIndexData.GetIndexData(conn, bingo_schema.Value, table.Value, getSPID(ctx_conn));

         bingoOperationDelegate searchOperation =
            (ctx_conn, conn, index_data) =>
            {
               if (index_data.needFlush())
                  throw new Exception("Index was changed. FlushOperations must be called before search");

               string options_str = options.Value;

               Dictionary<string, string> common_options = ParseOptions(ref options_str);

               int max_count = -1;
               if (common_options.ContainsKey("TOP"))
               {
                  max_count = Convert.ToInt32(common_options["TOP"]);
                  if (max_count < 0)
                     throw new Exception("Limit for 'TOP' option cannot be negative");
               }

               int? next_from = null;
               if (common_options.ContainsKey("NEXT"))
                  next_from = Convert.ToInt32(common_options["NEXT"]);
               if (common_options.ContainsKey("START"))
                  next_from = -1;

               IEnumerable<FetchedData> fetched;
               fetched = _Fetch(query, search_type, highlighting, ext_options,
                  conn, index_data, options_str, next_from);

               foreach (FetchedData id in fetched)
               {
                  res_list.Add(id);
                  if (res_list.Count == max_count)
                     break;
               }
            };


         _ExecuteBingoOperation(table, bingo_schema, searchOperation, getDataDelegate, op_flags);

         return res_list;
      }

      private static IEnumerable<FetchedData> _Fetch (SqlString query, string search_type,
         bool highlighting, object[] ext_options, SqlConnection conn,
         BingoIndexData index_data, string options_str, int? id_next_from)
      {
         int? storage_id_next_from = null;
         if (id_next_from.HasValue)
         {
            // -1 means start of the iterations
            if (id_next_from.Value != -1)
            {
               storage_id_next_from = index_data.getStorageIdById(conn, id_next_from.Value);
               if (!storage_id_next_from.HasValue)
                  throw new Exception(String.Format("Cannot find record with id={0}", id_next_from));
            }
            else
               storage_id_next_from = -1;
         }

         IEnumerable<FetchedData> fetched;
         if (search_type == "SUB" || search_type == "SMARTS")
         {
            MangoFastIndexFetch fetch_sub = new MangoFastIndexFetch((MangoIndexData)index_data);
            fetch_sub.prepareSub(query.Value, options_str, highlighting, search_type == "SMARTS");
            fetch_sub.nextAfterStorageId = storage_id_next_from;
            fetched = fetch_sub.fetch(conn);
         }
         else if (search_type == "EXACT")
         {
            MangoShadowFetch fetch_exact = new MangoShadowFetch((MangoIndexData)index_data);
            fetch_exact.prepareExact(query.Value, options_str);
            fetch_exact.nextAfterStorageId = storage_id_next_from;
            fetched = fetch_exact.fetch(conn);
         }
         else if (search_type == "SIM")
         {
            MangoFastIndexFetch fetch_sim = new MangoFastIndexFetch((MangoIndexData)index_data);
            float min = (float)ext_options[0];
            float max = (float)ext_options[1];

            fetch_sim.prepareSimilarity(query.Value, options_str, min, max);
            fetch_sim.nextAfterStorageId = storage_id_next_from;
            fetched = fetch_sim.fetch(conn);
         }
         else if (search_type == "MASS")
         {
            MangoShadowFetch fetch_mass = new MangoShadowFetch((MangoIndexData)index_data);
            float? min = (float?)ext_options[0];
            float? max = (float?)ext_options[1];

            fetch_mass.prepareMass(min, max);
            fetch_mass.nextAfterStorageId = storage_id_next_from;
            fetched = fetch_mass.fetch(conn);
         }
         else if (search_type == "GROSS")
         {
            MangoShadowFetch fetch_gross = new MangoShadowFetch((MangoIndexData)index_data);
            fetch_gross.prepareGross(query.Value);
            fetch_gross.nextAfterStorageId = storage_id_next_from;
            fetched = fetch_gross.fetch(conn);
         }
         else if (search_type == "RSUB")
         {
            RingoFastIndexFetch fetch_sub = new RingoFastIndexFetch((RingoIndexData)index_data);
            fetch_sub.prepareSub(query.Value, options_str, highlighting);
            fetch_sub.nextAfterStorageId = storage_id_next_from;
            fetched = fetch_sub.fetch(conn);
         }
         else
            throw new Exception("Unknown search type: " + search_type);
         return fetched;
      }

      [SqlFunction(FillRowMethodName = "FillRowInt",
         DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read,
         TableDefinition = "id int")]
      [BingoSqlFunctionForReader]
      public static IEnumerable SearchSub (SqlString table, SqlString query,
                 SqlString options, SqlString bingo_schema)
      {                  
         return _MakeSearch(query, table, options, bingo_schema, "SUB", false);
      }

      [SqlFunction(FillRowMethodName = "FillRowIntString",
         DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read,
         TableDefinition = "id int, highlighting nvarchar(max)")]
      [BingoSqlFunctionForReader]
      public static IEnumerable SearchSubHi (SqlString table, SqlString query,
                 SqlString options, SqlString bingo_schema)
      {
         return _MakeSearch(query, table, options, bingo_schema, "SUB", true);
      }

      [SqlFunction(FillRowMethodName = "FillRowInt",
         DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read,
         TableDefinition = "id int")]
      [BingoSqlFunctionForReader]
      public static IEnumerable SearchSMARTS (SqlString table, SqlString query,
                 SqlString options, SqlString bingo_schema)
      {
         return _MakeSearch(query, table, options, bingo_schema, "SMARTS", false);
      }

      [SqlFunction(FillRowMethodName = "FillRowIntString",
         DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read,
         TableDefinition = "id int, highlighting nvarchar(max)")]
      [BingoSqlFunctionForReader]
      public static IEnumerable SearchSMARTSHi (SqlString table, SqlString query,
                 SqlString options, SqlString bingo_schema)
      {
         return _MakeSearch(query, table, options, bingo_schema, "SMARTS", true);
      }

      [SqlFunction(FillRowMethodName = "FillRowInt",
         DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read,
         TableDefinition = "id int")]
      [BingoSqlFunctionForReader]
      public static IEnumerable SearchRSub (SqlString table, SqlString query,
                 SqlString options, SqlString bingo_schema)
      {
         return _MakeSearch(query, table, options, bingo_schema, "RSUB", false);
      }

      [SqlFunction(FillRowMethodName = "FillRowIntString",
         DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read,
         TableDefinition = "id int, highlighting nvarchar(max)")]
      [BingoSqlFunctionForReader]
      public static IEnumerable SearchRSubHi (SqlString table, SqlString query,
                 SqlString options, SqlString bingo_schema)
      {
         return _MakeSearch(query, table, options, bingo_schema, "RSUB", true);
      }

      [SqlFunction(FillRowMethodName = "FillRowInt",
         DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read,
         TableDefinition = "id int")]
      [BingoSqlFunctionForReader]
      public static IEnumerable SearchExact (SqlString table, SqlString query,
                 SqlString options, SqlString bingo_schema)
      {
         return _MakeSearch(query, table, options, bingo_schema, "EXACT", false);
      }

      [SqlFunction(FillRowMethodName = "FillRowInt",
         DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read,
         TableDefinition = "id int")]
      [BingoSqlFunctionForReader]
      public static IEnumerable SearchGross (SqlString table, SqlString query,
                 SqlString options, SqlString bingo_schema)
      {
         return _MakeSearch(query, table, options, bingo_schema, "GROSS", false);
      }

      [SqlFunction(FillRowMethodName = "FillRowIntFloat",
         DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read,
         TableDefinition = "id int, similarity real")]
      [BingoSqlFunctionForReader]
      public static IEnumerable SearchSim (SqlString table, SqlString query,
                 SqlString metric, SqlString bingo_schema, SqlDouble min_bound, SqlDouble max_bound)
      {
         float min = -0.1f, max = 1.1f;
         if (!min_bound.IsNull)
            min = (float)min_bound.Value;
         if (!max_bound.IsNull)
            max = (float)max_bound.Value;

         return _MakeSearch(query, table, metric, bingo_schema, "SIM", false, min, max);
      }

      [SqlFunction(FillRowMethodName = "FillRowIntFloat",
         DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read,
         TableDefinition = "id int, weight real")]
      [BingoSqlFunctionForReader]
      public static IEnumerable SearchMolecularWeight (SqlString table, SqlDouble min_bound,
         SqlDouble max_bound, SqlString options, SqlString bingo_schema)
      {
         float? min = null, max = null;
         if (!min_bound.IsNull)
            min = (float)min_bound.Value;
         if (!max_bound.IsNull)
            max = (float)max_bound.Value;

         return _MakeSearch(null, table, options.Value, bingo_schema, "MASS", false, min, max);
      }

      private delegate bool bingoImportPopulateDataRow 
         (DataRow row, List<string[]> parameters);

      private static void _ImportData (string table_name, string data_column_name,
         string additional_parameters, bingoImportPopulateDataRow populateRowFunc)
      {
         BingoLog.logMessage("Importing into {0}", table_name);

         using (SqlConnection ctx_conn = new SqlConnection("context connection=true"))
         {
            ctx_conn.Open();

            List<string[]> parameters = new List<string[]>();
            string[] params_array = additional_parameters.Split(new char[] {',', ';'});
            foreach (string p in params_array)
            {
               if (p != "")
               {
                  string[] name_and_column = p.Trim().Split(' ');
                  parameters.Add(name_and_column);
               }
            }

            DataTable dt = new DataTable();

            dt.Columns.Add(new DataColumn(data_column_name, 
               Type.GetType("System.String")));

            foreach (string[] p in parameters)
               dt.Columns.Add(new DataColumn(p[1], Type.GetType("System.String")));

            using (BingoSession session = new BingoSession())
            {
               BingoCore.bingoProfilingReset(true);
               BingoTimer timer = new BingoTimer("total");

               SqlConnection ext_conn = null;
               try
               {
                  ext_conn = new SqlConnection("server=" + getServername(ctx_conn) +
                     ";integrated security=true;database=" + ctx_conn.Database);
                  ext_conn.Open();

                  int imported_count = 0;

                  bool has_data = false;
                  do
                  {
                     DataRow new_row = dt.NewRow();
                     has_data = populateRowFunc(new_row, parameters);
                     if (has_data)
                     {
                        dt.Rows.Add(new_row);
                        imported_count++;
                     }

                     if (dt.Rows.Count >= 10000 || !has_data)
                     {
                        // Flush data table via SqlBulkCopy
                        BingoTimer timer_sql = new BingoTimer("import.sql_bulk_copy");

                        using (SqlTransaction transaction =
                                 ext_conn.BeginTransaction())
                        {
                           using (SqlBulkCopy bulkCopy = new SqlBulkCopy(ext_conn, 
                                                               SqlBulkCopyOptions.TableLock | 
                                                               SqlBulkCopyOptions.FireTriggers, 
                                                               transaction))
                           {
                              bulkCopy.DestinationTableName = table_name;
                              bulkCopy.ColumnMappings.Add(data_column_name, data_column_name);
                              foreach (string[] p in parameters)
                                 bulkCopy.ColumnMappings.Add(p[1], p[1]);

                              bulkCopy.BatchSize = dt.Rows.Count;
                              bulkCopy.BulkCopyTimeout = 3600;
                              bulkCopy.WriteToServer(dt);
                           }
                           transaction.Commit();
                        }
                        timer_sql.end();

                        BingoCore.bingoProfIncCounter("import.sql_bulk_copy_size", dt.Rows.Count);
                        dt.Rows.Clear();

                        BingoLog.logMessage("  {0} molecules imported", imported_count);
                        BingoLog.logMessage("Intermediate statistics for import into table {0}:\n{1}\n",
                           table_name, BingoCore.bingoProfilingGetStatistics(true));
                        BingoCore.bingoProfilingReset(false);
                     }
                  } while (has_data);

                  BingoLog.logMessage("  Done.");
                  timer.end();

                  BingoLog.logMessage("Statistics for import into table {0}:\n{1}\n",
                     table_name, BingoCore.bingoProfilingGetStatistics(true));
               }
               catch (Exception ex)
               {
                  BingoLog.logMessage("Exception {0} in {1}:\n{2}", ex.Message, ex.Source, ex.StackTrace);
                  throw;
               }
               finally
               {
                  if (ext_conn != null)
                     ext_conn.Close();
               }
            }
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static void ImportSDF(SqlString table_name, SqlString mol_column_name,
         SqlString file_name, SqlString additional_parameters, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         bool reader_opered = false;

         bingoImportPopulateDataRow populateRowFunc =
               (row, parameters) =>
               {
                  if (!reader_opered)
                  {
                     if (BingoCore.bingoSDFImportOpen(file_name.Value) != 1)
                        throw new Exception(BingoCore.bingoGetError());
                     reader_opered = true;
                  }

                  if (BingoCore.bingoSDFImportEOF() != 0)
                     return false;

                  BingoTimer timer_mol = new BingoTimer("import.read_mol");
                  row[mol_column_name.Value] = BingoCore.bingoSDFImportGetNext();
                  timer_mol.end();

                  foreach (string[] p in parameters)
                     row[p[1]] = BingoCore.bingoSDFImportGetParameter(p[0]);

                  return true;
               };

         try
         {
            _ImportData(table_name.Value, mol_column_name.Value, 
               additional_parameters.Value, populateRowFunc);
         }
         finally
         {
            if (reader_opered)
               BingoCore.bingoSDFImportClose();
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static void ImportRDF (SqlString table_name, SqlString react_column_name,
         SqlString file_name, SqlString additional_parameters, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         bool reader_opered = false;

         bingoImportPopulateDataRow populateRowFunc =
               (row, parameters) =>
               {
                  if (!reader_opered)
                  {
                     if (BingoCore.bingoRDFImportOpen(file_name.Value) != 1)
                        throw new Exception(BingoCore.bingoGetError());
                     reader_opered = true;
                  }

                  if (BingoCore.bingoRDFImportEOF() != 0)
                     return false;

                  BingoTimer timer_mol = new BingoTimer("import.read_mol");
                  row[react_column_name.Value] = BingoCore.bingoRDFImportGetNext();
                  timer_mol.end();

                  foreach (string[] p in parameters)
                     row[p[1]] = BingoCore.bingoRDFImportGetParameter(p[0]);

                  return true;
               };

         try
         {
            _ImportData(table_name.Value, react_column_name.Value,
               additional_parameters.Value, populateRowFunc);
         }
         finally
         {
            if (reader_opered)
               BingoCore.bingoRDFImportClose();
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static void ImportSMILES (SqlString table_name, SqlString mol_column_name,
         SqlString file_name, SqlString id_column_name, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         bool have_id_col = !id_column_name.IsNull && id_column_name.Value.Length > 1;
         StreamReader reader = null;

         bingoImportPopulateDataRow populateRowFunc =
               (row, parameters) =>
               {
                  if (reader.EndOfStream)
                     return false;

                  String line = reader.ReadLine();
                  row[mol_column_name.Value] = line;

                  if (have_id_col)
                  {
                     Match match = Regex.Match(line, @"[\s]*[^\s]*[\s]*\|.*\|[\s]+([^\s].*)");

                     string id_data;

                     if (match != Match.Empty)
                        id_data = match.Groups[1].Value;
                     else
                     {
                        match = Regex.Match(line, @"[\s]*[^\s]*[\s]+([^\s\|].*)");

                        if (match != Match.Empty)
                           id_data = match.Groups[1].Value;
                        else
                           id_data = null;
                     }
                     row[id_column_name.Value] = id_data;
                  }

                  return true;
               };

         using (reader = new StreamReader(file_name.Value))
         {
            string additional_parameters = "";
            if (have_id_col)
               additional_parameters = id_column_name.Value + " " + id_column_name.Value;

            _ImportData(table_name.Value, mol_column_name.Value,
               additional_parameters, populateRowFunc);
         }
      }

      [Flags]
      enum ContextFlags
      {
         FLAG_TAU_RULES = 0x01,
         FLAG_FINGERPRINTS = 0x02,
         FLAG_CMF = 0x04,
         FLAG_X_PSEUDO = 0x08,
         FLAG_IGNORE_CBDM = 0x10,
      };

      private static void prepareContext (SqlConnection connection, string bingo_schema,
         int id, ContextFlags flags)
      {
         BingoCore.setContext(id);

         if ((flags & ContextFlags.FLAG_X_PSEUDO) != 0)
         {
            BingoCore.setConfigInt("treat-x-as-pseudoatom",
                BingoConfig.getInt(connection, bingo_schema, "treat-x-as-pseudoatom", id));
         }

         if ((flags & ContextFlags.FLAG_IGNORE_CBDM) != 0)
         {
            BingoCore.setConfigInt("ignore-closing-bond-direction-mismatch",
                BingoConfig.getInt(connection, bingo_schema,
                "ignore-closing-bond-direction-mismatch", id));
         }

         if ((flags & ContextFlags.FLAG_TAU_RULES) != 0)
         {
            ArrayList rules = BingoConfig.getTautomerRules(connection, bingo_schema);

            BingoCore.clearTautomerRules();
            foreach (BingoConfig.TautomerRule rule in rules)
            {
               BingoCore.addTautomerRule(rule.n, rule.beg, rule.end);
            }
            BingoCore.tautomerRulesReady();
         }

         if ((flags & ContextFlags.FLAG_FINGERPRINTS) != 0)
         {
            BingoCore.setConfigInt("FP_ORD_SIZE",
                BingoConfig.getInt(connection, bingo_schema, "FP_ORD_SIZE", id));
            BingoCore.setConfigInt("FP_ANY_SIZE",
                BingoConfig.getInt(connection, bingo_schema, "FP_ANY_SIZE", id));
            BingoCore.setConfigInt("FP_TAU_SIZE",
                BingoConfig.getInt(connection, bingo_schema, "FP_TAU_SIZE", id));
            BingoCore.setConfigInt("FP_SIM_SIZE",
                BingoConfig.getInt(connection, bingo_schema, "FP_SIM_SIZE", id));

            BingoCore.setConfigInt("SUB_SCREENING_MAX_BITS",
                BingoConfig.getInt(connection, bingo_schema, "SUB_SCREENING_MAX_BITS", id));
            BingoCore.setConfigInt("SIM_SCREENING_PASS_MARK",
                BingoConfig.getInt(connection, bingo_schema, "SIM_SCREENING_PASS_MARK", id));
         }
         if ((flags & ContextFlags.FLAG_CMF) != 0)
         {
            BingoCore.setConfigBin("cmf-dict",
                BingoConfig.getBinary(connection, bingo_schema, "cmf-dict", id));
         }
      }

      private static void saveContext (SqlConnection connection, string bingo_schema,
         int id, ContextFlags flags)
      {
         BingoCore.setContext(id);

         if ((flags & ContextFlags.FLAG_CMF) != 0)
         {
            BingoConfig.setBinary(connection, bingo_schema, "cmf-dict", id, 
               BingoCore.getConfigBin("cmf-dict"));
         }
      }

      [SqlProcedure]
      [BingoSqlFunction(access_level = AccessLevelKind.None)]
      public static void OnSessionClose (SqlString spid_str)
      {
         int spid = Convert.ToInt32(spid_str.Value);

         BingoIndexData.OnSessionClose(spid);
      }

      private static int getSPID (SqlConnection conn)
      {
         return BingoSqlUtils.ExecIntQuery(conn, "SELECT @@spid AS spid").Value;
      }

      private static string getServername (SqlConnection conn)
      {
         return BingoSqlUtils.ExecStringQuery(conn, "SELECT @@SERVERNAME AS spid");
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString Smiles (SqlString molecule, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM;
            using (SqlConnection conn = new SqlConnection("context connection=true"))
            {
               conn.Open();
               prepareContext(conn, bingo_schema.Value, 0, flags);
            }

            return BingoCore.mangoSMILES(molecule.Value, false);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString RSmiles (SqlString molecule, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM;
            using (SqlConnection conn = new SqlConnection("context connection=true"))
            {
               conn.Open();
               prepareContext(conn, bingo_schema.Value, 0, flags);
            }

            return BingoCore.ringoRSMILES(molecule.Value);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString Molfile (SqlString molecule, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM;
            using (SqlConnection conn = new SqlConnection("context connection=true"))
            {
               conn.Open();
               prepareContext(conn, bingo_schema.Value, 0, flags);
            }

            return BingoCore.mangoMolfile(molecule.Value);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString Rxnfile (SqlString reaction, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM;
            using (SqlConnection conn = new SqlConnection("context connection=true"))
            {
               conn.Open();
               prepareContext(conn, bingo_schema.Value, 0, flags);
            }

            return BingoCore.ringoRxnfile(reaction.Value);
         }
      }


      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString CanSmiles (SqlString molecule, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema); 

         using (BingoSession session = new BingoSession())
         {
            ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM;
            using (SqlConnection conn = new SqlConnection("context connection=true"))
            {
               conn.Open();
               prepareContext(conn, bingo_schema.Value, 0, flags);
            }

            return BingoCore.mangoSMILES(molecule.Value, true);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlSingle Mass (SqlString molecule, SqlString type, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM;
            using (SqlConnection conn = new SqlConnection("context connection=true"))
            {
               conn.Open();
               prepareContext(conn, bingo_schema.Value, 0, flags);
            }

            return BingoCore.mangoMass(molecule.Value, molecule.Value.Length, type.Value);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString Gross (SqlString molecule, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            ContextFlags flags = ContextFlags.FLAG_X_PSEUDO | ContextFlags.FLAG_IGNORE_CBDM;
            using (SqlConnection conn = new SqlConnection("context connection=true"))
            {
               conn.Open();
               prepareContext(conn, bingo_schema.Value, 0, flags);
            }

            return BingoCore.mangoGross(molecule.Value);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
         SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunctionForReader]
      public static SqlString Name(SqlString molecule, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            return BingoCore.bingoGetNameCore(molecule.Value);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static void _OnInsertRecordTrigger (SqlString full_table_name,
         SqlInt32 id, SqlString data, SqlString bingo_schema)
      {
         bingoGetIndexDataDelegate getDataDelegate =
            (ctx_conn, conn, schema) => BingoIndexData.GetIndexData(conn, bingo_schema.Value,
               full_table_name.Value, getSPID(ctx_conn));

         bingoOperationDelegate insertOp =
            (ctx_conn, conn, index_data) =>
            {
               if (id.IsNull || data.IsNull)
               {
                  string message = String.Format("Record: id=null or data=null.");
                  SqlContext.Pipe.Send(message);
                  BingoLog.logMessage(message);
                  return;
               }
               if (index_data.getIndexType() == BingoIndexData.IndexType.Molecule)
                  _AddMoleculeToIndex(conn, (MangoIndexData)index_data, id.Value, data.Value);
               else
                  _AddReactionToIndex(conn, (RingoIndexData)index_data, id.Value, data.Value);
            };

         _ExecuteBingoOperationChangeIndex(full_table_name, bingo_schema, insertOp,
            getDataDelegate, BingoOp.LOAD_CMF);
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static void _OnDeleteRecordTrigger (SqlString full_table_name,
         SqlInt32 id, SqlString bingo_schema)
      {
         bingoGetIndexDataDelegate getDataDelegate =
            (ctx_conn, conn, schema) => BingoIndexData.GetIndexData(conn, bingo_schema.Value,
               full_table_name.Value, getSPID(ctx_conn));

         bingoOperationDelegate insertOp =
            (ctx_conn, conn, index_data) =>
            {
               index_data.prepareForDeleteRecord(conn);
               index_data.deleteRecordById(id.Value, conn);
            };

         _ExecuteBingoOperationChangeIndex(full_table_name, bingo_schema, insertOp,
            getDataDelegate, BingoOp.LOAD_CMF);
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static void FlushOperations (SqlString table_name, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (SqlConnection conn = new SqlConnection("context connection=true"))
         {
            conn.Open();
            using (SqlConnection ext_conn = new SqlConnection("server=" + getServername(conn) +
                  ";integrated security=true;database=" + conn.Database))
            {
               ext_conn.Open();

               using (BingoSession session = new BingoSession())
               {
                  BingoIndexData index_data = BingoIndexData.GetIndexData(conn,
                     bingo_schema.Value, table_name.Value, getSPID(conn));

                  index_data.flush(ext_conn);
               }
            }
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction(access_level=AccessLevelKind.None)]
      public static void _DropAllIndices (SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);
         using (SqlConnection conn = new SqlConnection("context connection=true"))
         {
            conn.Open();
            using (BingoSession session = new BingoSession())
            {
               BingoIndexData.DropAllIndices(conn, bingo_schema.Value, false);
            }
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction(access_level = AccessLevelKind.None)]
      public static void _FlushInAllSessions (SqlString bingo_schema)
      {
         BingoIndexData.FlushInAllSessions(-1, false);
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static void DropInvalidIndices (SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);
         using (SqlConnection conn = new SqlConnection("context connection=true"))
         {
            conn.Open();
            using (BingoSession session = new BingoSession())
            {
               BingoIndexData.DropAllIndices(conn, bingo_schema.Value, true);
            }
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static int GetAtomCount (SqlString molecule, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            return BingoCore.mangoGetAtomCount(molecule.Value, molecule.Value.Length);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static int GetBondCount (SqlString molecule, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            return BingoCore.mangoGetBondCount(molecule.Value, molecule.Value.Length);
         }
      }

      /*
       * Profiling functions
       */
      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static SqlString GetStatistics (SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            return BingoCore.bingoProfilingGetStatistics(true);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static void ResetStatistics (SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            BingoCore.bingoProfilingReset(true);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static float ProfilingGetTime (SqlString counter_name, 
         SqlBoolean whole_session, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            return BingoCore.bingoProfilingGetTime(counter_name.Value, 
               whole_session.Value);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static long ProfilingGetValue (SqlString counter_name,
         SqlBoolean whole_session, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            return BingoCore.bingoProfilingGetValue(counter_name.Value,
               whole_session.Value);
         }
      }

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static long ProfilingGetCount (SqlString counter_name,
         SqlBoolean whole_session, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);

         using (BingoSession session = new BingoSession())
         {
            return BingoCore.bingoProfilingGetCount(counter_name.Value,
               whole_session.Value);
         }
      }

      /*
       * Test functions 
       */

      [SqlFunction(DataAccess = DataAccessKind.Read,
        SystemDataAccess = SystemDataAccessKind.Read)]
      [BingoSqlFunction]
      public static void _CheckMemoryAllocate(SqlInt32 dotnet_size_mb, SqlInt32 block_size_mb, 
         SqlInt32 core_size_mb, SqlString bingo_schema)
      {
         BingoDll.load(bingo_schema);
         using (BingoSession session = new BingoSession())
         {
            long block_size = block_size_mb.Value;
            BingoLog.logMessage("Allocating {0} Mb in .NET by {1}Mb blocks...", 
               dotnet_size_mb.Value, block_size);

            int sum = 0;

            List<object> mem_blocks = new List<object>();
            long mem_left = dotnet_size_mb.Value;

            int index = 0;
            while (mem_left != 0)
            {
               long cur_alloc;
               if (mem_left > block_size)
                  cur_alloc = block_size;
               else
                  cur_alloc = mem_left;

               byte[] data = new byte[cur_alloc * 1024 * 1024];
               foreach (byte b in data)
                  sum += b;
               mem_blocks.Add(data);
               index++;

               mem_left -= cur_alloc;
               BingoLog.logMessage("   block {0} allocated, {1}Mb is left to allocate...", 
                  index, mem_left);
            }

            BingoLog.logMessage("Check sum is {0}", sum);
            BingoLog.logMessage("Allocating {0} Mb in bingo-core...", core_size_mb.Value);

            int ret = BingoCore.bingoCheckMemoryAllocate(core_size_mb.Value * 1024 * 1024);
            if (ret != 1)
            {
               BingoLog.logMessage("  Failed: {0}", BingoCore.bingoGetError());
               throw new Exception("Failed. See Bingo log for details");
            }
            BingoLog.logMessage("  OK");

            BingoLog.logMessage("  Waiting 2 sec...");
            Thread.Sleep(2000);

            BingoCore.bingoCheckMemoryFree();

            BingoLog.logMessage("  Done.");
            SqlContext.Pipe.Send("Done.");
         }
      }

   }
}
