#ifndef CRC16_H
#define CRC16_H

#include <QVector>
#include <QByteArray>

class Crc16
{
public:
    static ushort ComputeChecksum(const QByteArray bytes);
    static QByteArray ComputeChecksumBytes(const QByteArray bytes);

private:
    static const ushort POLYNOM = 0xA001;
    static const QVector< ushort > table;
    static QVector< ushort > initTable();
};

#endif // CRC16_H
