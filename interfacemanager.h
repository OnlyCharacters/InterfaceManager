#ifndef INTERFACEMANAGER_H
#define INTERFACEMANAGER_H

#include <QWidget>
#include <QHostAddress>
#include <QProcess>
#include <QStringList>
#include <QList>
#include <string.h>

struct Interface {
    bool        dhcp;
    uint32_t    index;
    QString     name;
    QString     ip4Addr;
    QString     netmask;
    QString     ip4Gateway;
    QStringList dnsList;
};

/*
 * inputString: A string like "Index : 9\r\nDescription : VMware Virtual Ethernet Adapter for VMnet1\r\n"
 * key: A string like "Index"
 * return: the string value of the key, like "9"
*/
QString obtainValue(const QString inputString, const QString key);

/*
 * inputSting: A string like "{192.168.137.1, fe80::f6a0:652b:9db2:fb40}"
 * return: the first ip4addr in the list
*/
QString getIP4Addr(const QString inputString);

/*
 * ip4String: 192.168.137.1
 * return: true or false
*/
bool isIPv4(const QString ip4String);

/*
 * inputString: {8.8.8.8, 8.8.4.4}
*/
QStringList getDnsList(const QString inputString);

/*
 * get the adapter's interface config
*/
QString getInterfaceConfig(const struct Interface* adapter);

QT_BEGIN_NAMESPACE
namespace Ui { class InterfaceManager; }
QT_END_NAMESPACE

class InterfaceManager : public QWidget
{
    Q_OBJECT

public:
    InterfaceManager(QWidget *parent = nullptr);
    ~InterfaceManager();
    struct Interface* findInterfaceFromList(const QString name);
    void parseInterfaceFromPowerShell(struct Interface* adapter, const QString interfaceString);
    void getInterfaceList();
    void updatePannelIPConfig(const struct Interface* adapter);
    void setIP4Addr(struct Interface* adapter, const QString ip4Addr, const QString netmask, const QString ip4Gateway);
    void setDHCP(struct Interface* adapter);
    void setDns(struct Interface* adapter, const QStringList dnsList);
    void setAutoDns(struct Interface* adapter);

private slots:
    void on_interface_list_currentTextChanged(const QString &arg1);

    void on_checkbox_dhcp_stateChanged(int arg1);

    void on_btn_apply_addr_clicked();

    void on_btn_refresh_clicked();

    void on_btn_apply_dns_clicked();

    void on_btn_apply_auto_dns_clicked();

    void on_btn_apply_all_clicked();

private:
    Ui::InterfaceManager *ui;
    QList<struct Interface> interfaceList;
};
#endif // INTERFACEMANAGER_H
