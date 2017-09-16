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

private:
	int myPortMin, myPortMax;
};


// Main window class containing a ChatDialog and NetSocket
class MessageSender : public QMainWindow
{
	Q_OBJECT

public:
	MessageSender();

	QByteArray getSerialized();
	void setupConnections();
	int getChatCounter();
	void updateChatCounter();
	QString getOriginID();
	QVariantMap getWantMap();
	void updateStatusMap(QVariantMap map);

public slots:
	void gotReturnPressed();
	void onReceive();


private:
	ChatDialog *chat;
	NetSocket *socket;
	QString originID;
	int chatCounter;
	QVariantMap statusMap;
};

#endif // PEERSTER_MAIN_HH
