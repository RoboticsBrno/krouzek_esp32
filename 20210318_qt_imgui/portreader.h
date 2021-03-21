#ifndef PORTREADER_H
#define PORTREADER_H

#include <QObject>
#include <QSerialPortInfo>
#include <QMutex>


class PortReader : public QObject
{
    Q_OBJECT
public:
    explicit PortReader(QObject *parent = nullptr);
    ~PortReader();

public slots:
    void onSerialPortSelected(QSerialPortInfo info);
    void portReadyRead();
    void onMsRateChanged(int msVal);

signals:
    void cosReceived(double val);

private:
    QSerialPort *m_port;

    QByteArray m_buffer;
};

#endif // PORTREADER_H
