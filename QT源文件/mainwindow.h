#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMqttClient>
#include <QStackedLayout>
#include <QLineEdit>
#include <ActiveQt/QAxWidget>
#include <QTimer>
#include <QDateTime>
#include <QTcpServer>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();
    void onTokenResponse();
    void onReplyFinished();
    void GetToken();
    QString getDeviceShadow(const QString &projectId, const QString &deviceId, const QString &token);
    void receiveAndDisplayVideo(const QString &url, QWidget *parent) ;

private slots:
    void updateLogStateChange();        // 更新日志状态
    void brokerDisconnected();          // 断开连接
    void setClientPort(int p);          // 设置客户端端口
    void on_buttonQuit_clicked();       // 退出
    void on_buttonConnect_clicked();    // 连接
    void on_buttonPublish_clicked();    // 发布
    void on_button_sim_data_clicked();  // 发送模拟数据
    void on_buttonLED1_clicked();       // LED1
    void on_buttonLED2_clicked();       // LED2
    void on_buttonLED3_clicked();       // LED3
    void send_sim_data();               // 发送模拟数据

    void loadNavigate();
    void timerUpdate(void);

    void on_loadButton_clicked();

private:
    void initView();                    // 初始化视图
    void initSlots();                   // 初始化槽函数
    void setupConnections();
    void setLED1State(int on);           // 设置LED1状态
    void setLED2State(int on);           // 设置LED2状态
    void setLED3State(int on);           // 设置LED3状态

    Ui::MainWindow* ui;
    QMqttClient* m_client;
    QTimer* timer;
    QLineEdit *lineUrl;
    QAxWidget* webWidget;
};
QByteArray gettoken();

#endif // MAINWINDOW_H
