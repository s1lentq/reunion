#include "precompiled.h"

uint8* CSizeBuf::GetSpace(unsigned int len) {
	if (m_bOverflowed) {
		return NULL;
	}

	if (m_CurSize + len + BITS_WRITING_RESERVE > m_MaxSize) {
		m_bOverflowed = true;
		return NULL;
	}

	uint8* res = m_Data + m_CurSize;
	m_CurSize += len;
	return res;
}

bool CSizeBuf::Write(const void *data, unsigned int length) {
	ensureNoBitsOp();
	uint8* buf = GetSpace(length);
	if (!buf)
		return false;

	memcpy(buf, data, length);
	return true;
}

bool CSizeBuf::Print(const char *data) {
	ensureNoBitsOp();
	return Write(data, strlen(data) + 1);
}

bool CSizeBuf::WriteChar(int8 val) {
	return WritePrimitive(val);
}

bool CSizeBuf::WriteByte(uint8 val) {
	return WritePrimitive(val);
}

bool CSizeBuf::WriteShort(int16 val) {
	return WritePrimitive(val);
}

bool CSizeBuf::WriteWord(uint16 val) {
	return WritePrimitive(val);
}

bool CSizeBuf::WriteDWord(uint32 val) {
	return WritePrimitive(val);
}

bool CSizeBuf::WriteUint64(uint64 val) {
	return WritePrimitive(val);
}

bool CSizeBuf::WriteLong(int32 val) {
	return WritePrimitive(val);
}

bool CSizeBuf::WriteFloat(float val) {
	return WritePrimitive(val);
}

bool CSizeBuf::WriteString(const char* s) {
	if (!s) s = "";
	return Write(s, strlen(s) + 1);
}

#ifdef SIZEBUF_BITS
const uint32 BITTABLE[] =
{
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x00000000,
};

const uint32 ROWBITTABLE[] =
{
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
	0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
	0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF,
	0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
	0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
	0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
	0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF,
	0xFFFFFFFF,
};

const uint32 INVBITTABLE[] =
{
	0xFFFFFFFE, 0xFFFFFFFD, 0xFFFFFFFB, 0xFFFFFFF7,
	0xFFFFFFEF, 0xFFFFFFDF, 0xFFFFFFBF, 0xFFFFFF7F,
	0xFFFFFEFF, 0xFFFFFDFF, 0xFFFFFBFF, 0xFFFFF7FF,
	0xFFFFEFFF, 0xFFFFDFFF, 0xFFFFBFFF, 0xFFFF7FFF,
	0xFFFEFFFF, 0xFFFDFFFF, 0xFFFBFFFF, 0xFFF7FFFF,
	0xFFEFFFFF, 0xFFDFFFFF, 0xFFBFFFFF, 0xFF7FFFFF,
	0xFEFFFFFF, 0xFDFFFFFF, 0xFBFFFFFF, 0xF7FFFFFF,
	0xEFFFFFFF, 0xDFFFFFFF, 0xBFFFFFFF, 0x7FFFFFFF,
	0xFFFFFFFF,
};

void CSizeBuf::StartBitWriting() {
	ensureNoBitsOp();

	m_BFWrite.nCurOutputBit = 0;
	m_BFWrite.pOutByte = m_Data + m_CurSize;
	m_bBitsWriting = true;
}

void CSizeBuf::EndBitWriting() {
	ensureBitsWriting();
	m_bBitsWriting = false;

	if (m_bOverflowed) {
		return;
	}

	*m_BFWrite.pOutByte &= 255 >> (8 - m_BFWrite.nCurOutputBit);
	GetSpace(1);
	m_BFWrite.nCurOutputBit = 0;
	m_BFWrite.pOutByte = 0;
}

void CSizeBuf::WriteOneBit(int val) {
	ensureBitsWriting();

	if (m_BFWrite.nCurOutputBit >= 8)
	{
		m_BFWrite.pOutByte = GetSpace(1);
		m_BFWrite.nCurOutputBit = 0;
	}

	if (m_bOverflowed)
		return;

	if (val)	{
		*m_BFWrite.pOutByte |= BITTABLE[m_BFWrite.nCurOutputBit];
	} else {
		*m_BFWrite.pOutByte &= INVBITTABLE[m_BFWrite.nCurOutputBit * 4];
	}

	m_BFWrite.nCurOutputBit++;
}

void CSizeBuf::WriteBits(uint32 data, unsigned int numbits) {
	ensureBitsWriting();
	if (numbits < 32)
	{
		if (data >= (uint32)(1 << numbits))
			data = ROWBITTABLE[numbits];
	}

	int surplusBytes = 0;
	if ((uint32)m_BFWrite.nCurOutputBit >= 8)
	{
		surplusBytes = 1;
		m_BFWrite.nCurOutputBit = 0;
		++m_BFWrite.pOutByte;
	}

	int bits = numbits + m_BFWrite.nCurOutputBit;
	if (bits <= 32)
	{
		int bytesToWrite = bits >> 3;
		int bitsLeft = bits & 7;
		if (!bitsLeft)
			--bytesToWrite;
		GetSpace(surplusBytes + bytesToWrite);
		if (!m_bOverflowed)
		{
			*(uint32 *)m_BFWrite.pOutByte = (data << m_BFWrite.nCurOutputBit) | *(uint32 *)m_BFWrite.pOutByte & ROWBITTABLE[m_BFWrite.nCurOutputBit];
			m_BFWrite.nCurOutputBit = 8;
			if (bitsLeft)
				m_BFWrite.nCurOutputBit = bitsLeft;
			m_BFWrite.pOutByte = &m_BFWrite.pOutByte[bytesToWrite];
		}
	}
	else
	{
		GetSpace(surplusBytes + 4);
		if (!m_bOverflowed)
		{
			*(uint32 *)m_BFWrite.pOutByte = (data << m_BFWrite.nCurOutputBit) | *(uint32 *)m_BFWrite.pOutByte & ROWBITTABLE[m_BFWrite.nCurOutputBit];
			int leftBits = 32 - m_BFWrite.nCurOutputBit;
			m_BFWrite.nCurOutputBit = bits & 7;
			m_BFWrite.pOutByte += 4;
			*(uint32 *)m_BFWrite.pOutByte = data >> leftBits;
		}
	}
}

void CSizeBuf::WriteSBits(int data, unsigned int numbits) {
	ensureBitsWriting();
	int idata = data;

	if (numbits < 32)
	{
		int maxnum = (1 << (numbits - 1)) - 1;

		if (data > maxnum || (maxnum = -maxnum, data < maxnum))
		{
			idata = maxnum;
		}
	}

	int sigbits = idata < 0;

	WriteOneBit(sigbits);
	WriteBits(abs(idata), numbits - 1);
}

void CSizeBuf::WriteBitString(const char *p) {
	ensureBitsWriting();
	const char *pch = p;

	while (*pch) {
		WriteBits(*pch, 8);
		++pch;
	}

	WriteBits(0, 8);
}

void CSizeBuf::WriteBitData(void *src, unsigned int length) {
	ensureBitsWriting();
	uint8 *p = (uint8 *)src;

	for (unsigned int i = 0; i < length; i++, p++) {
		WriteBits(*p, 8);
	}
}

void CSizeBuf::StartBitReading() {
	ensureNoBitsOp();
	m_BFRead.nCurInputBit = 0;
	m_BFRead.nBytesRead = 0;
	m_BFRead.nBitFieldReadStartByte = m_ReadCount;
	m_BFRead.pInByte = m_Data + m_ReadCount;
	m_BFRead.nMsgReadCount = m_ReadCount + 1;

	if (m_ReadCount + 1 > m_CurSize) {
		m_bBadRead = true;
	}

	m_bBitsReading = true;
}

void CSizeBuf::EndBitReading() {
	ensureBitsReading();

	if (m_BFRead.nMsgReadCount > m_CurSize) {
		m_bBadRead = true;
	}

	m_ReadCount = m_BFRead.nMsgReadCount;
	m_BFRead.nBitFieldReadStartByte = 0;
	m_BFRead.nCurInputBit = 0;
	m_BFRead.nBytesRead = 0;
	m_BFRead.pInByte = 0;

	m_bBitsReading = false;
}

int CSizeBuf::CurrentBit() {
	ensureBitsReading();

	return m_BFRead.nCurInputBit + 8 * m_BFRead.nBytesRead;
}

int CSizeBuf::ReadOneBit() {
	int nValue;

	if (m_bBadRead)	{
		return 1;
	}

	if (m_BFRead.nCurInputBit >= 8)
	{
		++m_BFRead.nMsgReadCount;
		m_BFRead.nCurInputBit = 0;
		++m_BFRead.nBytesRead;
		++m_BFRead.pInByte;
	}

	if (m_BFRead.nMsgReadCount <= m_CurSize)
	{
		nValue = (*m_BFRead.pInByte & BITTABLE[m_BFRead.nCurInputBit]) != 0;
		++m_BFRead.nCurInputBit;
	}
	else
	{
		nValue = 1;
		m_bBadRead = true;
	}

	return nValue;
}

uint32 CSizeBuf::ReadBits(unsigned int numbits) {
	uint32 result;

	if (m_bBadRead)	{
		return 1;
	}

	if (m_BFRead.nCurInputBit >= 8)
	{
		++m_BFRead.nMsgReadCount;
		++m_BFRead.nBytesRead;
		++m_BFRead.pInByte;

		m_BFRead.nCurInputBit = 0;
	}

	uint32 bits = (m_BFRead.nCurInputBit + numbits) & 7;

	if ((unsigned int)(m_BFRead.nCurInputBit + numbits) <= 32) {
		result = (*(unsigned int *)m_BFRead.pInByte >> m_BFRead.nCurInputBit) & ROWBITTABLE[numbits];

		uint32 bytes = (m_BFRead.nCurInputBit + numbits) >> 3;

		if (bits) {
			m_BFRead.nCurInputBit = bits;
		} else {
			m_BFRead.nCurInputBit = 8;
			bytes--;
		}

		m_BFRead.pInByte += bytes;
		m_BFRead.nMsgReadCount += bytes;
		m_BFRead.nBytesRead += bytes;
	}
	else
	{
		result = ((*(unsigned int *)(m_BFRead.pInByte + 4) & ROWBITTABLE[bits]) << (32 - m_BFRead.nCurInputBit)) | (*(unsigned int *)m_BFRead.pInByte >> m_BFRead.nCurInputBit);
		m_BFRead.nCurInputBit = bits;
		m_BFRead.pInByte += 4;
		m_BFRead.nMsgReadCount += 4;
		m_BFRead.nBytesRead += 4;
	}

	if (m_BFRead.nMsgReadCount > m_CurSize)
	{
		result = 1;
		m_bBadRead = true;
	}

	return result;
}

int CSizeBuf::ReadSBits(unsigned int numbits) {
	int nSignBit = ReadOneBit();
	int result = ReadBits(numbits - 1);

	if (nSignBit) {
		result = -result;
	}

	return result;
}
#endif // SIZEBUF_BITS

CSizeBuf::CSizeBuf(const uint8* buf, unsigned int maxSize, unsigned int curSize) {
	Clear();
	m_Data = (uint8_t *)buf;
	m_MaxSize = maxSize;
	m_CurSize = curSize;
}

void CSizeBuf::Clear() {
	m_CurSize = m_ReadCount = 0;
	m_bOverflowed = m_bBadRead = false;
#ifdef SIZEBUF_BITS
	m_bBitsWriting = m_bBitsReading = false;

	memset(&m_BFRead, 0, sizeof(m_BFRead));
	memset(&m_BFWrite, 0, sizeof(m_BFWrite));
#endif // SIZEBUF_BITS
}

uint32 CSizeBuf::ReadDWord() {
	return ReadPrimitive<uint32>();
}

uint64 CSizeBuf::ReadUint64() {
	return ReadPrimitive<uint64>();
}

int32 CSizeBuf::ReadLong() {
	return ReadPrimitive<int32>();
}

int8 CSizeBuf::ReadChar() {
	return ReadPrimitive<int8>();
}

uint8 CSizeBuf::ReadByte() {
	return ReadPrimitive<uint8>();
}

int16 CSizeBuf::ReadShort() {
	return ReadPrimitive<int16>();
}

uint16 CSizeBuf::ReadWord() {
	return ReadPrimitive<uint16>();
}

char* CSizeBuf::ReadString()
{
	int c = 0, l = 0;
	static char string[8192];

	while ((c = ReadChar(), c) && c != -1 && (unsigned)l < ARRAYSIZE(string) - 1) {
		string[l++] = c;
	}
	string[l] = 0;

	return string;
}

char* CSizeBuf::ReadStringLine() {
	int c = 0, l = 0;
	static char string[8192];

	while ((c = ReadChar(), c) && c != -1 && (unsigned)l < ARRAYSIZE(string) - 1) {
		string[l++] = c;
	}
	string[l] = 0;

	return string;
}

void CSizeBuf::Cut(unsigned int cutpos, unsigned int cutlen) {
	if (cutpos + cutlen > m_CurSize) {
		util_syserror("CSizeBuf::Cut: Invalid cut request; cutpos=%u, cutlen=%u, curSize=%u", cutpos, cutlen, m_CurSize);
		return;
	}

	uint8* wpos = m_Data + cutpos;
	uint8* rpos = m_Data + cutpos + cutlen;
	unsigned int moveLen = m_CurSize - (cutpos + cutlen);
	if (moveLen) {
		memmove(wpos, rpos, moveLen);
	}
	m_CurSize -= cutlen;
}
