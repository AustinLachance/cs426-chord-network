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
#include <QFileDialog>
#include <QPushButton>
#include <QCoreApplication>
#include <QtCrypto/QtCrypto>
#include <QDataStream>
#include <QTable>
#include <QTableWidgetItem>



class MultiLineEdit : public QTextEdit
{
	Q_OBJECT

public:
	MultiLineEdit(QWidget * parent = 0) : QTextEdit(parent) {}

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

	MultiLineEdit *getDownloadFileLine();
	MultiLineEdit *getFileSearchLine();
	MultiLineEdit *getJoinChordLine();
	QListWidget *getFileSearchResultsList();
	QPushButton *getShareFileButton();
	QPushButton *getDisplayTableButton();
	QTextEdit *getPredecessorGui();
	QTextEdit *getSuccessorGui();

private:
	MultiLineEdit *downloadFileLine;
	MultiLineEdit *fileSearchLine;
	MultiLineEdit *joinChordLine;
	QListWidget *fileSearchResultsList;
	QTextEdit *successorGui;
	QTextEdit *predecessorGui;
	QPushButton *shareFileButton;
	QPushButton *displayTableButton;
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
	QString getOriginID();
	int getNeighbor(int val);
	Peer getNeighbor();
	void addPeer(QString input);
	QVariantMap createBlockReply(QString dest, QString origin, QByteArray dataHash, QByteArray data);
	QVariantMap createBlockRequest(QString dest, QString origin);
	QVariantMap createBlockRequest(QString dest, QString origin, QByteArray dataHash);
	QVariantMap createSearchRequest();
	bool findSuccessor(quint32 newNode);
	void sendToPeers(QByteArray data);
	void handleStatusMessage(QVariantMap receivedMap, QHostAddress *senderAddress, quint16 *senderPort);
	void handleRumorMessage(QVariantMap receivedMap, QHostAddress *senderAddress, quint16 *senderPort);
	void handleBlockReplyMessage(QVariantMap receivedMap, QString senderOrigin);
	void handleBlockRequestMessage(QVariantMap receivedMap, QString senderOrigin);
	void handleSearchReplyMessage(QVariantMap receivedMap);
	void localFileSearch(QString searchStr, QString dest);
	void sendPointToPoint(QVariantMap map);
	bool createFingerTable();
	void stabilizePredecessor(QVariantMap map);
	QByteArray findClosestPredecessor(quint32 newNode);
	void joinChord(QString input);
	void handleFindSuccessor(QVariantMap receivedMap);


public slots:
	void onReceive();
	void peerLookup(QHostInfo host);
	void chordLookup(QHostInfo host);
	void joinGuiChord();
	void startFileDownload(QListWidgetItem * listItem);
	void openFileDialog();
	void getFileMetadata(const QStringList &fileList);
	void downloadFile();
	void stabilizeNode();
	void checkPredecessor();
	void deadPredecessor();
	void updateTable();
	void failureProtocol();
	void displayTable();

private:
	ChatDialog *chat;
	NetSocket *socket;
	QFileDialog *fileDialog;
	QString originID;
	quint32 nodeID;
	QVariantMap msgMap;
	QVariantMap fileHash;
	QVariantMap fileMetadata;
	QTimer *searchRequestTimer;
	QVector<Peer> peerLst;
	QVariantMap portMap;
	QSet<QString> peerCheck;
	QHash<QString, QPair<QHostAddress, quint16>> routeTable;
	QByteArray fileReceiving;
	QByteArray fileBuilder;
	QString currentSearch;
	QVariantMap searchResultsMap;
	int updateNum;
	int entryNum;

	QPair<int, QPair<QHostAddress, quint16>> successor;
	QPair<int, QPair<QHostAddress, quint16>> predecessor;
	QList<QPair<int, QPair<QHostAddress, quint16>>> rNearest;
	QHash<QByteArray, QList<QByteArray>>* fingerTable;
	QHash<QByteArray, QList<QByteArray>>* fileTable;

	QTimer *stabilizeTimer;
	QTimer *checkPredTimer;
	QTimer *predResponseTimer;

	QTimer *fingerTableTimer;
	QTimer *successorFailTimer;

	QTable *visualTable;

};

#endif // PEERSTER_MAIN_HH
