
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <thread>
#include "InputHandler.h"

FILE* log_file = nullptr;

CInputHandler::CInputHandler(const char* Prompt, FILE* Stream)
   : mStdinTermios{},
     mHistory(),
     mHistoryIndex(0),
     mLine{},
     mPrompt(Prompt),
     mStream(Stream)
{
   EnableRawMode();

   log_file = fopen("log.txt", "w");
}

CInputHandler::~CInputHandler()
{
   DisableRawMode();

   fclose(log_file);
}

int CInputHandler::GetInput(char* String)
{
   int key;
   int i;
   int output_length;

   memset(mLine, 0, sizeof(mLine));
   fprintf(log_file, "print prompt [%s] line [%s]\n", mPrompt, mLine);
   fflush(log_file);
   PutLine();

   do
   {
      key = ReadInput();

      if (key > 0)
      {
         fprintf(log_file, "got key %x, line [%s]\n", key, mLine);
         fflush(log_file);
         ProcessKeyPress(key);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));

   } while (key != KEY_ENTER);

   output_length = strlen(mLine);

   // fprintf(log_file, "got line [%s] %d\n", mLine, output_length);
   // fflush(log_file);

   for (i = 0; i < output_length; i++)
   {
      String[i] = mLine[i];
   }
   String[i] = 0;

   PutLine("\r\n");

   return output_length;
}

void CInputHandler::ClearLine()
{
   fprintf(mStream, "\r%s", mPrompt);
   for (int i = 0; i < MAX_LINE; i++)
   {
      fprintf(mStream, " ");
   }
}

void CInputHandler::PutLine(const char* Output)
{
   if (Output)
      fprintf(mStream, Output);
   else
      fprintf(mStream, "\r%s%s", mPrompt, mLine);
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
         size_t len = strlen(mLine);
         if (len > 0)
            mLine[len-1] = 0;
         ClearLine();
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
               if (mLine != (char*)mHistory.PeekAt(mHistory.Size()-1))
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
               int i;
               for (i = 0; i < strlen(line); i++)
               {
                  mLine[i] = line[i];
               }
               mLine[i] = 0;
               PutLine();
            }
         }
         mHistoryIndex = mHistory.Size();
         break;
      }
      case KEY_ARROW_DOWN:
         if (mHistoryIndex < mHistory.Size()-1)
         {
            mHistoryIndex++;
            const char* line = (const char*)mHistory.PeekAt(mHistoryIndex);
            if (line)
            {
               int i;
               for (i = 0; i < strlen(line); i++)
               {
                  mLine[i] = line[i];
               }
               mLine[i] = 0;
            }
            else
            {
               memset(mLine, 0, sizeof(mLine));
            }
            ClearLine();
            PutLine();
         }
         else
         {
            memset(mLine, 0, sizeof(mLine));
            ClearLine();
            PutLine();
         }
         break;
      case KEY_ARROW_UP:
         if (mHistoryIndex > 0)
         {
            mHistoryIndex--;
            const char* line = (const char*)mHistory.PeekAt(mHistoryIndex);
            if (line)
            {
               int i;
               for (i = 0; i < strlen(line); i++)
               {
                  mLine[i] = line[i];
               }
               mLine[i] = 0;
            }
            else
            {
               memset(mLine, 0, sizeof(mLine));
            }
            ClearLine();
            PutLine();
         }
         break;
      case KEY_ARROW_LEFT:
      case KEY_ARROW_RIGHT:
         break;
      default:
      {
         size_t len = strlen(mLine);
         if (len < MAX_LINE-1)
            mLine[len] = (char)Key;
         PutLine();
         break;
      }
   }
}