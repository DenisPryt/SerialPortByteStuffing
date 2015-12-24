#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class QSerialPort;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void onConnectButton();
    void onSendButton();
    void onTransformButton();
    void onReadyRead();

private :
    enum{
        EMPTY_FLAG = 0,
        OK_FLAG    = 1,
        ERROR_FLAG = 2
    };
    struct Packet{
        Packet() : Flags( EMPTY_FLAG ){}

        QByteArray IncomCrc;
        QByteArray CalcCrc;
        QByteArray DestAddress;
        QByteArray SourceAddress;
        QByteArray Data;
        quint8     Flags;

        QByteArray toBytes(){ return SourceAddress + DestAddress + (char)Flags + Data + IncomCrc; }
    };

    QList< QByteArray > makePackets( QList< QByteArray > data );

    static QList< QByteArray >  splitByMTU(const QString dataToSplit);
    static QList<QByteArray>    makeByteStuffing(const QList<QByteArray> data);
    static QByteArray           makeByteStuffing(const QByteArray data);
    static QList<QByteArray>    makeAntiByteStuffing(const QByteArray data);
    static QByteArray           addEscSumbols(const QList< QByteArray > packets);

    static Packet parcePacket( const QByteArray packet );

    Ui::MainWindow *ui;

    QSerialPort     *m_serialPortWrite;
    QSerialPort     *m_serialPortRead;
    int              m_repeatTransferCount;

    static const QByteArray _7E;
    static const QByteArray _7D;
    static const QByteArray _5E;
    static const QByteArray _5D;
    static const quint64 MaxPacketSize;
};

#endif // MAINWINDOW_H
