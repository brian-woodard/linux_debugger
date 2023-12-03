#pragma once

#include <termios.h>
#include <string>
#include "RingBuffer.h"

class CInputHandler
{
public:

   static const int MAX_LINE = 128;
   static const int MAX_HISTORY = 50;

   CInputHandler(const char* Prompt, FILE* Stream);
   ~CInputHandler();

   int GetInput(char* String);

   int DisableRawMode();
   int EnableRawMode();

private:

   enum eSpecialKeys
   {
      KEY_CTRL_C = 3,
      KEY_ENTER = 13,
      KEY_BACKSPACE = 127,
      KEY_ARROW_LEFT = 1000,
      KEY_ARROW_RIGHT,
      KEY_ARROW_UP,
      KEY_ARROW_DOWN,
      KEY_DEL,
      KEY_HOME,
      KEY_END,
      KEY_PAGE_UP,
      KEY_PAGE_DOWN
   };

   void AddCharToLine(int Key);
   void RemoveCharFromLine(int Key);
   void ClearLine();
   void ClearPrompt();
   void CopyToLine(const char* String);
   void PutLine(const char* Output = nullptr);
   void FlushStream();

   int ReadInput();
   void ProcessKeyPress(int Key);

   char                                     mLine[MAX_LINE];
   struct termios                           mStdinTermios;
   CRingBuffer<char[MAX_LINE], MAX_HISTORY> mHistory;
   int                                      mHistoryIndex;
   int                                      mCursorCol;
   const char*                              mPrompt;
   FILE*                                    mStream;

};
