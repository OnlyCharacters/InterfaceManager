#include "interfacemanager.h"
#include "./ui_interfacemanager.h"
#include "./error_dialog.h"

QString obtainValue(const QString inputString, const QString key)
{
    qsizetype key_value_start = inputString.indexOf(key);
    if (key_value_start == -1) {
        return "";
    }

    qsizetype key_value_end = inputString.indexOf("\r\n", key_value_start);

    QString key_value = "";
    if (key_value_end == -1)
        key_value = inputString.sliced(key_value_start);
    else {
        key_value = inputString.sliced(key_value_start, key_value_end - key_value_start);
    }

    QString value = "";
    if (key_value.indexOf(':') == key_value.length() - 1)
        value = "";
    else
        value = key_value.sliced(key_value.indexOf(':') + 2);

    return value;
}

bool isIPv4(const QString ip4String)
{
    QHostAddress tempAddr(ip4String);
    return tempAddr.protocol() == QAbstractSocket::IPv4Protocol;
}

QString getIP4Addr(const QString inputString)
{
    QString temp(inputString);
    temp.replace("{", "");
    temp.replace("}", "");

    QStringList ip4StringList = temp.split(", ");
    foreach(QString ip4String, ip4StringList) {
        if (isIPv4(ip4String)) {
            return ip4String;
        }
    }

    return "";
}

QString getInterfaceConfig(const struct Interface* adapter)
{
    QProcess* qProcess = new QProcess();
    QString rtn = "";
    QStringList args;

    args << QString("Get-WmiObject Win32_NetworkAdapterConfiguration -filter \"Index=%1\" |\
                    Select Index, DHCPEnabled, Description, IpAddress, IPSubnet, DefaultIPGateway, DNSServerSearchOrder").arg(adapter->index);
    qProcess->start("powershell", args);
    qProcess->waitForFinished();
    rtn = qProcess->readAllStandardOutput();
    delete qProcess;
    return rtn;
}

QStringList getDnsList(const QString inputString)
{
    QString temp(inputString);
    QStringList rtn;
    temp.replace("{", "");
    temp.replace("}", "");

    rtn.clear();
    QStringList ip4StringList = temp.split(", ");
    foreach(QString ip4String, ip4StringList) {
        if (isIPv4(ip4String)) {
            rtn << ip4String;
        }
    }

    return rtn;
}

InterfaceManager::InterfaceManager(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::InterfaceManager)
{
    ui->setupUi(this);

    tray = new QSystemTrayIcon(this);
    tray->setIcon(QIcon("./icons8-internet-16_1.png"));
    connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(tray_clicked(QSystemTrayIcon::ActivationReason)));
    tray->show();

    getInterfaceList();
    foreach(struct Interface interface, interfaceList) {
        this->ui->interface_list->addItem(interface.name);
    }
}

InterfaceManager::~InterfaceManager()
{
    delete ui;
}

void InterfaceManager::parseInterfaceFromPowerShell(struct Interface* adapter, const QString interfaceString)
{
    QString temp = interfaceString;
    QStringList dnsListTemp;
    temp = temp.trimmed();
    QString index = obtainValue(temp, "Index");
    if (index == "")
        return;
    adapter->index = index.toInt(nullptr, 10);
    adapter->dhcp = ((obtainValue(temp, "DHCPEnabled")) == "True");
    adapter->name = obtainValue(temp, "Description");
    adapter->ip4Addr = getIP4Addr(obtainValue(temp, "IpAddress"));
    adapter->netmask = getIP4Addr(obtainValue(temp, "IPSubnet"));
    adapter->ip4Gateway = getIP4Addr(obtainValue(temp, "DefaultIPGateway"));
    dnsListTemp = getDnsList(obtainValue(temp, "DNSServerSearchOrder"));
    if (dnsListTemp.count() != 0) {
        adapter->dnsList.clear();
        adapter->dnsList = dnsListTemp;
    }
}

void InterfaceManager::getInterfaceList()
{
    QProcess* getInterface = new QProcess(this);
    QString cmdOutput;
    QString interfaceString;
    QStringList args;
    QStringList interfaceStringList;

    // Get all Network Interface properties by using powershell
    args << "chcp 65001 | out-null;";   // Set output UTF-8
    args << "Get-WmiObject Win32_NetworkAdapterConfiguration\
             | Where IPEnabled | Select Index, DHCPEnabled, Description,\
             IpAddress, IPSubnet, DefaultIPGateway, DNSServerSearchOrder";   // Get NetworkInterface
    getInterface->start("powershell", args);
    getInterface->waitForFinished();
    cmdOutput = QString::fromUtf8(getInterface->readAllStandardOutput()).trimmed();
    delete getInterface;

    // Parse the powershell output
    interfaceList.clear();
    interfaceStringList = cmdOutput.split("\r\n\r\n");
    foreach (interfaceString, interfaceStringList) {
        struct Interface temp;
        temp.name = "";
        this->parseInterfaceFromPowerShell(&temp, interfaceString);
        if (temp.name == "")
            continue;
        interfaceList << temp;
    }
}

void InterfaceManager::updatePannelIPConfig(const struct Interface* adapter)
{
    this->ui->lineEdit_addr->setText(adapter->ip4Addr);
    this->ui->lineEdit_netmask->setText(adapter->netmask);
    this->ui->lineEdit_gateway->setText(adapter->ip4Gateway);

    if (adapter->dhcp) {
        this->ui->checkbox_dhcp->setCheckState(Qt::Checked);
    } else {
        this->ui->checkbox_dhcp->setCheckState(Qt::Unchecked);
    }

    if (adapter->dnsList.count() > 1) {
        this->ui->lineEdit_dns1->setText(adapter->dnsList[0]);
        this->ui->lineEdit_dns2->setText(adapter->dnsList[1]);
    }
    else if (adapter->dnsList.count() == 1) {
        this->ui->lineEdit_dns1->setText(adapter->dnsList[0]);
        this->ui->lineEdit_dns2->setText("");
    } else {
        this->ui->lineEdit_dns1->setText("");
        this->ui->lineEdit_dns2->setText("");
    }
}

void InterfaceManager::setIP4Addr(struct Interface* adapter, const QString ip4Addr, const QString netmask, const QString ip4Gateway)
{
    QProcess* setInterface = new QProcess();
    QStringList args;
    QString cmdOutput;

    args << QString("$ada = Get-WmiObject Win32_NetworkAdapterConfiguration\
                    -filter \"Index=%1\";$ada.EnableStatic(\"%2\", \"%3\")").arg(adapter->index).arg(ip4Addr, netmask);

    if (ip4Gateway != "")
        args << QString(";$ada.SetGateways(\"%1\")").arg(ip4Gateway);

    setInterface->start("powershell", args);
    setInterface->waitForFinished();
    cmdOutput = setInterface->readAllStandardOutput();

    if (setInterface->exitCode() != 0) {
        error_dialog ed1(this);
        ed1.exec();
        ed1.setContent(cmdOutput);
    } else {
        error_dialog ed1(this);
        ed1.setContent("修改地址成功");
        ed1.exec();
        adapter->ip4Addr = ip4Addr;
        adapter->netmask = netmask;
        adapter->ip4Gateway = ip4Gateway;
    }

    delete setInterface;
}

void InterfaceManager::setDHCP(struct Interface* adapter)
{
    QProcess* setInterface = new QProcess();
    QStringList args;
    QString cmdOutput, interfaceConfig;

    // Setting dhcp
    args << QString("$ada = Get-WmiObject Win32_NetworkAdapterConfiguration -filter \"Index=%1\"").arg(adapter->index);
    args << QString(";$ada.EnableDHCP()");
    setInterface->start("powershell", args);

    setInterface->waitForFinished();
    cmdOutput = setInterface->readAllStandardError();
    if (setInterface->exitCode() != 0) {
        qDebug() << cmdOutput;
        error_dialog ed1(this);
        ed1.setContent("未知错误");
        ed1.exec();
        delete setInterface;
        return;
    } else {
        error_dialog ed1(this);
        ed1.setContent("修改DHCP成功");
        ed1.exec();
    }
    delete setInterface;

    // Get the DHCP config of this Interface
    // Sometimes, after set dhcp from static, has 2 gateway address
    // This make sure the dhcp's gateway is the only gateway
    interfaceConfig = getInterfaceConfig(adapter);
    QStringList ip4GatewayList = getDnsList(obtainValue(interfaceConfig, "DefaultIPGateway"));
    if (ip4GatewayList.count() > 1) {
        // Set the second gateway ip to gateway
        setInterface = new QProcess();
        args.clear();
        args << QString("$ada = Get-WmiObject Win32_NetworkAdapterConfiguration -filter \"Index=%1\"").arg(adapter->index);
        args << QString(";$ada.SetGateways(\"%1\")").arg(ip4GatewayList[1]);
        setInterface->start("powershell", args);
        setInterface->waitForFinished();
    }
}

void InterfaceManager::setDns(struct Interface* adapter, const QStringList dnsList)
{
    QProcess* setInterface = new QProcess();
    QStringList args;
    QString cmdOutput;

    args << QString("$ada = Get-WmiObject Win32_NetworkAdapterConfiguration -filter \"Index=%1\"").arg(adapter->index);
    if (dnsList.count() > 1) {
        QString DNSServerSearchOrder = '"' + dnsList[0] + '"' + "," + '"' + dnsList[1] + '"';
        args << QString(";$DNSServerSearchOrder = %1").arg(DNSServerSearchOrder);
        args << QString(";$ada.SetDNSServerSearchOrder($DNSServerSearchOrder)");
    } else {
        args << QString(";$ada.SetDNSServerSearchOrder(\"%1\")").arg(dnsList[0]);
    }
    setInterface->start("powershell", args);
    setInterface->waitForFinished();

    cmdOutput = setInterface->readAllStandardError();
    if (setInterface->exitCode() != 0) {
        qDebug() << cmdOutput;
        error_dialog ed1(this);
        ed1.setContent("未知错误");
        ed1.exec();
        delete setInterface;
        return;
    } else {
        error_dialog ed1(this);
        ed1.setContent("修改DNS成功");
        ed1.exec();
    }
    delete setInterface;
}

void InterfaceManager::setAutoDns(struct Interface* adapter)
{
    QProcess* setInterface = new QProcess();
    QStringList args;
    QString cmdOutput;

    args << QString("$ada = Get-WmiObject Win32_NetworkAdapterConfiguration -filter \"Index=%1\"").arg(adapter->index);
    args << QString(";$ada.SetDNSServerSearchOrder()");
    setInterface->start("powershell", args);
    setInterface->waitForFinished();

    cmdOutput = setInterface->readAllStandardOutput();
    if (setInterface->exitCode() != 0) {
        qDebug() << cmdOutput;
        error_dialog ed1(this);
        ed1.setContent("未知错误");
        ed1.exec();
        delete setInterface;
        return;
    } else {
        error_dialog ed1(this);
        ed1.setContent("修改自动DNS成功");
        ed1.exec();
    }
    delete setInterface;
}

struct Interface* InterfaceManager::findInterfaceFromList(const QString name)
{
    QList<struct Interface>::Iterator it = this->interfaceList.begin();
    struct Interface* rtn;

    rtn = nullptr;
    for (; it != this->interfaceList.end(); it++) {
        if (it->name == name) {
            rtn = &*it;
            break;
        }
    }

    return rtn;
}

void InterfaceManager::on_interface_list_currentTextChanged(const QString &arg1)
{
    struct Interface* adapter = this->findInterfaceFromList(arg1);
    if (adapter != nullptr) {
        this->updatePannelIPConfig(adapter);
    }
}

void InterfaceManager::on_checkbox_dhcp_stateChanged(int arg1)
{
    if (arg1 != 0) {
        this->ui->lineEdit_addr->setReadOnly(true);
        this->ui->lineEdit_netmask->setReadOnly(true);
        this->ui->lineEdit_gateway->setReadOnly(true);
    } else {
        this->ui->lineEdit_addr->setReadOnly(false);
        this->ui->lineEdit_netmask->setReadOnly(false);
        this->ui->lineEdit_gateway->setReadOnly(false);
    }
}


void InterfaceManager::on_btn_apply_addr_clicked()
{
    struct Interface* adapter = this->findInterfaceFromList(this->ui->interface_list->currentText());
    if (adapter->name != "") {
        if (this->ui->checkbox_dhcp->checkState() == Qt::Checked) {
            this->setDHCP(adapter);
        } else {
            QString ip4Addr = this->ui->lineEdit_addr->text();
            QString netmask = this->ui->lineEdit_netmask->text();
            QString ip4Gateway = this->ui->lineEdit_gateway->text();
            InterfaceManager::setIP4Addr(adapter, ip4Addr, netmask, ip4Gateway);
        }
        QString interfaceConfig = getInterfaceConfig(adapter);
        this->parseInterfaceFromPowerShell(adapter, interfaceConfig);
        this->updatePannelIPConfig(adapter);
    }
}


void InterfaceManager::on_btn_refresh_clicked()
{
    this->getInterfaceList();
    this->ui->interface_list->clear();
    foreach(struct Interface interface, interfaceList) {
        this->ui->interface_list->addItem(interface.name);
    }
}


void InterfaceManager::on_btn_apply_dns_clicked()
{
    struct Interface* adapter = this->findInterfaceFromList(this->ui->interface_list->currentText());
    if (adapter->name != "") {
        QStringList dnsStringList;
        QString dns1 = this->ui->lineEdit_dns1->text();
        if (dns1 != "")
            dnsStringList << dns1;

        QString dns2 = this->ui->lineEdit_dns2->text();
        if (dns2 != "")
            dnsStringList << dns2;

        if (dnsStringList.count() == 0)
            setAutoDns(adapter);
        else
            setDns(adapter, dnsStringList);
    }
    QString interfaceConfig = getInterfaceConfig(adapter);
    this->parseInterfaceFromPowerShell(adapter, interfaceConfig);
    this->updatePannelIPConfig(adapter);
}


void InterfaceManager::on_btn_apply_auto_dns_clicked()
{
    struct Interface* adapter = this->findInterfaceFromList(this->ui->interface_list->currentText());
    setAutoDns(adapter);
    QString interfaceConfig = getInterfaceConfig(adapter);
    this->parseInterfaceFromPowerShell(adapter, interfaceConfig);
    this->updatePannelIPConfig(adapter);
}


void InterfaceManager::on_btn_apply_all_clicked()
{
    struct Interface* adapter = this->findInterfaceFromList(this->ui->interface_list->currentText());
    if (adapter->name != "") {
        if (this->ui->checkbox_dhcp->checkState() == Qt::Checked) {
            this->setDHCP(adapter);
        } else {
            QString ip4Addr = this->ui->lineEdit_addr->text();
            QString netmask = this->ui->lineEdit_netmask->text();
            QString ip4Gateway = this->ui->lineEdit_gateway->text();
            InterfaceManager::setIP4Addr(adapter, ip4Addr, netmask, ip4Gateway);
        }

        QStringList dnsStringList;
        QString dns1 = this->ui->lineEdit_dns1->text();
        if (dns1 != "")
            dnsStringList << dns1;

        QString dns2 = this->ui->lineEdit_dns2->text();
        if (dns2 != "")
            dnsStringList << dns2;

        if (dnsStringList.count() == 0)
            setAutoDns(adapter);
        else
            setDns(adapter, dnsStringList);

        QString interfaceConfig = getInterfaceConfig(adapter);
        this->parseInterfaceFromPowerShell(adapter, interfaceConfig);
        this->updatePannelIPConfig(adapter);
    }
}

void InterfaceManager::tray_clicked(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::ActivationReason::Trigger:
        this->showNormal();
        this->activateWindow();
        break;
    case QSystemTrayIcon::ActivationReason::DoubleClick:
        this->showNormal();
        this->activateWindow();
        break;
    case QSystemTrayIcon::ActivationReason::MiddleClick:
        this->showNormal();
        this->activateWindow();
        break;
    case QSystemTrayIcon::ActivationReason::Context:
        this->showNormal();
        this->activateWindow();
        break;
    case QSystemTrayIcon::ActivationReason::Unknown:
        break;
    default:
        break;
    }
}

void InterfaceManager::changeEvent(QEvent* e)
{
    switch (e->type()) {
    case QEvent::WindowStateChange:
        if (this->windowState() == Qt::WindowMinimized) {
            this->hide();
        }
        break;
    default:
        break;
    }

    QWidget::changeEvent(e);
}
