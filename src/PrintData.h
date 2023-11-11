
#pragma once

class CPrintData
{
public:

   static constexpr int BYTES_PER_LINE = 8;
   static constexpr int CHARS_PER_LINE = 50;

   CPrintData() {}
   ~CPrintData() = default;

   static const char* GetDataAsString(char* Data, int Length);
   static const char* GetTimeAsString();

private:

   static char* mDataString;
   static int   mDataLength;

};
