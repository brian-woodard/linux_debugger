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

   void ClearLine();
   void PutLine(const char* Output = nullptr);
   void FlushStream();

   int ReadInput();
   void ProcessKeyPress(int Key);

   struct termios                           mStdinTermios;
   CRingBuffer<char[MAX_LINE], MAX_HISTORY> mHistory;
   int                                      mHistoryIndex;
   std::string                              mLine;
   std::string                              mOutputLine;
   const char*                              mPrompt;
   FILE*                                    mStream;

};
