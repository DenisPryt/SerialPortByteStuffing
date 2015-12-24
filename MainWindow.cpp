#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QSerialPort>
#include <QDebug>
#include <QByteArray>
#include <Crc16.h>
#include <QDataStream>

const QByteArray MainWindow::_7E           = "~";//QChar( 0x01111110 );
const QByteArray MainWindow::_7D           = "}";//QChar( 0x01111101 );
const QByteArray MainWindow::_5E           = "^";//QChar( 0x01011110 );
const QByteArray MainWindow::_5D           = "]";//QChar( 0x01011101 );
const quint64 MainWindow::MaxPacketSize = 10;  //bytes without 2 ESC symbols

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->spinBoxMyAdderss->setValue( rand() % 100 );
    ui->incommingDataBrowser->setDisabled( true );

    m_serialPortRead = new QSerialPort( this );
    m_serialPortWrite = new QSerialPort( this );
    connect( ui->buttonConnect, &QPushButton::clicked, this, &MainWindow::onConnectButton );
    connect( ui->buttonSend,    &QPushButton::clicked, this, &MainWindow::onSendButton );
    connect( ui->buttonTransform, &QPushButton::clicked, this, &MainWindow::onTransformButton );
    connect( m_serialPortRead,      &QSerialPort::readyRead, this, &MainWindow::onReadyRead );

    m_repeatTransferCount = 0;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onConnectButton()
{
    QString portNameRead = ui->lineEditPortRead->text();
    m_serialPortRead->setPortName( portNameRead );
    if ( !m_serialPortRead->open( QIODevice::ReadOnly ) ){
        ui->logBrowser->append( "read port" + m_serialPortRead->errorString() );
    }
    else{
        ui->logBrowser->append( "read port connected" );
    }

    QString portNameWrite = ui->lineEditPortWrite->text();
    m_serialPortWrite->setPortName( portNameWrite );
    if ( !m_serialPortWrite->open( QIODevice::WriteOnly ) ){
        ui->logBrowser->append( "write port" + m_serialPortRead->errorString() );
    }
    else{
        ui->logBrowser->append( "write port connected" );
    }
}

QByteArray MainWindow::addEscSumbols(const QList<QByteArray> packets)
{
    QByteArray res = _7E;
    for ( int i = 0; i < packets.size(); ++i ){
        res += packets.at( i ) + _7E;
    }
    return res;
}

void MainWindow::onSendButton()
{
    if ( !m_serialPortWrite->isOpen() ){
        onConnectButton();
        if ( !m_serialPortWrite->isOpen() ){
            return;
        }
    }

    QByteArray transformData = QByteArray::fromHex( ui->stuffingDataBrowser->toPlainText().toUtf8() );
    m_serialPortWrite->write( transformData );
}

void MainWindow::onTransformButton()
{
    QByteArray transformDataStashL  = makeByteStuffing( ui->textEditStashL->toPlainText().toUtf8() );
    QByteArray transformDataStashR  = makeByteStuffing( ui->textEditStashR->toPlainText().toUtf8() );

    QByteArray dataMsg = ui->textEditData->toPlainText().toUtf8();
    QList< QByteArray > packets = makePackets( splitByMTU( dataMsg ) );
    QList< QByteArray > stuffing = makeByteStuffing( packets );

    QByteArray transformData = transformDataStashL + addEscSumbols( packets ) +
                               transformDataStashR;

    ui->stuffingDataBrowser->setPlainText( transformData.toHex() );
}

MainWindow::Packet MainWindow::parcePacket(const QByteArray packet)
{
    if ( packet.size() < 3 ){
        qDebug() << "Empty packet";
        return Packet();
    }
    QByteArray payload      = packet.left( packet.size() - 2 );

    Packet parsePacket;
    parsePacket.IncomCrc     = packet.right( 2 );
    parsePacket.CalcCrc      = Crc16::ComputeChecksumBytes( payload );
    parsePacket.SourceAddress= payload.mid( 0, 1 );
    parsePacket.DestAddress  = payload.mid( 1, 1 );
    parsePacket.Flags        = payload.mid( 2, 1 )[0];
    parsePacket.Data         = payload.mid( 3 );

    return parsePacket;
}

void MainWindow::onReadyRead()
{
    QByteArray myAdr;
    myAdr.append( (char)ui->spinBoxMyAdderss->value() );

    QByteArray  incommingData           = m_serialPortRead->readAll();
    QList< QByteArray > transformedData = makeAntiByteStuffing( incommingData );

    QString htmlData;
    for ( int i = 0; i < transformedData.size(); ++i ){
        Packet packet = parcePacket( transformedData[ i ] );
        if ( packet.SourceAddress == myAdr ){
            switch( packet.Flags )
            {
            case EMPTY_FLAG  : ui->logBrowser->append( "No dest address in network" ); break;
            case OK_FLAG     : ui->logBrowser->append( "Transfer done" ); break;
            case ERROR_FLAG  : ui->logBrowser->append( "Transfer fail" );
                if ( m_repeatTransferCount++ > 2 ){
                    ui->logBrowser->append( "Too much transfers" );
                    m_repeatTransferCount = 0;
                }
                else{
                    onSendButton();
                }
                break;
            default          : ui->logBrowser->append( "Unknown flag" ); break;
            }

            return;
        }
        if ( packet.DestAddress != myAdr ){
            ui->logBrowser->append( "Alien packet" );

            QByteArray stuffingPacket = makeByteStuffing( packet.toBytes() );
            m_serialPortWrite->write( _7E + stuffingPacket + _7E );
            return;
        }
        else{ // myAdr == DestAdr
            if ( packet.IncomCrc == packet.CalcCrc ){
                htmlData += packet.Data;
                packet.Flags = OK_FLAG;
            }
            else{
                htmlData += "<font color=\"#CC0000\">" + packet.Data + "</font>";
                packet.Flags = ERROR_FLAG;
            }
            QByteArray stuffingPacket = makeByteStuffing( packet.toBytes() );
            m_serialPortWrite->write( _7E + stuffingPacket + _7E );
        }
    }

    ui->incommingDataBrowser->setPlainText( incommingData.toHex() );
    ui->stuffingDataBrowser->setHtml( htmlData );
}

QList<QByteArray> MainWindow::makePackets(QList<QByteArray> data)
{
    QByteArray sourceAdr;
    sourceAdr.append( (char)ui->spinBoxMyAdderss->value() );

    QByteArray destAdr;
    destAdr.append( (char)ui->spinBoxDestAddress->value() );

    QList< QByteArray > res;
    for ( int i = 0; i < data.size(); ++i ){
        QByteArray packet = sourceAdr + destAdr + (char)0 + data.at( i );
        QByteArray crc    = Crc16::ComputeChecksumBytes( packet );
        packet += crc;
        res << packet;
    }
    return res;
}

QList<QByteArray> MainWindow::splitByMTU(const QString dataToSplit)
{
    QList<QByteArray> dataRes;

    for ( int i = 0; i < dataToSplit.size(); i += MaxPacketSize ){
        dataRes << dataToSplit.mid( i, MaxPacketSize ).toUtf8();
    }

    return dataRes;
}

QList<QByteArray> MainWindow::makeByteStuffing(const QList<QByteArray> data)
{
    QList<QByteArray> res;
    for ( int i = 0; i < data.count(); ++i ){
        res << makeByteStuffing( data.at(i) );
    }
    return res;
}

QByteArray MainWindow::makeByteStuffing(const QByteArray data)
{
    QByteArray res = data;
    res.replace( _7D, _7D + _5D );
    res.replace( _7E, _7D + _5E );
    return res;
}

QList< QByteArray > MainWindow::makeAntiByteStuffing(const QByteArray data)
{
    QList< QByteArray > splitData = data.split( _7E[0] );
    if ( splitData.size() < 3 ){
        qDebug() << "Invalid data. ECS bytes count < 2";
        return QList< QByteArray >();
    }

    QList< QByteArray > outData;
    for ( int i = 1; i < splitData.size() - 1; ++i ){
        QByteArray packet = splitData[ i ];
        if ( packet.isEmpty() )
            continue;

        packet.replace( _7D + _5E, _7E );
        packet.replace( _7D + _5D, _7D );
        outData << packet;
    }

    return outData;
}
