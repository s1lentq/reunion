#pragma once

#include "reunion_shared.h"

class CSizeBuf {
private:
	struct bf_write_t
	{
		unsigned int nCurOutputBit;
		unsigned char *pOutByte;
	};

	struct bf_read_t
	{
		unsigned int nMsgReadCount;	// was msg_readcount
		unsigned int nBitFieldReadStartByte;
		unsigned int nBytesRead;
		unsigned int nCurInputBit;
		unsigned char *pInByte;
	};

#ifdef SIZEBUF_BITS
	static const unsigned int BITS_WRITING_RESERVE = 5;
#else
	static const unsigned int BITS_WRITING_RESERVE = 0;
#endif

private:
	uint8* m_Data;
	unsigned int m_MaxSize;
	unsigned int m_CurSize;
	unsigned int m_ReadCount;
	bool m_bOverflowed;
	bool m_bBadRead;
#ifdef SIZEBUF_BITS
	bool m_bBitsWriting;
	bool m_bBitsReading;

	bf_read_t m_BFRead;
	bf_write_t m_BFWrite;
#endif // SIZEBUF_BITS

	inline void ensureNoBitsOp() {
#if defined SIZEBUF_BITS && !defined NDEBUG
		if (m_bBitsWriting || m_bBitsReading) {
			util_syserror("Invalid BitsWriting/BitsReading mode");
		}
#endif
	}

	inline void ensureBitsWriting() {
#if defined SIZEBUF_BITS && !defined NDEBUG
		if (!m_bBitsWriting || m_bBitsReading) {
			util_syserror("Invalid BitsWriting/BitsReading mode");
		}
#endif
	}

	inline void ensureBitsReading() {
#if defined SIZEBUF_BITS && !defined NDEBUG
		if (m_bBitsWriting || !m_bBitsReading) {
			util_syserror("Invalid BitsWriting/BitsReading mode");
		}
#endif
	}

	template<typename T> inline bool WritePrimitive(T val) {
		ensureNoBitsOp();
		T* buf = (T*)GetSpace(sizeof(T));
		if (!buf)
			return false;

		*buf = val;
		return true;
	}

	template<typename T> inline T ReadPrimitive() {
		ensureNoBitsOp();
		T res;
		if (!m_bBadRead && m_ReadCount + sizeof(T) <= m_CurSize)
		{
			res = *(T *)(m_Data + m_ReadCount);
			m_ReadCount += sizeof(T);
		} else {
			m_bBadRead = true;
			res = (T) -1;
		}

		return res;
	}

public:
	CSizeBuf(const uint8* buf, unsigned int maxSize, unsigned int curSize = 0);

	uint8* GetSpace(unsigned int len);
	bool Write(const void *data, unsigned int length);
	bool Print(const char *data);

	bool WriteChar(int8 val);
	bool WriteByte(uint8 val);
	bool WriteShort(int16 val);
	bool WriteWord(uint16 val);
	bool WriteDWord(uint32 val);
	bool WriteUint64(uint64 val);
	bool WriteLong(int32 val);
	bool WriteFloat(float val);
	bool WriteString(const char* s);

#ifdef SIZEBUF_BITS
	void StartBitWriting();
	void EndBitWriting();
	void WriteOneBit(int val);
	void WriteBits(uint32 data, unsigned int numbits);
	void WriteSBits(int data, unsigned int numbits);
	void WriteBitString(const char *p);
	void WriteBitData(void *src, unsigned int length);
#endif // SIZEBUF_BITS

	void StartBitReading();
	void EndBitReading();
	int CurrentBit();
	int ReadOneBit();
	uint32 ReadBits(unsigned int numbits);
	int ReadSBits(unsigned int numbits);

	int8 ReadChar();
	uint8 ReadByte();
	int16 ReadShort();
	uint16 ReadWord();
	uint32 ReadDWord();
	uint64 ReadUint64();
	int32 ReadLong();
	char* ReadString();
	char* ReadStringLine();


	void Clear();
	void Cut(unsigned int cutpos, unsigned int cutlen);

	bool IsOverflowed() { return m_bOverflowed; }
	bool IsBadRead() { return m_bBadRead; }
	unsigned int GetCurSize() { return m_CurSize; }
	uint8* GetData() { return m_Data; }
	unsigned int GetMaxSize() { return m_MaxSize; }
	uint8* GetReadPtr() { return m_Data + m_ReadCount; }
	unsigned int GetReadPos() { return m_ReadCount; }
	unsigned int RemainingReadBytes() { return m_CurSize - m_ReadCount;  }
	bool HasSomethingToRead() { return m_ReadCount < m_CurSize; }
};
