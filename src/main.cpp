
//#include "gui.cpp"
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <termios.h>

#include "PrintData.cpp"
#include "RingBuffer.h"

const char* clear_line = "                                                                            ";

bool done = false;
struct termios orig_stdin_termios;
CRingBuffer<char[128], 50> ring;
int history_index = 0;
std::string line = "";

int DisableRawMode()
{
   int status = tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_stdin_termios);
   return status;
}

int EnableRawMode()
{
   int status = tcgetattr(STDIN_FILENO, &orig_stdin_termios);

   setvbuf(stdin, nullptr, _IONBF, 0);

   if (status < 0)
      return status;

   struct termios raw = orig_stdin_termios;
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

int ReadInput()
{
   int nread;
   char c;

   nread = read(STDIN_FILENO, &c, 1);

   if (nread < 0)
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

void ProcessKeyPress(int Key)
{
   switch (Key)
   {
      case KEY_CTRL_C:
         done = true;
         break;
      case KEY_BACKSPACE:
      {
         size_t len = line.length();
         if (len > 0)
            line.erase(line.begin()+len-1);
         printf("\r>%s", clear_line);
         printf("\r>%s", line.c_str());
         break;
      }
      case KEY_ENTER:
      {
         bool add_to_history = false;

         if (line.length() > 0)
         {
            if (ring.Size() > 0)
            {
               if (line != (char*)ring.PeekAt(ring.Size()-1))
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
            ring.PushBack(line.c_str());
         history_index = ring.Size();
         line = "";
         printf("\r\nEnter something\r\n>");
         break;
      }
      case KEY_ARROW_DOWN:
         if (history_index < ring.Size()-1)
         {
            history_index++;
            line = (char*)ring.PeekAt(history_index);
            printf("\r>%s", clear_line);
            printf("\r>%s", line.c_str());
         }
         else
         {
            line = "";
            printf("\r>%s", clear_line);
            printf("\r>%s", line.c_str());
         }
         break;
      case KEY_ARROW_UP:
         if (history_index > 0)
         {
            line = (char*)ring.PeekAt(history_index-1);
            history_index--;
            printf("\r>%s", clear_line);
            printf("\r>%s", line.c_str());
         }
         break;
      case KEY_ARROW_LEFT:
      case KEY_ARROW_RIGHT:
         break;
      default:
         line += (char)Key;
         printf("\r>%s", line.c_str());
   }

   fflush(stdout);
}

int main(int argc, char** argv)
{
   EnableRawMode();

   printf("Enter something\r\n>");

   std::this_thread::sleep_for(std::chrono::milliseconds(10));
   ReadInput();

   while (!done)
   {
      int key = ReadInput();

      if (key > 0)
      {
         ProcessKeyPress(key);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }

   DisableRawMode();

#if 0
   if (gui_init("Debugger"))
   {
      gui_run();
   }

   gui_shutdown();
#endif

   return 0;
}
