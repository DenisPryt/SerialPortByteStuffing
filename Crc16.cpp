#include "Crc16.h"

const QVector< ushort > Crc16::table = Crc16::initTable();

ushort Crc16::ComputeChecksum(const QByteArray bytes)
{
    ushort crc = 0;
    for (int i = 0; i < bytes.size(); ++i)
    {
        uchar index = (uchar)(crc ^ bytes[i]);
        crc = (ushort)((crc >> 8) ^ table[index]);
    }
    return crc;
}

QByteArray Crc16::ComputeChecksumBytes(const QByteArray bytes)
{
    ushort crc = ComputeChecksum( bytes );
    QByteArray byteRes( 2, 0 );
    byteRes[ 0 ] = static_cast<char>( crc );
    byteRes[ 1 ] = static_cast<char>( crc >> 8 );
    return byteRes;
}

QVector<ushort> Crc16::initTable()
{
    QVector<ushort> table;
    table.resize( 256 );
    for (ushort i = 0; i < table.size(); ++i)
    {
        ushort value = 0;
        ushort temp = i;
        for (uchar j = 0; j < 8; ++j)
        {
            if ( ((value ^ temp) & 0x0001) ){
                value = (ushort)((value >> 1) ^ POLYNOM);
            }
            else{
                value >>= 1;
            }
            temp >>= 1;
        }
        table[i] = value;
    }
    return table;
}
