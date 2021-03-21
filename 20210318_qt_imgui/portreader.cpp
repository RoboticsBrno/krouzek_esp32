#include <QSerialPort>
#include <QDebug>
#include "portreader.h"
#include <QDataStream>

PortReader::PortReader(QObject *parent) : QObject(parent), m_port(nullptr)
{

}

PortReader::~PortReader() {

}


void PortReader::onSerialPortSelected(QSerialPortInfo info) {
    if(m_port) {
        delete m_port;
    }

    m_port = new QSerialPort(info, this);
    m_port->setBaudRate(115200);
    if(!m_port->open(QSerialPort::ReadWrite)) {
        qWarning() << "Failed to open port";
        delete m_port;
        m_port = nullptr;
        return;
    }

    connect(m_port, &QSerialPort::readyRead, this, &PortReader::portReadyRead);
}

void PortReader::portReadyRead() {

    static const QByteArray pktStart("\xFF\x01");
    while(true) {
        auto cur = m_port->read(16*1024);
        if(cur.size() == 0)
            break;

        m_buffer.append(cur);
        int idx = m_buffer.indexOf(pktStart);
        while(idx != -1) {
            m_buffer = m_buffer.mid(idx);
            if(m_buffer.size() < 4)
                break;

            uint8_t cmd = m_buffer.at(2);
            uint8_t size = m_buffer.at(3);
            if(m_buffer.size() < 4 + size)
                break;

            if(cmd == 1) {
                double *cosValPtr = (double*)(m_buffer.data() + 4);
                emit cosReceived(*cosValPtr);
            }

            m_buffer = m_buffer.mid(4 + size);
            idx = m_buffer.indexOf(pktStart);
        }
    }
}

void PortReader::onMsRateChanged(int msVal) {
    if(!m_port || !m_port->isOpen()) {
        return;
    }

    QByteArray cmd;
    cmd.push_back(0xFF);
    cmd.push_back(1); //device id
    cmd.push_back(uint8_t(0)); // command
    cmd.push_back(4 + 8); // size

    QDataStream str(&cmd, QIODevice::ReadWrite | QIODevice::Append);
    str.setByteOrder(QDataStream::LittleEndian);
    str << uint32_t(msVal);
    str << double(0.1);

    m_port->write(cmd);
}
