#include "IOutputStream.h"

IOutputStream::~IOutputStream()
{
}

void IOutputStream::WriteBool(bool Val)
{
    char ChrVal = (Val ? 1 : 0);
    WriteBytes(&ChrVal, 1);
}

void IOutputStream::WriteByte(char Val)
{
    WriteBytes(&Val, 1);
}

void IOutputStream::WriteShort(short Val)
{
    if (mDataEndianness != IOUtil::kSystemEndianness) IOUtil::SwapBytes(Val);
    WriteBytes(&Val, 2);
}

void IOutputStream::WriteLong(long Val)
{
    if (mDataEndianness != IOUtil::kSystemEndianness) IOUtil::SwapBytes(Val);
    WriteBytes(&Val, 4);
}

void IOutputStream::WriteLongLong(long long Val)
{
    if (mDataEndianness != IOUtil::kSystemEndianness) IOUtil::SwapBytes(Val);
    WriteBytes(&Val, 8);
}

void IOutputStream::WriteFloat(float Val)
{
    if (mDataEndianness != IOUtil::kSystemEndianness) IOUtil::SwapBytes(Val);
    WriteBytes(&Val, 4);
}

void IOutputStream::WriteDouble(double Val)
{
    if (mDataEndianness != IOUtil::kSystemEndianness) IOUtil::SwapBytes(Val);
    WriteBytes(&Val, 8);
}

void IOutputStream::WriteFourCC(long Val)
{
    if (IOUtil::kSystemEndianness == IOUtil::eLittleEndian) IOUtil::SwapBytes(Val);
    WriteBytes(&Val, 4);
}

void IOutputStream::WriteString(const TString& rkVal)
{
    WriteBytes(rkVal.Data(), rkVal.Size());

    if (rkVal.IsEmpty() || rkVal.Back() != '\0')
        WriteByte(0);
}

void IOutputStream::WriteString(const TString& rkVal, u32 Count, bool Terminate)
{
    WriteBytes(rkVal.Data(), Count);

    if (Terminate && (Count == 0 || rkVal.Back() != '\0'))
        WriteByte(0);
}

void IOutputStream::WriteSizedString(const TString& rkVal)
{
    WriteLong(rkVal.Size());
    WriteBytes(rkVal.Data(), rkVal.Size());
}

void IOutputStream::WriteWideString(const TWideString& rkVal)
{
    WriteBytes(rkVal.Data(), rkVal.Size() * 2);

    if (rkVal.IsEmpty() || rkVal.Back() != '\0')
        WriteShort(0);
}

void IOutputStream::WriteWideString(const TWideString& rkVal, u32 Count, bool Terminate)
{
    WriteBytes(rkVal.Data(), Count * 2);

    if (Terminate && (Count == 0 || rkVal.Back() != 0))
        WriteShort(0);
}

void IOutputStream::WriteSizedWideString(const TWideString& rkVal)
{
    WriteLong(rkVal.Size());
    WriteBytes(rkVal.Data(), rkVal.Size() * 2);
}

bool IOutputStream::GoTo(u32 Address)
{
    return Seek(Address, SEEK_SET);
}

bool IOutputStream::Skip(s32 SkipAmount)
{
    return Seek(SkipAmount, SEEK_CUR);
}

void IOutputStream::WriteToBoundary(u32 Boundary, u8 Fill)
{
    u32 Num = Boundary - (Tell() % Boundary);
    if (Num == Boundary) return;
    for (u32 iByte = 0; iByte < Num; iByte++)
        WriteByte(Fill);
}

void IOutputStream::SetEndianness(IOUtil::EEndianness Endianness)
{
    mDataEndianness = Endianness;
}

void IOutputStream::SetDestString(const TString& rkDest)
{
    mDataDest = rkDest;
}

IOUtil::EEndianness IOutputStream::GetEndianness() const
{
    return mDataEndianness;
}

TString IOutputStream::GetDestString() const
{
    return mDataDest;
}

bool IOutputStream::Seek64(s64 Offset, u32 Origin)
{
    return Seek((s32) Offset, Origin);
}

u64 IOutputStream::Tell64() const
{
    return (u64) (Tell());
}
