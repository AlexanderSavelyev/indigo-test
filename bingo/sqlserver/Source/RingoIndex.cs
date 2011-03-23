﻿using System;
using System.Runtime.InteropServices; 

namespace indigo
{
   public class RingoIndex
   {
      public float mass;
      public byte[] crf;

      public byte[] fingerprint;

      public bool prepare (string reaction)
      {
         IntPtr crf_ptr, fingerprint_ptr;
         int crf_buf_len, fingerprint_buf_len;

         int ret = BingoCore.lib.ringoIndexPrepareReaction(reaction, reaction.Length,
            out crf_ptr, out crf_buf_len, out fingerprint_ptr, out fingerprint_buf_len);

         if (ret == -2)
            throw new Exception(BingoCore.lib.bingoGetError());
         if (ret == -1)
            return false;

         crf = new byte[crf_buf_len];
         fingerprint = new byte[fingerprint_buf_len];

         Marshal.Copy(crf_ptr, crf, 0, crf_buf_len);
         Marshal.Copy(fingerprint_ptr, fingerprint, 0, fingerprint_buf_len);

         return true;
      }
   }
}
