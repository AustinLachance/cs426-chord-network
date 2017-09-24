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

private:
	QTextEdit *textview;
	MultiLineEdit *textline;
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


// Main window class containing a ChatDialog and NetSocket
class MessageSender : public QMainWindow
{
	Q_OBJECT

public:
	MessageSender();

	QByteArray getSerialized(QVariantMap map);
	void setupConnections();
	int getChatCounter();
	void updateChatCounter();
	QString getOriginID();
	QVariantMap getWantMap();
	void updateStatusMap(QVariantMap map);
	int getNeighbor(int val);

public slots:
	void gotReturnPressed();
	void onReceive();
	void sendTimeoutStatus();


private:
	ChatDialog *chat;
	NetSocket *socket;
	QString originID;
	int chatCounter;
	QVariantMap statusMap;
	QVariantMap msgMap;
	QTimer *timer;
};

#endif // PEERSTER_MAIN_HH
