#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore/QDateTime>
#include <QtMqtt/QMqttClient>
#include <QtWidgets/QMessageBox>
#include <QTimer>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QCamera>
#include <QTcpSocket>

#include <QMediaPlayer>
#include <QVideoWidget>
#include <QVBoxLayout>
#include <QMainWindow>



// 定义 MQTT 配置常量
const QString MQTT_HOST = "117.78.5.125";
const quint16 MQTT_PORT = 1883;
const QString MQTT_CLIENT_ID = "66e7c3a7d671df42eb372386_dev1_0_0_2024091800";
const QString MQTT_USERNAME = "66e7c3a7d671df42eb372386_dev1";
const QString MQTT_PASSWORD = "e9c621f8212c344cac5ed6b72599593c6dc832576c82e1d1c0d3b780c15e21e5";
const QString MQTT_TOPIC = "$oc/devices/66e7c3a7d671df42eb372386_dev1/sys/properties/report";
const QString MQTT_MESSAGE = "{\"services\":[{\"service_id\":\"stm32\",\"properties\":{\"DHT11_T\":30,\"DHT11_H\":34}}]}";

QString MAIN_USER="CSDN-m0_73451791";
QString IAM_USER="pikabu";
QString IAM_PASSWORD="@LSQ13086409525";
QString SERVER_ID="cn-north-4";
QString PROJECT_ID="8f175f1608ae424c97d9d8300de76925";
QString Product_id="66e7c3a7d671df42eb372386_dev1";
QNetworkAccessManager* manager = new QNetworkAccessManager();
QByteArray token;
// 存储MQ2等值的容器
QMap<QString, QJsonValue> sensorValues;


MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    // 调用函数显示视频流
    QString url = ui->addressEdit->text();
    receiveAndDisplayVideo(url, this);

    QTimer *timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(timerUpdate()));
    timer->start(1000);

    initView();    // 初始化视图
    initSlots();   // 初始化槽函数
}

MainWindow::~MainWindow() {
    delete ui;
    delete timer;
}
void MainWindow::receiveAndDisplayVideo(const QString &url, QWidget *parent) {
    // 创建视频播放器
    QMediaPlayer *player = new QMediaPlayer;

    // 创建视频窗口部件
    QVideoWidget *videoWidget = new QVideoWidget(parent);
    videoWidget->setGeometry(20, 360, 320, 240); // 设置视频窗口部件的位置和大小

    // 设置视频窗口部件的布局
    QVBoxLayout *imagelayout = new QVBoxLayout(parent);
    imagelayout->addWidget(videoWidget);

    // 设置布局的边缘填充和控件之间的间距
    imagelayout->setContentsMargins(10, 10, 10, 10); // 左、上、右、下的填充
    imagelayout->setSpacing(5); // 控件之间的间距

    // 设置视频窗口部件的大小策略
    videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 设置视频窗口部件的最小大小
    videoWidget->setMinimumSize(320, 240);

    // 设置视频窗口部件的最大大小
    videoWidget->setMaximumSize(1280, 720);


    parent->setLayout(imagelayout);

    // 设置视频输出到视频窗口部件
    player->setVideoOutput(videoWidget);

    // 设置媒体源为视频流URL
    player->setSource(QUrl(url));

    // 开始播放视频
    player->play();

    // 可以添加错误处理和状态更新的槽函数
    connect(player, &QMediaPlayer::mediaStatusChanged, [this, player](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            qDebug() << "Video playback stopped";
        }
    });

    connect(player, &QMediaPlayer::errorOccurred, [this, player](QMediaPlayer::Error error) {
        qDebug() << "Error:" << player->errorString();
    });
}
/*
 功能: 获取token
 */
void MainWindow::GetToken()
{
    QString requestUrl;
    QNetworkRequest request;
    // 设置请求地址
    QUrl url;
    // 获取token请求地址
    requestUrl = QString("https://iam.%1.myhuaweicloud.com/v3/auth/tokens")
                     .arg(SERVER_ID);
    // 设置数据提交格式
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json;charset=UTF-8"));
    // 构造请求
    url.setUrl(requestUrl);
    request.setUrl(url);
    QString text = QString("{\"auth\":{\"identity\":{\"methods\":[\"password\"],\"password\":"
                           "{\"user\":{\"domain\": {"
                           "\"name\":\"%1\"},\"name\": \"%2\",\"password\": \"%3\"}}},"
                           "\"scope\":{\"project\":{\"name\":\"%4\"}}}}")
                       .arg(MAIN_USER)
                       .arg(IAM_USER)
                       .arg(IAM_PASSWORD)
                       .arg(SERVER_ID);
    // 发送请求
    QNetworkReply *reply = manager->post(request, text.toUtf8());

    // 连接信号到槽函数以获取响应
    connect(reply, &QNetworkReply::finished, this, &MainWindow::onTokenResponse);
}
// 槽函数，用于处理响应
void MainWindow::onTokenResponse()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply && reply->error() == QNetworkReply::NoError) {
        // 获取响应头
        QList<QByteArray> rawHeadersList = reply->rawHeaderList();
        foreach(QByteArray header, rawHeadersList) {
            QString headerStr = QString(header);
            // 打印每个响应头
            //qDebug() << headerStr;
            // 查找X-Subject-Token响应头
            if (headerStr.startsWith("X-Subject-Token")) {
                token = reply->rawHeader("X-Subject-Token");
                //qDebug() << "X-Subject-Token:" << token;
                getDeviceShadow("","",(QString)token);
            }
        }
    } else {
        // 处理错误情况
        qDebug() << "Error:" << reply->errorString();
    }
    reply->deleteLater();
}


QString MainWindow::getDeviceShadow(const QString &projectId, const QString &deviceId, const QString &token) {
    // 构建 URL
    QString strUrl = QString("https://de8796ee74.st1.iotda-app.cn-north-4.myhuaweicloud.com:443/v5/iot/8f175f1608ae424c97d9d8300de76925/devices/66e7c3a7d671df42eb372386_dev1/shadow");

    // 创建网络请求对象
    QNetworkRequest request = QNetworkRequest(QUrl(strUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    //qDebug() << "X-Subject-Token:" << token;
    request.setRawHeader("X-Auth-Token", token.toUtf8());

    // 创建网络访问管理器
    QNetworkAccessManager manager;

    // 发送 GET 请求
    QNetworkReply *reply = manager.get(request);

    // 等待请求完成
    // 等待请求完成
    // 使用事件循环等待请求完成
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // 检查错误
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error:" << reply->errorString();
        return QString();
    }

    // 读取响应
    QByteArray result = reply->readAll();

    // 打印结果
    //qDebug() << result;

    //解析数据
    QJsonParseError json_error;
    QJsonDocument document = QJsonDocument::fromJson(result, &json_error);
    if(json_error.error == QJsonParseError::NoError)
    {
        QJsonObject obj = document.object();
        //判断是否是对象,然后开始解析数据
        if(document.isObject())
        {
            QJsonObject jsonObj = document.object();
            //qDebug() <<jsonObj;
            // 获取设备ID
            QString deviceId = jsonObj.value("device_id").toString();
            // 获取影子对象数组
            QJsonArray shadowArray = jsonObj.value("shadow").toArray();
            // 遍历影子对象数组
            for (const QJsonValue &shadowValue : shadowArray) {
                QJsonObject shadowObj = shadowValue.toObject();

                // 获取reported对象
                QJsonObject reportedObj = shadowObj.value("reported").toObject();
                QJsonObject propertiesReported = reportedObj.value("properties").toObject();

                // 存储MQ2等值
                sensorValues.insert("MQ2", propertiesReported.value("MQ2"));
                sensorValues.insert("DHT11_T", propertiesReported.value("DHT11_T"));
                sensorValues.insert("DHT11_H", propertiesReported.value("DHT11_H"));
                sensorValues.insert("flame", propertiesReported.value("flame"));
                sensorValues.insert("light", propertiesReported.value("light"));

                // 打印获取的值
                //qDebug() << "Device ID:" << deviceId;

                // 打印sensorValues中的所有键值对
                for (auto it = sensorValues.constBegin(); it != sensorValues.constEnd(); ++it) {
                    qDebug() << it.key() << ":" << it.value();
                }
            }
        }
    }

    // 断开连接
    reply->deleteLater();

    return result;
}
void MainWindow::loadNavigate()
{
    QString sUrl = lineUrl->text().trimmed();
    webWidget->dynamicCall("Navigate(const QString&)",sUrl);
}
void MainWindow::timerUpdate(void)
{
    QDateTime time = QDateTime::currentDateTime();
    QString str = time.toString("yyyy-MM-dd hh:mm:ss dddd");
    ui->timelabel->setText(str);
}
void sendHttpRequest() {
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QUrl url("https://iotda.cn-north-4.myhuaweicloud.com/v5/iot/8f175f1608ae424c97d9d8300de76925/devices/66e7c3a7d671df42eb372386_dev1/shadow");
    QNetworkRequest request(url);

    // 设置请求头
    request.setRawHeader("Authorization", "<Your signed string>");

    // 发送GET请求
    QNetworkReply *reply = manager->get(request);

    // 等待请求完成
    QObject::connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString response = reply->readAll();
            qDebug() << "Response:" << response;
            // 处理响应数据
        } else {
            qDebug() << "Error:" << reply->errorString();
        }
        reply->deleteLater();
    });

    // 等待请求发送
    QObject::connect(reply, &QNetworkReply::uploadProgress, [](qint64 bytesSent, qint64 bytesTotal) {
        qDebug() << "Uploaded" << bytesSent << "of" << bytesTotal;
    });

    // 等待请求完成
    QObject::connect(reply, &QNetworkReply::downloadProgress, [](qint64 bytesReceived, qint64 bytesTotal) {
        qDebug() << "Downloaded" << bytesReceived << "of" << bytesTotal;
    });

    // 确保事件循环运行
    QCoreApplication::instance()->exec();
}
void MainWindow::initView() {
    // 使用常量初始化 MQTT 信息
    ui->lineEditHost->setText(MQTT_HOST);
    ui->spinBoxPort->setValue(MQTT_PORT);
    ui->lineEditHost_client_id->setText(MQTT_CLIENT_ID);
    ui->lineEditHost_user_name->setText(MQTT_USERNAME);
    ui->lineEditHost_password->setText(MQTT_PASSWORD);
    ui->lineEditTopic->setText(MQTT_TOPIC);
    ui->lineEditMessage->setText(MQTT_MESSAGE);

    m_client = new QMqttClient(this);
    m_client->setHostname(MQTT_HOST);
    m_client->setPort(MQTT_PORT);
    m_client->setClientId(MQTT_CLIENT_ID);
    m_client->setUsername(MQTT_USERNAME);
    m_client->setPassword(MQTT_PASSWORD);
}
void MainWindow::setupConnections() {

    // 连接信息文本框监听
    connect(ui->lineEditHost, &QLineEdit::textChanged, m_client, &QMqttClient::setHostname);
    connect(ui->spinBoxPort, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::setClientPort);
    connect(ui->lineEditHost_client_id, &QLineEdit::textChanged, m_client, &QMqttClient::setClientId);
    connect(ui->lineEditHost_user_name, &QLineEdit::textChanged, m_client, &QMqttClient::setUsername);
    connect(ui->lineEditHost_password, &QLineEdit::textChanged, m_client, &QMqttClient::setPassword);
    updateLogStateChange();

    // 定时器
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(send_sim_data()));
    // 定时器开关
    connect(ui->pushButton_sim_data, &QPushButton::clicked, this, &MainWindow::on_button_sim_data_clicked);
    // LED1开关
    connect(ui->pushButton_led1, &QPushButton::clicked, this, &MainWindow::on_buttonLED1_clicked);
    // LED2开关
    connect(ui->pushButton_led2, &QPushButton::clicked, this, &MainWindow::on_buttonLED2_clicked);
    // LED3开关
    connect(ui->pushButton_led3, &QPushButton::clicked, this, &MainWindow::on_buttonLED3_clicked);
    // 清理日志
    connect(ui->button_clean, &QPushButton::clicked, this, [=]() {
        ////ui->editLog->clear();
    });
}


void MainWindow::initSlots() {
    // 日志状态改变
    connect(m_client, &QMqttClient::stateChanged, this, &MainWindow::updateLogStateChange);
    // 断开连接
    connect(m_client, &QMqttClient::disconnected, this, &MainWindow::brokerDisconnected);
    // 收到消息
    connect(m_client, &QMqttClient::messageReceived, this, [this](const QByteArray& message, const QMqttTopicName& topic) {
        const QString content = QDateTime::currentDateTime().toString()
            + QLatin1String(" Received Topic: ")
            + topic.name()
            + QLatin1String(" Message: ")
            + message
            + QLatin1Char('\n');
        ////ui->editLog->insertPlainText(content);

        // message转为QString
        QString messageStr = QString::fromUtf8(message);

        //将message转为json对象
        QJsonDocument jsonDoc = QJsonDocument::fromJson(messageStr.toUtf8());
        QJsonObject jsonObj = jsonDoc.object();
        QString statusKey, statusValue;

            if (jsonObj.value("command_name").toString().contains("LED1")) {
                //控制LED1
                statusKey = "LED1";
                QJsonObject parasObj = jsonObj.value("paras").toObject();
                if (parasObj.value("LED1Cmd").toString().contains("ON")) {
                    //打开LED1
                    setLED1State(1);
                    ui->pushButton_led1->setText("ON");
                    //ui->editLog->insertPlainText("ON");
                    statusValue = "ON";
                }
                else {
                    setLED1State(0);
                    ui->pushButton_led1->setText("OFF");
                    //ui->editLog->insertPlainText("OFF");
                    statusValue = "OFF";
                }
            }
            else if (jsonObj.value("command_name").toString().contains("LED2")){
                //控制LED2
                statusKey = "LED2";
                QJsonObject parasObj = jsonObj.value("paras").toObject();
                if (parasObj.value("LED2Cmd").toString().contains("ON")) {
                    //打开LED2
                    setLED2State(1);
                    ui->pushButton_led2->setText("ON");
                    //ui->editLog->insertPlainText("ON");
                    statusValue = "ON";
                }
                else {
                    setLED2State(0);
                    ui->pushButton_led2->setText("OFF");
                    //ui->editLog->insertPlainText("OFF");
                    statusValue = "OFF";
                }
            }
            else if (jsonObj.value("command_name").toString().contains("LED3")){
                //控制LED3
                statusKey = "LED3";
                QJsonObject parasObj = jsonObj.value("paras").toObject();
                if (parasObj.value("LED3Cmd").toString().contains("ON")) {
                    //打开LED3
                    setLED3State(1);
                    ui->pushButton_led3->setText("ON");
                    //ui->editLog->insertPlainText("ON");
                    statusValue = "ON";
                }
                else {
                    setLED3State(0);
                    ui->pushButton_led3->setText("OFF");
                    //ui->editLog->insertPlainText("OFF");
                    statusValue = "OFF";
                }
            }

        //接收到消息后，做出响应
        QString topicStr = topic.name();
        QString request_id = topicStr.mid(topicStr.lastIndexOf("=") + 1);
        QString responseTopic = "$oc/devices/66e7c3a7d671df42eb372386_dev1/sys/commands/response/request_id=" + request_id;
        QString responseBody = "{\"result_code\":0,\"response_name\":\"COMMAND_RESPONSE\",\"paras\":{\"result\":\"success\"}}";
        if (m_client->publish(responseTopic, responseBody.toUtf8()) == -1)
            QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message"));
        responseTopic = "$oc/devices/66e7c3a7d671df42eb372386_dev1/sys/properties/set/response/request_id=" + request_id;
        if (m_client->publish(responseTopic, responseBody.toUtf8()) == -1)
            QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message"));
        // 更新状态
        QString jsonStr = "{\"services\":[{\"service_id\":\"stm32\",\"properties\":{\"%1\":\"%2\"}}]}";
        jsonStr = jsonStr.arg(statusKey).arg(statusValue);
        if (m_client->publish(ui->lineEditTopic->text(), jsonStr.toUtf8()) == -1)
            QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message"));
        //ui->editLog->insertPlainText(jsonStr);

        });
    setupConnections();
}

void MainWindow::on_buttonLED1_clicked() {
    QString LED1;
    if (ui->pushButton_led1->text().contains("OFF")) {
        setLED1State(1);
        ui->pushButton_led1->setText("ON");
        LED1 = "ON";
    }
    else {
        setLED1State(0);
        ui->pushButton_led1->setText("OFF");
        LED1 = "OFF";
    }

    // 更新状态
    // "{\"services\":[{\"service_id\":\"BasicData\",\"properties\":{\"luminance\":%1}}]}"转为JSON对象
    QString jsonStr = "{\"services\":[{\"service_id\":\"stm32\",\"properties\":{\"LED1\":\"%1\"}}]}";
    jsonStr = jsonStr.arg(LED1);
    if (m_client->publish(ui->lineEditTopic->text(), jsonStr.toUtf8()) == -1)
        QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message"));
    //ui->editLog->insertPlainText(jsonStr);
}

void MainWindow::on_buttonLED2_clicked() {
    QString LED2;
    if (ui->pushButton_led2->text().contains("OFF")) {
        setLED2State(1);
        ui->pushButton_led2->setText("ON");
        LED2 = "ON";
    }
    else {
        setLED2State(0);
        ui->pushButton_led2->setText("OFF");
        LED2 = "OFF";
    }
    // "{\"services\":[{\"service_id\":\"BasicData\",\"properties\":{\"luminance\":%1}}]}"转为JSON对象
    QString jsonStr = "{\"services\":[{\"service_id\":\"stm32\",\"properties\":{\"LED2\":\"%1\"}}]}";
    jsonStr = jsonStr.arg(LED2);
    if (m_client->publish(ui->lineEditTopic->text(), jsonStr.toUtf8()) == -1)
        QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message"));
    //ui->editLog->insertPlainText(jsonStr);
}

void MainWindow::on_buttonLED3_clicked()
{
    QString LED3;
    if (ui->pushButton_led3->text().contains("OFF")) {
        setLED3State(1);
        ui->pushButton_led3->setText("ON");
        LED3 = "ON";
    }
    else {
        setLED3State(0);
        ui->pushButton_led3->setText("OFF");
        LED3 = "OFF";
    }

    // 更新状态
    // "{\"services\":[{\"service_id\":\"BasicData\",\"properties\":{\"luminance\":%1}}]}"转为JSON对象
    QString jsonStr = "{\"services\":[{\"service_id\":\"stm32\",\"properties\":{\"LED3\":\"%1\"}}]}";
    jsonStr = jsonStr.arg(LED3);
    if (m_client->publish(ui->lineEditTopic->text(), jsonStr.toUtf8()) == -1)
        QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message"));
    //ui->editLog->insertPlainText(jsonStr);
}

void MainWindow::on_buttonConnect_clicked() {
    if (m_client->state() == QMqttClient::Disconnected) {
        ui->lineEditHost->setEnabled(false);
        ui->spinBoxPort->setEnabled(false);
        ui->lineEditHost_client_id->setEnabled(false);
        ui->lineEditHost_user_name->setEnabled(false);
        ui->lineEditHost_password->setEnabled(false);
        ui->buttonConnect->setText(tr("断开连接"));
        m_client->connectToHost();
    }
    else {
        ui->lineEditHost->setEnabled(true);
        ui->spinBoxPort->setEnabled(true);
        ui->buttonConnect->setText(tr("连接"));
        m_client->disconnectFromHost();
    }
}

void MainWindow::on_buttonQuit_clicked() {
    QApplication::quit();
}

void MainWindow::updateLogStateChange() {
    const QString content = QDateTime::currentDateTime().toString()
        + QLatin1String(": State Change")
        + QString::number(m_client->state())
        + QLatin1Char('\n');
    //ui->editLog->insertPlainText(content);
}

void MainWindow::brokerDisconnected() {
    ui->lineEditHost->setEnabled(true);
    ui->spinBoxPort->setEnabled(true);
    ui->lineEditHost_client_id->setEnabled(true);
    ui->lineEditHost_user_name->setEnabled(true);
    ui->lineEditHost_password->setEnabled(true);
    ui->buttonConnect->setText(tr("连接"));
}

void MainWindow::setClientPort(int p) {
    m_client->setPort(p);
}

void MainWindow::on_buttonPublish_clicked() {
    if (m_client->publish(ui->lineEditTopic->text(), ui->lineEditMessage->text().toUtf8()) == -1)
        QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message"));
}



void MainWindow::on_button_sim_data_clicked() {
    timer->setInterval(ui->spinBoxPort_time->value() * 1000);
    timer->setSingleShot(false); // 周期性触发
    if (timer->isActive()) {
        timer->stop();
        ui->pushButton_sim_data->setText("获取数据");
        ui->spinBoxPort_time->setEnabled(true);
    }
    else {
        timer->start();
        ui->pushButton_sim_data->setText("停止获取");
        ui->spinBoxPort_time->setEnabled(false);
    }
}

void MainWindow::send_sim_data() {
    GetToken();
    if(sensorValues.value("DHT11_T").toDouble()>=0.01||sensorValues.value("DHT11_T").toDouble()<=-0.01){
        QString DHT11_T = QString::number(sensorValues.value("DHT11_T").toDouble(), 'f', 1);
        QString DHT11_H = QString::number(sensorValues.value("DHT11_H").toDouble(), 'f', 1);
        int MQ2=sensorValues.value("MQ2").toInt();
        int Light=sensorValues.value("light").toInt();
        int Flame=sensorValues.value("flame").toInt();
        ui->DHT11_TlcdNumber->setSegmentStyle(QLCDNumber::Flat);
        ui->DHT11_HlcdNumber->setSegmentStyle(QLCDNumber::Flat);
        ui->DHT11_TlcdNumber->display(DHT11_T);
        ui->DHT11_HlcdNumber->display(DHT11_H);
        if(Light==0){
             ui->Lightlabel->setText("阳光普照大地");
        }
        else{
            ui->Lightlabel->setText("月黑风高夜深");
        }
        if(MQ2){
            ui->MQ2label->setText("未检测到烟雾");
        }
        else{
            ui->MQ2label->setText("你家冒烟啦！");
        }
        if(Flame){
            ui->Flamelabel->setText("未检测到明火");
        }
        else{
            ui->Flamelabel->setText("你家着火啦！");
        }
    }
}

void MainWindow::setLED3State(int on) {
#ifdef __arm__
    QString strFile = "/sys/class/leds/green/brightness";
    QFile file(strFile);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        qDebug() << "/sys/class/leds/blue/brightness export failed!";
        return;
    }

    file.write(1 == on ? "1" : "0");
    file.close();
#else
    qDebug() << "state" << on;
#endif
}

void MainWindow::setLED1State(int on) {
#ifdef __arm__
    QString strFile = "/sys/class/leds/green/brightness";
    QFile file(strFile);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        qDebug() << "/sys/class/leds/red/brightness export failed!";
        return;
    }

    file.write(1 == on ? "1" : "0");
    file.close();
#else
    qDebug() << "state" << on;
#endif
}
void MainWindow::setLED2State(int on) {
#ifdef __arm__
    QString strFile = "/sys/class/leds/green/brightness";
    QFile file(strFile);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        qDebug() << "/sys/class/leds/green/brightness export failed!";
        return;
    }

    file.write(1 == on ? "1" : "0");
    file.close();
#else
    qDebug() << "state" << on;
#endif
}





void MainWindow::on_loadButton_clicked()
{
    QString url = ui->addressEdit->text();
}

