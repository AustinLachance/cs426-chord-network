
#include <unistd.h>

#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>

#include "main.hh"

ChatDialog::ChatDialog()
{
	setWindowTitle("Peerster");

	// Read-only text box where we display messages from everyone.
	// This widget expands both horizontally and vertically.
	textview = new QTextEdit(this);
	textview->setReadOnly(true);

	// Small text-entry box the user can enter messages.
	// Default focus upon application start
	// This widget normally expands only horizontally,
	// leaving extra vertical space for the textview widget.
	//
	// You might change this into a read/write QTextEdit,
	// so that the user can easily enter multi-line messages.
	textline = new MultiLineEdit(this);
	textline->setFocus();
	textline->setMaximumHeight(50);

	// Lay out the widgets to appear in the main window.
	// For Qt widget and layout concepts see:
	// http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(textview);
	layout->addWidget(textline);
	setLayout(layout);
}


// Return the textView of the ChatDialog
QTextEdit *ChatDialog::getTextView() {
	return textview;
}


// Return the text editor of the ChatDialog
MultiLineEdit *ChatDialog::getTextLine() {
	return textline;
}


// NetSocket constructor
NetSocket::NetSocket()
{
	// Pick a range of four UDP ports to try to allocate by default,
	// computed based on my Unix user ID.
	// This makes it trivial for up to four Peerster instances per user
	// to find each other on the same host,
	// barring UDP port conflicts with other applications
	// (which are quite possible).
	// We use the range from 32768 to 49151 for this purpose.
	myPortMin = 32768 + (getuid() % 4096)*4;
	myPortMax = myPortMin + 3;
}

// Bind socket to a UDP port
bool NetSocket::bind()
{
	// Try to bind to each of the range myPortMin..myPortMax in turn.
	for (int p = myPortMin; p <= myPortMax; p++) {
		if (QUdpSocket::bind(p)) {
			qDebug() << "bound to UDP port " << p;
			myPortVal = p;
			return true;
		}
	}

	qDebug() << "Oops, no ports in my default range " << myPortMin
		<< "-" << myPortMax << " available";
	return false;
}

// Return the min port
int NetSocket::getMyPortMin() {
	return myPortMin;
}


// Return the max port
int NetSocket::getMyPortMax() {
	return myPortMax;
}

// Return the bound port
int NetSocket::getMyPortVal() {
	return myPortVal;
}

// Constructor for MessageSender class
MessageSender::MessageSender()
{
	// Create instance of ChatDialog and show it
	chat = new ChatDialog();
	chat->show();

	// Create instance of NetSocket and bind it to UDP port
	socket = new NetSocket();
	if (!socket->bind())
		exit(1);

	// Set chatCounter to 1 and create the statusMap
	QVariantMap wantMap;
	statusMap.insert("Want", wantMap);
	chatCounter = 1;

	// Create instance of msgMap



	// Create a unique ID for this instance of MessageSender
	qint64 seedVal = QDateTime::currentMSecsSinceEpoch();
	qsrand(seedVal);
	QString idVal = QString::number(qrand());
	QString hostName = QHostInfo::localHostName();
	originID = hostName + idVal;

	// Get reference to the text editor
	MultiLineEdit* textline = chat->getTextLine();

	// Create timer to send status every 10 seconds
	timer = new QTimer(this);

	// Signal->Slot connections
	connect(textline, SIGNAL(returnPressed()),
		this, SLOT(gotReturnPressed()));

	connect(socket, SIGNAL(readyRead()),
		this, SLOT(onReceive()));

	connect(timer, SIGNAL(timeout()), this, SLOT(sendTimeoutStatus()));

	timer->start(10000);
}


int MessageSender::getChatCounter() {
	return chatCounter;
}


// Increment the number of chats sent by Peerster node X
void MessageSender::updateChatCounter() {
	chatCounter += 1;
}


QString MessageSender::getOriginID() {
	return originID;
}

QVariantMap MessageSender::getWantMap() {
	return statusMap["Want"].toMap();
}


// Update the status of the node when a new msg is received
void MessageSender::updateStatusMap(QVariantMap map) {

	QVariantMap wantMap = getWantMap();
	QString msgOrigin = map["Origin"].toString();
	int msgSeqNo = (map["SeqNo"].toString()).toInt();
	qDebug() << "Msg Sent: " << map["SeqNo"].toString() << endl;
	// If the sending node is not in the map, add it with SeqNo 1 (Msg received was not msg 1) or 2 (Msg received was msg 1)
	if(wantMap.find(msgOrigin) == wantMap.end()) {
		qDebug() << "Adding Node to my Network!" << endl;
		int msgNeeded = (msgSeqNo == 1) ? 2 : 1;
		wantMap.insert(msgOrigin, QString::number(msgNeeded));
	}
	// If in the map already, see if the received msg is the next msg wanted
	else if(wantMap[msgOrigin].toString() == map["SeqNo"].toString()) {
		qDebug() << "Already have this node. Updating msg Number!" << endl;
		wantMap[msgOrigin] = (wantMap[msgOrigin].toString()).toInt() + 1;
	}
	qDebug() << "Message I Need is # " << wantMap[msgOrigin].toString() << endl;
}


// Slot method for when return is pressed
void MessageSender::gotReturnPressed()
{
	qDebug() << "Got ReturnPressed Slot" << endl;
	MultiLineEdit *textline = chat->getTextLine();
	int myPortMin = socket->getMyPortMin();
	int myPortMax = socket->getMyPortMax();
	int myPortVal = socket->getMyPortVal();

	// Initially, just echo the string locally.
	// Insert some networking code here...
	qDebug() << "FIX: send message to other peers: " << textline->toPlainText();
	

	// Serialize the msg written in the text editor
	QString str = textline->toPlainText() + " FROM: " + getOriginID() + " MSG# " + QString::number(chatCounter);
	QVariantMap map;
	map.insert("ChatText", str);
	map.insert("Origin", originID);
	map.insert("SeqNo", chatCounter);

	QByteArray serializedMsg = getSerialized(map);
	qDebug() << "SERIALIZED: " << serializedMsg.toHex() << endl;
	for (int i = myPortMin; i <= myPortMax; i++) {
		// if(i != myPortVal) {
			qDebug() << "SENT: " << textline->toPlainText() << " TO PORT: " << i << endl;
			socket->writeDatagram(serializedMsg, QHostAddress::LocalHost, i);
		// }
	}



	updateChatCounter();

	// Clear the textline to get ready for the next input message.
	textline->clear();
}


// Slot method to receive incoming msg from another peerster node
void MessageSender::onReceive()
{

	// Initialize byte array to store incoming msg & resize to required length of msg
	QByteArray *serializedMsg = new QByteArray();
	int incomingSize = socket->pendingDatagramSize();
	serializedMsg->resize(incomingSize);

	// Vars to hold port & address of sender's host
	quint16 *port = new quint16();
	QHostAddress *host = new QHostAddress();
	QVariantMap receivedMap;

	// Read in msg to serializedMsg. Deserialize msg and store in receivedMap
	socket->readDatagram(serializedMsg->data(), incomingSize, host, port);
	QDataStream stream(serializedMsg, QIODevice::ReadOnly);
	stream >> receivedMap;

	qDebug() << "Receiving message from " << *port << endl;

	// Status Message
	if(receivedMap.contains("Want")) {
		bool sendStatus = false;
		QVector<QVariantMap> sendLst;

		qDebug() << "is STATUS " << endl;
		//compare statuses
		QVariantMap senderStatus = receivedMap["Want"].toMap();
		QVariantMap myStatus = statusMap["Want"].toMap();
		for(auto k: myStatus.keys()) {
			QString origin = k;
			qDebug() << "Comparing msgs from " << origin << endl;
			// If origin does not exist at sender's node, send everything we have for origin
			if(!senderStatus.contains(origin)) {
				qDebug() << "Sender doesnt have " << origin << " so sending it now" << endl;
				for(int i=1; i < myStatus[origin].toInt(); i++) {
					QString key = origin + QString::number(i);
					QVariantMap msgToSend = msgMap[key].toMap();
					sendLst.push_back(msgToSend);
					// QByteArray byteArrayToSender = getSerialized(msgToSend);
					// socket->writeDatagram(byteArrayToSender, QHostAddress::LocalHost, *port);
				}
			}
			else {
				int numMsgSender = senderStatus[origin].toInt();
				int numMsgLocal = myStatus[origin].toInt();

				// Send everything from origin the sender doesn't already have
				if(numMsgSender < numMsgLocal) {
					qDebug() << "Sender has some msgs but not all from " << origin << endl;
					for(int i=numMsgSender; i < numMsgLocal; i++) {
						QString key = origin + QString::number(i);
						qDebug() << "KEY HERE IS " << key << endl;
						QVariantMap msgToSend = msgMap[key].toMap();
						qDebug() << msgToSend << endl;
						sendLst.push_back(msgToSend);
						// QByteArray byteArrayToSender = getSerialized(msgToSend);
						// socket->writeDatagram(byteArrayToSender, QHostAddress::LocalHost, *port);
					}
				}
				// Send a status message to get missing messages from sender
				else if(numMsgLocal < numMsgSender) {
					// QByteArray byteArrayToSender= getSerialized(statusMap);
					// socket->writeDatagram(byteArrayToSender, QHostAddress::LocalHost, *port);
					sendStatus = true;
				}
				else {
					qDebug() << "Everything same do nothing" << endl;
				}

			}
		}
		for(auto k: senderStatus.keys()) {
			if(!senderStatus.contains(k)) {
				sendStatus = true;
			}
		}
		for(int i=0; i<sendLst.size(); i++) {
			qDebug() << "SENDING! " << sendLst[i] << endl;
			QByteArray byteArrayToSender = getSerialized(sendLst[i]);
			socket->writeDatagram(byteArrayToSender, QHostAddress::LocalHost, *port);
		}
		if(sendStatus) {
			QByteArray byteArrayToSender= getSerialized(statusMap);
			socket->writeDatagram(byteArrayToSender, QHostAddress::LocalHost, *port);
		}
	}
	// Rumor Message
	else {
		QString origin = receivedMap["Origin"].toString();
		qDebug() << "is RUMOR from " << origin << endl;
		int seqNo = receivedMap["SeqNo"].toInt();
		QString key = origin + receivedMap["SeqNo"].toString();
		QString msg = receivedMap["ChatText"].toString();
		qDebug() << "Key is " << key << endl;

		// If new msg then start mongering with random neighbor & send status
		if(!msgMap.contains(key)) {
			qDebug() << "new message: " << key << " " << msg << endl;
			msgMap.insert(key, receivedMap);
			qDebug() << msgMap <<endl;

			chat->getTextView()->append(msg);

			// Update Status Map
			qDebug() << "Updating Status Map" << endl;
			QVariantMap wantMap = getWantMap();
			if(!wantMap.contains(origin)) {
				qDebug() << "inserting " << origin << endl;
				if(seqNo == 1) {
					wantMap.insert(origin, 2);
				}
				else wantMap.insert(origin, 1);
			}
			else {
				int currentSeqNo = wantMap[origin].toInt();
				if(seqNo == currentSeqNo) {
					wantMap[origin] = wantMap[origin].toInt() + 1;
				}
			}
			statusMap["Want"] = wantMap;

			// Start mongering
			int neighbor = getNeighbor(socket->getMyPortVal());
			QByteArray byteArrayToSender = getSerialized(receivedMap);
			socket->writeDatagram(byteArrayToSender, QHostAddress::LocalHost, neighbor);

			// Send status
			qDebug() << "Sending status to " << *port << endl;
			byteArrayToSender = getSerialized(statusMap);
			socket->writeDatagram(byteArrayToSender, QHostAddress::LocalHost, *port);
		}
		else {
			qDebug() << "Already Have this message" << endl;
		}
	}
}


int MessageSender::getNeighbor(int val) {
	int min = socket->getMyPortMin();
	int max = socket->getMyPortMax();
	qint64 seedVal = QDateTime::currentMSecsSinceEpoch();
	qsrand(seedVal);
	int randVal = qrand();
	if(val == min) {
		return min+1;
	}
	else if(val == max) {
		return max-1;
	}
	else return (randVal%2) ? val-1 : val+1;
}

void MessageSender::sendTimeoutStatus() {
	qDebug() << "TIMEOUT!!!" <<endl;
	int myPortVal = socket->getMyPortVal();
	int neighbor = getNeighbor(myPortVal);
	QByteArray byteArrayToSender= getSerialized(statusMap);
	socket->writeDatagram(byteArrayToSender, QHostAddress::LocalHost, neighbor);
	timer->start(10000);
}

// Method to serialize text sent by a peerster node
QByteArray MessageSender::getSerialized(QVariantMap map) {
	// QVariantMap map;
	QByteArray out;
	// Retrieve msg from text editor
	// MultiLineEdit *textline = chat->getTextLine();
	// QString str = textline->toPlainText() + " FROM: " + getOriginID() + " MSG# " + QString::number(chatCounter); 

	// Msg being sent. Origin of msg. Sequence # of msg
	// map.insert("ChatText", str);
	// map.insert("Origin", originID);
	// map.insert("SeqNo", chatCounter);
	QDataStream stream(&out, QIODevice::WriteOnly);
	stream << map;
	return out;
}

// UNUSED (DELETE LATER)
void MessageSender::setupConnections() {
	MultiLineEdit* textline = chat->getTextLine();

	// Register a callback on the textline's returnPressed signal
	// so that we can send the message entered by the user.
	connect(textline, SIGNAL(returnPressed()),
		this, SLOT(gotReturnPressed()));
}


int main(int argc, char **argv)
{
	// Initialize Qt toolkit
	QApplication app(argc,argv);

	// Create an instance of messageSender (super class containing chatDialog & socket)
	MessageSender msgSend;

	// Enter the Qt main loop; everything else is event driven
	return app.exec();
}

