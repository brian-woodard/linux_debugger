#pragma once

template <typename T, int MAX>
class CRingBuffer
{
public:
   CRingBuffer() { Initialize(); }
   ~CRingBuffer() {}

   void Initialize()
   {
      // TODO: It's easier to let the head/tail indices get
      // arbitrarily large and just modulus them down to the range
      // we need. And putting them halfway makes it easier to modify
      // the front of the circular buffer without having off-by-1
      // issues. This is probably dumb, maybe fix later.

      uint32_t start_idx = UINT32_MAX/2;
      uint32_t offset    = start_idx % MAX;

      mHeadIdx = start_idx - offset;
      mTailIdx = start_idx - offset;
   }

#if 0
   void Print()
   {
      int size = Size();

      printf("\nHead:  %zu\n", mHeadIdx);
      printf("Tail:  %zu\n", mTailIdx);
      printf("Size:  %zu (max %zu)\n", size, MAX);
      printf("Bytes: %zu (max %zu)\n", sizeof(T)*size, sizeof(T)*MAX);

      for (uint32_t i = mHeadIdx; i < mHeadIdx + size; i++)
      {
         uint32_t idx = i % MAX;
         std::cout << (i - mHeadIdx) << ": " << mBuffer[idx] << std::endl;
      }

      printf("%s\n", CPrintData::GetDataAsString((char*)mBuffer, size*sizeof(T)));
   }
#endif

   uint32_t Size()
   {
      int32_t size = mTailIdx - mHeadIdx;

      if (size < 0)
         size = sizeof(mBuffer) + size + 1;

      return (uint32_t)size;
   }

   T* PeekAt(uint32_t Index)
   {
      T* result = nullptr;

      if (Index < Size())
      {
         uint32_t idx = (mHeadIdx + Index) % MAX;
         result = &mBuffer[idx];
      }

      return result;
   }

   T* PopBack()
   {
      T* result = nullptr;

      if (Size())
      {
         int idx = --mTailIdx % MAX;

         result = &mBuffer[idx];

         if (mTailIdx == mHeadIdx)
         {
            // buffer empty, reset indices to initial values
            Initialize();
         }
      }

      return result;
   }

   T* PopFront()
   {
      T* result = nullptr;

      if (Size())
      {
         int idx = mHeadIdx++ % MAX;

         result = &mBuffer[idx];

         if (mTailIdx == mHeadIdx)
         {
            // buffer empty, reset indices to initial values
            Initialize();
         }
      }

      return result;
   }

   void PushFront(const T Element)
   {
      if (mHeadIdx == 0) return;

      int idx = (--mHeadIdx % MAX);

      memcpy((char*)&mBuffer[idx], Element, sizeof(T));

      if (Size() > MAX)
      {
         mTailIdx--;
      }
   }

   void PushBack(const T Element)
   {
      int idx = (mTailIdx++ % MAX);

      memcpy((char*)&mBuffer[idx], Element, sizeof(T));

      if (Size() > MAX)
      {
         mHeadIdx++;
      }
   }

private:

   T mBuffer[MAX] = {};

   uint32_t mHeadIdx;
   uint32_t mTailIdx;
};
