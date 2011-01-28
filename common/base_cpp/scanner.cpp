/****************************************************************************
 * Copyright (C) 2009-2011 GGA Software Services LLC
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
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>

#include "base_c/defs.h"
#include "base_cpp/scanner.h"
#include "base_cpp/tlscont.h"
#include "reusable_obj_array.h"

using namespace indigo;

Scanner::~Scanner ()
{
}

int Scanner::readIntFix (int digits)
{
   int result;

   char buf[20];
   if (digits >= NELEM(buf) - 1)
      throw Error("readIntFix(): digits = %d", digits);

   read(digits, buf);
   buf[digits] = 0;

   char *end;
   result = strtol(buf, &end, 10);
   // Check that unread part contains onle spaces
   while (end != buf + digits)
   {
      if (!isspace(*end))
         throw Error("readIntFix(): invalid number representation: %s", buf);
      end++;
   }

   return result;
}

int Scanner::readInt1 (void)
{
   QS_DEF(Array<char>, buf);
   char c;
   int result;

   buf.clear();

   while (!isEOF())
   {
      c = readChar();
      if (!isdigit(c) && c != '-' && c != '+')
         break;
      buf.push(c);
   }

   buf.push(0);
   
   if (sscanf(buf.ptr(), "%d", &result) < 1)
      throw Error("readInt(): error parsing %s", buf.ptr());

   return result;
}

int Scanner::readInt (void)
{
   QS_DEF(Array<char>, buf);
   char c;
   int result;

   buf.clear();

   c = readChar();

   if (c == '+' || c == '-' || isdigit(c))
      buf.push(c);

   while (isdigit(lookNext()))
      buf.push(readChar());

   buf.push(0);

   if (sscanf(buf.ptr(), "%d", &result) < 1)
      throw Error("readInt(): error parsing %s", buf.ptr());

   return result;
}

int Scanner::readUnsigned ()
{
   int result = 0;
   bool was_digit = false;

   while (!isEOF())
   {
      char c = readChar();
      if (isdigit(c))
      {
         was_digit = true;
         result = (int)(c - '0') + result * 10;
      }
      else
      {
         seek(-1, SEEK_CUR);
         break;
      }
   }

   if (!was_digit)
      throw Error("readUnsigned(): no digits");

   return result;
}

float Scanner::readFloat (void)
{
   QS_DEF(Array<char>, buf);
   float result;

   buf.clear();

   while (!isEOF())
   {
      int c = lookNext();

      if (!isdigit(c) && c != '-' && c != '+' && c != '.')
         break;
      buf.push(readChar());
   }

   buf.push(0);
   
   if (sscanf(buf.ptr(), "%f", &result) < 1)
      throw Error("readFloat(): error parsing %s", buf.ptr());

   return result;
}

bool Scanner::tryReadFloat (float &value)
{
   QS_DEF(Array<char>, buf);

   int pos = tell();

   buf.clear();

   while (!isEOF())
   {
      char c = readChar();

      if (!isdigit(c) && c != '-' && c != '+' && c != '.')
         break;
      buf.push(c);
   }

   buf.push(0);
   
   if (sscanf(buf.ptr(), "%f", &value) < 1)
   {
      seek(pos, SEEK_SET);
      return false;
   }

   return true;
}

void Scanner::readWord (Array<char> &word, const char *delimiters)
{
   word.clear();

   if (isEOF())
      throw Error("readWord(): end of stream");

   do
   {
      int next = lookNext();

      if (next == -1)
         break;

      if (delimiters == 0 && isspace((char)next))
         break;

      if (delimiters != 0 && strchr(delimiters, (char)next) != NULL)
         break;

      word.push(readChar());
   } while (!isEOF());

   word.push(0);
}


float Scanner::readFloatFix (int digits)
{
   char buf[40];
   if (digits >= NELEM(buf) - 1)
      throw Error("readFloatFix(): digits = %d", digits);

   float result;

   read(digits, buf);
   buf[digits] = 0;

   char *end;
   result = (float)strtod(buf, &end);
   // Check that unread part contains onle spaces
   while (end != buf + digits)
   {
      if (!isspace(*end))
         throw Error("readIntFix(): invalid number representation: %s", buf);
      end++;
   }

   return result;
}

char Scanner::readChar ()
{
   char c;

   read(sizeof(char), &c);
   return c;
}

byte Scanner::readByte ()
{
   byte c;

   read(1, &c);
   return c;
}

bool Scanner::skipLine ()
{
   char c;

   if (isEOF())
      return false;

   while (!isEOF())
   {
      c = readChar();
      if (c == '\n')
      {
         if (lookNext() == '\r')
            skip(1);
         return true;
      }
      if (c == '\r')
      {
         if (lookNext() == '\n')
            skip(1);
         return true;
      }
   }

   return false;
}

void Scanner::read (int length, Array<char> &buf)
{
   buf.resize(length);
   read(length, buf.ptr());
}

bool Scanner::skipString ()
{
   return skipLine();
}

void Scanner::skipSpace ()
{
   while (isspace(lookNext()))
      skip(1);
}

void Scanner::appendLine (Array<char> &out, bool append_zero)
{
   if (isEOF())
      throw Error("appendLine(): end of stream");

   do
   {
      char c = readChar();

      if (c == '\r')
      {
         if (lookNext() == '\n')
            continue;
         break;
      }
      if (c == '\n')
         break;

      out.push(c);
   } while (!isEOF());

   if (append_zero)
      out.push(0);
}

void Scanner::readString (Array<char> &out, bool append_zero)
{
   out.clear();
   appendLine(out, append_zero);
}

void Scanner::readLine (Array<char> &out, bool append_zero)
{
   out.clear();
   appendLine(out, append_zero);
}

void Scanner::readCharsFix (int n, char *chars_out)
{
   read(n, chars_out);
}

word Scanner::readBinaryWord ()
{
   word res;

   read(sizeof(word), &res);

   return res;
}

dword Scanner::readBinaryDword ()
{
   dword res;

   read(sizeof(dword), &res);

   return res;
}

int Scanner::readBinaryInt ()
{
   int res;

   read(sizeof(int), &res);

   return res;
   //*res = ntohl(*res);
}

float Scanner::readBinaryFloat ()
{
   float res;

   read(sizeof(float), &res);

   return res;
}

short Scanner::readPackedShort ()
{
   byte high = readByte();

   if (high < 128)
      return high;

   byte low = readByte();

   high -= 128;

   return high * (short)256 + low;
}

void Scanner::readAll (Array<char> &arr)
{
   arr.clear_resize(length() - tell());

   read(arr.size(), arr.ptr());
}

bool Scanner::isSingleLine (Scanner &scanner)
{
   int pos = scanner.tell();

   scanner.skipString();

   bool res = scanner.isEOF();

   scanner.seek(pos, SEEK_SET);
   return res;
}

//
// FileScanner
//

FileScanner::FileScanner (Encoding filename_encoding, const char *filename)
{
   _init(filename_encoding, filename);
}

FileScanner::FileScanner (const char *format, ...)
{
   char filename[1024];

   va_list args;

   va_start(args, format);
   vsnprintf(filename, sizeof(filename), format, args);
   va_end(args);

   _init(ENCODING_ASCII, filename);
}

void FileScanner::_init (Encoding filename_encoding, const char *filename)
{
   _file = 0;
   _file_len = 0;

   _file = openFile(filename_encoding, filename, "rb");

   if (_file == NULL)
      throw Error("can't open file %s", filename);

   fseek(_file, 0, SEEK_END);
   _file_len = ftell(_file);
   fseek(_file, 0, SEEK_SET);
   _invalidateCache();
}

int FileScanner::lookNext ()
{
   _validateCache();
   if (_cache_pos == _max_cache)
      return -1;
   
   return _cache[_cache_pos];
}

void FileScanner::_invalidateCache ()
{
   _max_cache = 0;
   _cache_pos = 0;
}

void FileScanner::_validateCache ()
{
   if (_cache_pos < _max_cache)
      return;

   size_t nread = fread(_cache, 1, NELEM(_cache), _file);
   _max_cache = nread;
   _cache_pos = 0;
}

int FileScanner::tell ()
{
   _validateCache();
   return ftell(_file) - _max_cache + _cache_pos;
}

void FileScanner::read (int length, void *res)
{
   int to_read_from_cache = __min(length, _max_cache - _cache_pos);
   memcpy(res, _cache + _cache_pos, to_read_from_cache);
   _cache_pos += to_read_from_cache;

   if (to_read_from_cache != length)
   {
      int left = length - to_read_from_cache;
      size_t nread = fread((char*)res + to_read_from_cache, 1, left, _file);

      if (nread != (size_t)left)
         throw Error("FileScanner::read() error");
   }
}

bool FileScanner::isEOF ()
{
   if (_file == NULL)
      return true;
   if (_cache_pos < _max_cache)
      return false;

   return tell() == _file_len;
}

void FileScanner::skip (int n)
{
   _validateCache();
   _cache_pos += n;

   if (_cache_pos > _max_cache)
   {
      int delta = _cache_pos - _max_cache;
      int res = fseek(_file, delta, SEEK_CUR);
      _invalidateCache();

      if (res != 0)
         throw Error("skip() passes after end of file");
   }
}

void FileScanner::seek (int pos, int from)
{
   if (from == SEEK_CUR)
      fseek(_file, pos - _max_cache + _cache_pos, from);
   else
      fseek(_file, pos, from);
   _invalidateCache();
}

int FileScanner::length ()
{
   return _file_len;
}

char FileScanner::readChar ()
{
   _validateCache();
   if (_cache_pos == _max_cache)
      throw Error("readChar() passes after end of file");
   return _cache[_cache_pos++];
}

FileScanner::~FileScanner ()
{
   if (_file != NULL)
      fclose(_file);
}

//
// BufferScanner
//

void BufferScanner::_init (const char *buffer, int size)
{
   if (size < -1 || (size > 0 && buffer == 0))
      throw Error("incorrect parameters in BufferScanner constructor");

   _buffer = buffer;
   _size = size;
   _offset = 0;
}

BufferScanner::BufferScanner (const char *buffer, int buffer_size)
{
   _init(buffer, buffer_size);
}

BufferScanner::BufferScanner (const byte *buffer, int buffer_size)
{
   _init((const char *)buffer, buffer_size);
}

BufferScanner::BufferScanner (const char *str)
{
   _init(str, (int)strlen(str));
}

BufferScanner::BufferScanner (const Array<char> &arr)
{
   _init(arr.ptr(), arr.size());
}

bool BufferScanner::isEOF ()
{
   if (_size < 0)
      throw Error("isEOF() called to unlimited buffer");
   return _offset >= _size;
}

void BufferScanner::read (int length, void *res)
{
   if (_size >= 0 && _offset + length > _size)
      throw Error("BufferScanner::read() error");

   memcpy(res, &_buffer[_offset], length);
   _offset += length;
}


int BufferScanner::lookNext ()
{
   if (_size >= 0 && _offset >= _size)
      return -1;

   return _buffer[_offset];
}

int BufferScanner::length ()
{
   return _size;
}

int BufferScanner::tell ()
{
   return _offset;
}

const void * BufferScanner::curptr ()
{
   return _buffer + _offset;
}

void BufferScanner::skip (int n)
{
   _offset += n;

   if (_size >= 0 && _offset > _size)
      throw Error("skip() passes after end of buffer");
}

void BufferScanner::seek (int pos, int from)
{
   if (from == SEEK_SET)
      _offset = pos;
   else if (from == SEEK_CUR)
      _offset += pos;
   else // SEEK_END
   {
      if (_size < 0)
         throw Error("can not seek from end: buffer is unlimited");
      _offset = _size - pos;
   }

   if ((_size >= 0 && _offset > _size) || _offset < 0)
      throw Error("size = %d, offset = %d after seek()", _size, _offset);
}

byte BufferScanner::readByte ()
{
   if (_size >= 0 && _offset >= _size)
      throw Error("readByte(): end of buffer");

   return _buffer[_offset++];
}

void Scanner::_prefixFunction (Array<char> &str, Array<int> &prefix)
{
   prefix.clear();
   prefix.push(0);

   int i, k = 0;

   for (i = 1; i < str.size(); i++)
   {
      while ((k > 0) && (str[k] != str[i]))
         k = prefix[k - 1];
      if (str[k] == str[i])
         k++;
      prefix.push(k);
   }
}

bool Scanner::findWord (const char *word)
{
   QS_DEF(ReusableObjArray< Array<char> >, strs);

   strs.clear();
   Array<char> &str = strs.push();

   str.readString(word, false);
   return findWord(strs) == 0;
}

int Scanner::findWord (ReusableObjArray< Array<char> > &words)
{
   if (isEOF())
      return -1;
   
   QS_DEF(ReusableObjArray< Array<int> >, prefixes);
   QS_DEF(Array<int>, pos);
   int i;
   int pos_saved = tell();
   
   prefixes.clear();
   pos.clear();
   
   for (i = 0; i < words.size(); i++)
   {
      _prefixFunction(words[i], prefixes.push());
      pos.push(0);
   }

   while (!isEOF())
   {
      int c = readChar();

      for (i = 0; i < words.size(); i++)
      {
         while (pos[i] > 0 && words[i][pos[i]] != c)
            pos[i] = prefixes[i][pos[i] - 1];
         if (words[i][pos[i]] == c)
            pos[i]++;

         if (pos[i] == words[i].size())
         {
            seek(-words[i].size(), SEEK_CUR);
            return i;
         }
      }
   }

   seek(pos_saved, SEEK_SET);
   return -1;
}
