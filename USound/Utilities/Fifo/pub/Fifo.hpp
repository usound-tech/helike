//====================================================================
//
// COPYRIGHT 2019 All rights reserved.
//       USound
//
//====================================================================
//
//                          DISCLAIMER
// NO WARRANTIES
// USound expressly disclaims any warranty for the SOFTWARE
// PRODUCT. The SOFTWARE PRODUCT and any related documentation is
// provided "as is" without warranty of any kind, either expressed or
// implied, including, without limitation, the implied warranties or
// merchantability, fitness for a particular purpose, or noninfringe-
// ment. The entire risk arising out of the use or performance of the
// SOFTWARE PRODUCT remains with the user.
//
// NO LIABILITY FOR DAMAGES.
// Under no circumstances is USound liable for any damages
// whatsoever (including, without limitation, damages for loss of busi-
// ness profits, business interruption, loss of business information,
// or any other pecuniary loss) arising out of the use of or inability
// to use this product.
//
//====================================================================
//
//  Description: Generic fifo implementation
//  Filename: fifo.h
//  Author(s): Kumar Bala (kb@socfpga.io)
//  Date: 20-November-2019
//
//====================================================================

#pragma once

#include <stdint.h>
#include <string>
#include <cstring>
#include <tuple>

/**
 * An everyday boring fifo.
 *
 * @tparam T
 */
template<typename T>
class Fifo
{
protected:
  T *data;
  uint32_t maxSamples;
  volatile uint32_t wr;
  volatile uint32_t rd;

public:
  Fifo(T *data, uint32_t size) :
      data(data),
      maxSamples(size)
  {
    reset(nullptr, 0);
  }

  void reset(T *data, uint32_t samples)
  {
    if (data != nullptr)
    {
      this->data = data;
      this->maxSamples = samples;
    }

    wr = 0;
    rd = 0;
  }

  /**
   * Simply advances the wr pointer. Useful when handling DMA data.
   * @param bufferSize
   */
  void advanceBuffer(uint32_t bufferLoc)
  {
    wr = bufferLoc;

#if 0
    while (bufferSize)
    {
      uint32_t capacity = maxSamples - wr;
      uint32_t cp_len = capacity >= bufferSize ? bufferSize : capacity;

      bufferSize -= cp_len;
      wr += cp_len;
      if (wr >= maxSamples)
      {
        wr = 0;
      }
    }
#endif
  }

  /**
   * Advances the wr pointer and wraps it around when it gets past the fifo size.
   * @param bufferSize
   */
  void incrementBufferWr(uint32_t bufferSize)
  {
    wr += bufferSize;
    if (wr >= maxSamples)
    {
      wr = 0;
    }
  }

  /**
   * Simply advances the rd pointer. Useful when discarding data.
   * It doesn't check the wr, so ensure that you are dropping less samples than the stored ones.
   * @param bufferSize
   */
  void consumeBuffer(uint32_t bufferSize)
  {
    while (bufferSize)
    {
      uint32_t capacity = maxSamples - rd;
      uint32_t cp_len = capacity >= bufferSize ? bufferSize : capacity;

      bufferSize -= cp_len;
      rd += cp_len;
      if (rd >= maxSamples)
      {
        rd = 0;
      }
    }
  }

  void pushBuffer(const T *srcBuffer, uint32_t bufferSize)
  {
    uint32_t src_idx = 0;

    while (bufferSize)
    {
      uint32_t capacity = maxSamples - wr;
      uint32_t cp_len = capacity >= bufferSize ? bufferSize : capacity;

      memcpy(&data[wr], &srcBuffer[src_idx], cp_len * sizeof(T));

      bufferSize -= cp_len;
      src_idx += cp_len;

      wr += cp_len;
      if (wr >= maxSamples)
      {
        wr = 0;
      }
    }
  }

  void push(const T data)
  {
    data[wr++] = data;
    if (wr >= maxSamples)
    {
      wr = 0;
    }
  }

  uint32_t popBuffer(T *dstBuffer, uint32_t requestedSamples)
  {
    uint32_t fifoDataLen = getSampleCount();

    if (fifoDataLen < requestedSamples)
    {
      requestedSamples = fifoDataLen;
    }

    for (uint32_t i = 0; i < requestedSamples; i++)
    {
      dstBuffer[i] = data[rd++];
      if (rd >= maxSamples)
      {
        rd = 0;
      }
    }

    return requestedSamples;
  }

  T pop()
  {
    T result = data[rd++];

    if (rd >= maxSamples)
    {
      rd = 0;
    }

    return result;
  }

  /**
   * Returns the number of samples stored in the fifo
   * @return
   */
  uint32_t getSampleCount()
  {
    if (wr >= rd)
    {
      return wr - rd;
    }
    else
    {
      return wr - rd + maxSamples;
    }
  }

  /**
   * Returns the number of free slots in the fifo
   * @return
   */
  uint32_t getCapacity()
  {
    if (wr >= rd)
    {
      return maxSamples - wr + rd;
    }
    else
    {
      return rd - wr;
    }
  }

  /**
   * Transfers data from one fifo to the other, until the number of requested samples is transfered or the source fifo is drained.
   *
   * @param srcFifo
   * @param len
   * @return the remaining number of samples that have not been transfered.
   */
  uint32_t transferFromFifo(Fifo<T> *srcFifo, uint32_t len)
  {
    T sample;

    while (len)
    {
      if (srcFifo->getSampleCount() > 0)
      {
        sample = srcFifo->pop();
        push(sample);
        len--;
      }
      else
      {
        break;
      }
    }

    return len;
  }

  /**
   * Returns the memory address of the rd pointer
   * @return
   */
  T* getReadBufferPtr()
  {
    return &data[rd];
  }

  /**
   * Returns the memory address of the wr pointer
   * @return
   */
  T* getWriteBufferPtr()
  {
    return &data[wr];
  }
};

/**
 * A fifo that sits on top of a ping-pong DMA buffer
 * @tparam T
 */
template<typename T>
class PpFifo: public Fifo<T>
{
public:
  PpFifo(T *data, uint32_t size) :
      Fifo<T>(data, size)
  {
  }

  /**
   * Pushes the wr index to the other half to avoid the DMA engine reading from the buffer that is being updated
   * @param firstHalf when true, the wr pointer  will move to the beninning of the buffer
   */
  void advanceToHalf(bool toFirstHalf)
  {
    if (!toFirstHalf && (this->wr < (this->maxSamples / 2)))
    {
      this->wr = (this->maxSamples / 2);
    }
    else if (toFirstHalf && (this->wr >= (this->maxSamples / 2)))
    {
      this->wr = 0;
    }
  }

  /**
   * Pushes the rd index to the other half to avoid the DMA engine reading from the buffer that is being updated
   * @param firstHalf when true, the rd pointer will move to the beginning of the buffer
   */
  void advanceRxToHalf(bool toFirstHalf)
  {
    if (!toFirstHalf)
    {
      this->rd = (this->maxSamples / 2);
    }
    else if (toFirstHalf)
    {
      this->rd = 0;
    }
  }
};

/**
 * A boring fifo with a twist: it can store sequences of bytes as buffers and can retrieve any of them
 */
template<typename T, int N>
class BufferFifo: public Fifo<T>
{
protected:
  std::tuple<uint16_t, uint16_t> offsets[N];
  uint32_t oldestOffset;
  uint32_t newestOffset;

  void discardOldestBuffer()
  {
    if (oldestOffset == newestOffset)
    {
      return;
    }

    auto len = std::get<1>(offsets[oldestOffset]);
    this->consumeBuffer(len);

    oldestOffset++;
    if (oldestOffset >= N)
    {
      oldestOffset = 0;
    }
  }

public:
  BufferFifo(T *data, uint32_t size) :
      Fifo<T>(data, size),
      oldestOffset(0),
      newestOffset(0)
  {

  }

  /**
   * Returns the number of stored chunks
   * @return
   */
  uint32_t getChunkCount()
  {
    if (newestOffset >= oldestOffset)
    {
      return newestOffset - oldestOffset;
    }
    else
    {
      return newestOffset - oldestOffset + N;
    }
  }

  bool pushBufferChunk(const T *srcBuffer, uint32_t bufferSize)
  {
    if (bufferSize > this->maxSamples)
    {
      return false;
    }

    if (this->getChunkCount() >= (N - 1))
    {
      discardOldestBuffer();
    }

    while (this->getCapacity() < bufferSize)
    {
      discardOldestBuffer();
    }

    offsets[newestOffset] = std::make_tuple(this->wr, bufferSize);
    Fifo<T>::pushBuffer(srcBuffer, bufferSize);

    newestOffset++;
    if (newestOffset >= N)
    {
      newestOffset = 0;
    }

    return true;
  }

  /**
   * Returns the size of the N-th chunk (0 is the latest).
   * If the chunk doesn't exist, it'll return -1.
   * @param past
   * @return
   */
  int getChunkSize(uint32_t past)
  {
    if (getChunkCount() < past)
    {
      return -1;
    }

    uint32_t offset = (past > this->wr) ? (N - this->wr + past) : (this->wr - past);
    return std::get<1>(offsets[offset]);
  }

  /**
   * Copies the chunk data into the provided buffer
   * If the chunk doesn't exist, it'll return false.
   * @param past
   * @return
   */
  bool getChunk(uint32_t past, T *dstBuffer)
  {
    if (getChunkCount() < past)
    {
      return false;
    }

    uint32_t entryIdx = (past > this->wr) ? (N - this->wr + past) : (this->wr - past);
    uint16_t offset = std::get<0>(offsets[entryIdx]);
    uint16_t len = std::get<1>(offsets[entryIdx]);
    uint32_t dst_idx = 0;

    while (len)
    {
      uint32_t capacity = this->maxSamples - offset;
      uint32_t cp_len = capacity >= len ? len : capacity;

      memcpy(&dstBuffer[dst_idx], &this->data[offset], cp_len * sizeof(T));

      len -= cp_len;
      dst_idx += cp_len;

      offset += cp_len;
      if (offset >= this->maxSamples)
      {
        offset = 0;
      }
    }

    return true;
  }
};


template<int N>
class StringFifo: public BufferFifo<uint8_t, N>
{
public:
  StringFifo(uint8_t *data, uint32_t size) :
      BufferFifo<uint8_t, N>(data, size)
  {

  }

  /**
   * Copies the chunk data into the provided string
   * If the chunk doesn't exist, it'll return false.
   * @param past
   * @return
   */
  bool getChunkToString(uint32_t past, std::string &dstString)
  {
    if (this->getChunkCount() < past)
    {
      return false;
    }

    uint32_t entryIdx = (past >= this->newestOffset) ? (N + this->newestOffset - past - 1) : (this->newestOffset - past - 1);
    uint16_t offset = std::get<0>(this->offsets[entryIdx]);
    uint16_t len = std::get<1>(this->offsets[entryIdx]);

    while (len)
    {
      uint32_t capacity = this->maxSamples - offset;
      uint32_t cp_len = capacity >= len ? len : capacity;

      dstString.append((const char*) &this->data[offset], cp_len);

      len -= cp_len;
      offset += cp_len;
      if (offset >= this->maxSamples)
      {
        offset = 0;
      }
    }

    return true;
  }
};
