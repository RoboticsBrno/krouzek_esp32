#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QOpenGLWindow>
#include <QOpenGLExtraFunctions>
#include <imgui.h>
#include <QSerialPortInfo>
#include <QMutex>
#include <QThread>

#include <vector>

class MainWindow : public QOpenGLWindow, private QOpenGLExtraFunctions
{
    Q_OBJECT

signals:
    void serialPortSelected(QSerialPortInfo info);
    void rateChanged(int rateMs);

public:
    MainWindow();
    ~MainWindow();

    void initializeGL() override;
    void paintGL() override;

private slots:
    void updateSerialPorts();
    void onPlotValue(double val);

private:
    bool show_test_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(114, 144, 154);

    QList<QSerialPortInfo> m_serialPorts;
    std::string m_selectedPort;

    QThread m_portReaderThread;

    std::vector<float> m_plotValues;
};
#endif // MAINWINDOW_H
