#ifndef PEERSTER_MAIN_HH
#define PEERSTER_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>
#include <QKeyEvent>
#include <QVariant>
#include <QByteArray>
#include <QMainWindow>
#include <QDateTime>
#include <QHostInfo>
#include <QVector>
#include <QTimer>
#include <QObject>
#include <QStringList>
#include <QCoreApplication>
#include <QLabel>
#include <QSet>
#include <QHash>
#include <QPair>
#include <QListView>
#include <QStringListModel>
#include <QListWidget>

class MultiLineEdit : public QTextEdit
{
	Q_OBJECT

public:
	MultiLineEdit(QWidget* parent) : QTextEdit(parent) {}

signals:
	void returnPressed();

private:
	void keyPressEvent(QKeyEvent *keyEvent) {
		if(keyEvent->key()==Qt::Key_Return) {
			qDebug() << "User Pressed Return" << endl;
			emit MultiLineEdit::returnPressed();
		}
		else QTextEdit::keyPressEvent(keyEvent);
	}



};

class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog();

	QTextEdit *getTextView();
	MultiLineEdit *getTextLine();
	MultiLineEdit *getAddPeersLine();
	QListWidget *getPrivateMsgList();

private:
	QTextEdit *textview;
	MultiLineEdit *textline;
	MultiLineEdit *addPeersLine;
	QListWidget *privateMsgList;
};


class PrivateChat : public QDialog
{
	Q_OBJECT

public:
	PrivateChat(QString dest);
	MultiLineEdit *getTextLine();
	QString getDest();
	void setDest(QString dest);
	void setLabel(QString label);

private:
	MultiLineEdit *textline;
	QString dest;
	QLabel *privateMessageLabel;
};



class NetSocket : public QUdpSocket
{
	Q_OBJECT

public:
	NetSocket();

	// Bind this socket to a Peerster-specific default port.
	bool bind();
	int getMyPortMin();
	int getMyPortMax();
	int getMyPortVal();

private:
	int myPortMin, myPortMax;
	int myPortVal;
};



class Peer
{

public:
	Peer();
	Peer(QHostAddress ipAddress, quint16 port);
	Peer(QString hostName, QHostAddress ipAddress, quint16 port);
	QString getHostName();
	void setHostName(QString name);
	QHostAddress getAddress();
	void setAddress(QHostAddress address);
	quint16 getPort();
	void setPort(quint16 portNum);

private:
	QString hostName;
	QHostAddress ipAddress;
	quint16 port;
};


// Main window class containing a ChatDialog and NetSocket
class MessageSender : public QMainWindow
{
	Q_OBJECT

public:
	MessageSender();

	QByteArray getSerialized(QVariantMap map);
	int getChatCounter();
	void updateChatCounter();
	QString getOriginID();
	QVariantMap getWantMap();
	void updateStatusMap(QString origin, int seqNo);
	int getNeighbor(int val);
	Peer getNeighbor();
	void addPeer(QString input);
	void updateRoutingTable(QString origin, QHostAddress address, quint16 port);
	void addInitialPrivateMsgPeers();
	QVariantMap createPrivateMessage();

public slots:
	void gotReturnPressed();
	void onReceive();
	void sendTimeoutStatus();
	void sendRouteRumorTimeout();
	void peerLookup(QHostInfo host);
	void addGuiPeer();
	void setupPrivateMessage(QListWidgetItem *listItem);
	void sendPrivateMessage();


private:
	ChatDialog *chat;
	NetSocket *socket;
	PrivateChat *privateChat;
	QString originID;
	int chatCounter;
	int hopLimit;
	bool noForward;
	QVariantMap statusMap;
	QVariantMap msgMap;
	QTimer *timer;
	QTimer *routeRumorTimer;
	QVector<Peer> peerLst;
	QVariantMap portMap;
	QSet<QString> peerCheck;
	QHash<QString, QPair<QHostAddress, quint16>> routeTable;
};

#endif // PEERSTER_MAIN_HH
