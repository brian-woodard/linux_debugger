
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <thread>
#include "InputHandler.h"

CInputHandler::CInputHandler(const char* Prompt, FILE* Stream)
   : mStdinTermios{},
     mHistory(),
     mHistoryIndex(0),
     mCursorCol(0),
     mLine{},
     mPrompt(Prompt),
     mStream(Stream)
{
   EnableRawMode();
}

CInputHandler::~CInputHandler()
{
   DisableRawMode();
}

int CInputHandler::GetInput(char* String)
{
   int key;
   int i;
   int output_length;

   ClearLine();
   PutLine();

   do
   {
      key = ReadInput();

      if (key > 0)
      {
         ProcessKeyPress(key);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));

   } while (key != KEY_ENTER);

   output_length = strlen(mLine);

   for (i = 0; i < output_length; i++)
   {
      String[i] = mLine[i];
   }
   String[i] = 0;

   PutLine("\r\n");

   return output_length;
}

void CInputHandler::AddCharToLine(char Key)
{
   size_t len = strlen(mLine);

   if (len < MAX_LINE-1)
   {
      if (mCursorCol < len)
      {
         for (int i = len+1; i >= mCursorCol; i--)
         {
            mLine[i+1] = mLine[i];
         }
         mLine[mCursorCol] = Key;
      }
      else
      {
         mLine[len] = Key;
      }
      mCursorCol++;
   }
}

void CInputHandler::RemoveCharFromLine()
{
   size_t len = strlen(mLine);
   if (len > 0 && mCursorCol > 0)
   {
      if (mCursorCol < len)
      {
         for (int i = mCursorCol-1; i < len; i++)
         {
            mLine[i] = mLine[i+1];
         }
      }
      else
      {
         mLine[len-1] = 0;
      }
      mCursorCol--;
   }
}

void CInputHandler::ClearLine()
{
   for (int i = 0; i < MAX_LINE; i++)
   {
      mLine[i] = 0;
   }
   mCursorCol = 0;
}

void CInputHandler::ClearPrompt()
{
   fprintf(mStream, "\r%s", mPrompt);
   for (int i = 0; i < MAX_LINE; i++)
   {
      fprintf(mStream, " ");
   }
}

void CInputHandler::CopyToLine(const char* String)
{
   if (String)
   {
      int i;
      int len = strlen(String);

      if (len > MAX_LINE) len = MAX_LINE;

      for (i = 0; i < len; i++)
      {
         mLine[i] = String[i];
      }
      for (; i < MAX_LINE; i++)
      {
         mLine[i] = 0;
      }
      mLine[i] = 0;
      mCursorCol = len;
   }
   else
   {
      ClearLine();
   }
}

void CInputHandler::PutLine(const char* Output)
{
   if (Output)
      fprintf(mStream, Output);
   else
      fprintf(mStream, "\r%s%s", mPrompt, mLine);

   if (mCursorCol < strlen(mLine))
   {
      fprintf(mStream, "\r%s", mPrompt);
      for (int i = 0; i < mCursorCol; i++)
      {
         fprintf(mStream, "%c", mLine[i]);
      }
   }
   FlushStream();
}

void CInputHandler::FlushStream()
{
   fflush(mStream);
}

int CInputHandler::DisableRawMode()
{
   int status = tcsetattr(STDIN_FILENO, TCSAFLUSH, &mStdinTermios);
   return status;
}

int CInputHandler::EnableRawMode()
{
   int status = tcgetattr(STDIN_FILENO, &mStdinTermios);

   setvbuf(stdin, nullptr, _IONBF, 0);

   if (status < 0)
      return status;

   struct termios raw = mStdinTermios;
   raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
   raw.c_oflag &= ~(OPOST);
   raw.c_cflag |= (CS8);
   raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
   raw.c_cc[VMIN] = 0;
   raw.c_cc[VTIME] = 0;

   // TODO: What is the difference between this and cfmakeraw?
   // cfmakeraw(&raw);
   // raw.c_cc[VMIN] = 0;
   // raw.c_cc[VTIME] = 0;

   status = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

   return status;
}

int CInputHandler::ReadInput()
{
   int nread;
   char c;

   nread = read(STDIN_FILENO, &c, 1);

   if (nread <= 0)
      return nread;

   if (c == '\x1b')
   {
      char seq[3];

      if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
      if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

      if (seq[0] == '[')
      {
         if (seq[1] >= '0' && seq[1] <= '9')
         {
            if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
            if (seq[2] == '~')
            {
               switch (seq[1])
               {
                  case '1': return KEY_HOME;
                  case '3': return KEY_DEL;
                  case '4': return KEY_END;
                  case '5': return KEY_PAGE_UP;
                  case '6': return KEY_PAGE_DOWN;
                  case '7': return KEY_HOME;
                  case '8': return KEY_END;
               }
            }
         }
         else
         {
            switch (seq[1])
            {
               case 'A': return KEY_ARROW_UP;
               case 'B': return KEY_ARROW_DOWN;
               case 'C': return KEY_ARROW_RIGHT;
               case 'D': return KEY_ARROW_LEFT;
               case 'H': return KEY_HOME;
               case 'F': return KEY_END;
            }
         }
      }
      else if (seq[0] == 'O')
      {
         switch (seq[1])
         {
            case 'H': return KEY_HOME;
            case 'F': return KEY_END;
         }
      }

      return 0;
   }
   else
   {
      return c;
   }
}

void CInputHandler::ProcessKeyPress(int Key)
{
   switch (Key)
   {
      case KEY_CTRL_C:
         // TODO: Handle Ctrl-C
         break;
      case KEY_BACKSPACE:
      {
         ClearPrompt();
         RemoveCharFromLine();
         PutLine();
         break;
      }
      case KEY_ENTER:
      {
         bool add_to_history = false;

         if (strlen(mLine) > 0)
         {
            if (mHistory.Size() > 0)
            {
               if (strcmp(mLine, (const char*)mHistory.PeekAt(mHistory.Size()-1)) != 0)
               {
                  add_to_history = true;
               }
            }
            else
            {
               add_to_history = true;
            }
         }

         if (add_to_history)
         {
            mHistory.PushBack(&mLine);
         }
         else if (mHistory.Size() > 0)
         {
            // copy last command into line
            uint32_t    hist_size = mHistory.Size();
            const char* line = (const char*)mHistory.PeekAt(hist_size-1);
            if (line)
            {
               CopyToLine(line);
               PutLine();
            }
         }
         mCursorCol = 0;
         mHistoryIndex = mHistory.Size();
         break;
      }
      case KEY_ARROW_DOWN:
         if (mHistoryIndex < mHistory.Size())
         {
            mHistoryIndex++;
            CopyToLine((const char*)mHistory.PeekAt(mHistoryIndex));
            ClearPrompt();
            PutLine();
         }
         else
         {
            ClearLine();
            ClearPrompt();
            PutLine();
         }
         break;
      case KEY_ARROW_UP:
         if (mHistoryIndex > 0)
         {
            mHistoryIndex--;
            CopyToLine((const char*)mHistory.PeekAt(mHistoryIndex));
            ClearPrompt();
            PutLine();
         }
         break;
      case KEY_ARROW_LEFT:
         if (mCursorCol > 0)
         {
            mCursorCol--;
            PutLine();
         }
         break;
      case KEY_ARROW_RIGHT:
         if (mCursorCol < strlen(mLine))
         {
            mCursorCol++;
            PutLine();
         }
         break;
      case KEY_HOME:
         mCursorCol = 0;
         PutLine();
         break;
      case KEY_END:
         mCursorCol = strlen(mLine);
         PutLine();
         break;
      default:
      {
         AddCharToLine(Key);
         PutLine();
         break;
      }
   }
}
