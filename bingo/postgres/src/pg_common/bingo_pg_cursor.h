#ifndef _BINGO_PG_CURSOR_H__
#define	_BINGO_PG_CURSOR_H__

#include "base_cpp/array.h"
#include "bingo_postgres.h"

class BingoItemData;
class BingoPgText;

class BingoPgCursor {
public:
   BingoPgCursor(const char *format, ...);
   BingoPgCursor(indigo::Array<char>& query_str);
   virtual ~BingoPgCursor();

   bool next();
   
   void getId(int arg_idx, BingoItemData&);
   void getText(int arg_idx, BingoPgText&);
   unsigned int getDatum(int arg_idx);
private:
   BingoPgCursor(const BingoPgCursor&); //no implicit copy

   void _init(indigo::Array<char>& query_str);

   indigo::Array<char> _cursorName;
   PG_OBJECT _cursorPtr;

};

#endif	/* BINGO_PG_CURSOR_H */

