#include <QtImGui.h>
#include <imgui.h>
#include <QTimer>
#include <QSerialPort>

#include "portreader.h"
#include "mainwindow.h"


MainWindow::MainWindow(): QOpenGLWindow()
{
    resize(1024, 600);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateSerialPorts(void)));
    timer->start(5000);

    qRegisterMetaType<QSerialPortInfo>("QSerialPortInfo");

    updateSerialPorts();

    PortReader *r = new PortReader;
    r->moveToThread(&m_portReaderThread);
    connect(&m_portReaderThread, &QThread::finished, r, &PortReader::deleteLater);

    connect(this, &MainWindow::serialPortSelected, r, &PortReader::onSerialPortSelected, Qt::QueuedConnection);

    connect(r, &PortReader::cosReceived, this, &MainWindow::onPlotValue, Qt::QueuedConnection);
    connect(this, &MainWindow::rateChanged, r, &PortReader::onMsRateChanged, Qt::QueuedConnection);

    m_portReaderThread.start();
}

MainWindow::~MainWindow()
{
    m_portReaderThread.quit();
    m_portReaderThread.wait();
}

void MainWindow::initializeGL() {
    initializeOpenGLFunctions();
    QtImGui::initialize(this);
}

void MainWindow::paintGL() {
    QtImGui::newFrame();

    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    {
        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&clear_color);
        if (ImGui::Button("Test Window")) {
            show_test_window ^= 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Another Window")) show_another_window ^= 1;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }//

    // 2. Show another simple window, this time using an explicit Begin/End pair
    if (show_another_window)
    {
        ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiCond_FirstUseEver);
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello");
        ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
    if (show_test_window)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
        ImGui::ShowDemoWindow();
    }

    ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiCond_FirstUseEver);
    ImGui::Begin("Serial Port graf!");
    if(ImGui::BeginCombo("Serial Port", m_selectedPort.c_str())) {
        for(auto p : m_serialPorts) {
            ImGui::PushID(p.systemLocation().toStdString().c_str());
            auto name = p.portName().toStdString();
            if(ImGui::Selectable(name.c_str())) {
                m_selectedPort = name;
                emit serialPortSelected(p);
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }


    const int toShow = std::min((int)m_plotValues.size(), 200);
    ImGui::PlotLines("Output",
                     m_plotValues.data() + m_plotValues.size() - toShow,
                     toShow,
                     0, "cos", -1.0f, 1.0f, ImVec2(0, 100.0f));

    static char rateNum[32];
    ImGui::InputText("rate (ms)", rateNum, sizeof(rateNum), ImGuiInputTextFlags_CharsDecimal);
    if(ImGui::Button("Set")) {
        int msVal = QString(rateNum).toInt();
        if(msVal != 0) {
            emit rateChanged(msVal);
        }
    }
    ImGui::End();

    // Do render before ImGui UI is rendered
    glViewport(0, 0, width(), height());
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Render();
    QtImGui::render();

    update();
}

void MainWindow::updateSerialPorts() {
    m_serialPorts = QSerialPortInfo::availablePorts();
}

void MainWindow::onPlotValue(double val) {
    m_plotValues.push_back(val);

    if(m_plotValues.size() > 1000) {
        m_plotValues.erase(m_plotValues.begin(), m_plotValues.begin()+500);
    }
}

