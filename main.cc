
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

	QLabel *editorLabel = new QLabel(this);
	editorLabel->setText("Send messages here!");
	
	addPeersLine = new MultiLineEdit(this);
	addPeersLine->setMaximumHeight(50);

	QLabel *peersLabel = new QLabel(this);
	peersLabel->setText("Add peer with host:port");

	// Lay out the widgets to appear in the main window.
	// For Qt widget and layout concepts see:
	// http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
	// QStringList list;
	// list << "Hello" << "Yep";

	QLabel *privateMsgLabel = new QLabel(this);
	privateMsgLabel->setText("Slide into the DMs");
	privateMsgList = new QListWidget();

	// Text editor for sending messages and adding new friends
	QVBoxLayout *editingLayout = new QVBoxLayout();
	editingLayout->addWidget(editorLabel);
	editingLayout->addWidget(textline);
	editingLayout->addWidget(peersLabel);
	editingLayout->addWidget(addPeersLine);

	// List of friends that you can send private messages
	QVBoxLayout *privateMsgLayout = new QVBoxLayout();
	privateMsgLayout->addWidget(privateMsgLabel);
	privateMsgLayout->addWidget(privateMsgList);

	QHBoxLayout *bottomLayout = new QHBoxLayout();
	bottomLayout->addLayout(editingLayout);
	bottomLayout->addLayout(privateMsgLayout);

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->addWidget(textview);
	mainLayout->addLayout(bottomLayout);
	setLayout(mainLayout);
}


// Return the textView of the ChatDialog
QTextEdit *ChatDialog::getTextView() {
	return textview;
}


// Return the text editor of the ChatDialog
MultiLineEdit *ChatDialog::getTextLine() {
	return textline;
}


// Return the add Peers text line of the ChatDialog
MultiLineEdit *ChatDialog::getAddPeersLine() {
	return addPeersLine;
}

QListWidget *ChatDialog::getPrivateMsgList() {
	return privateMsgList;
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


Peer::Peer() {

}

Peer::Peer(QHostAddress ipAddress, quint16 port)
{
	this->ipAddress = ipAddress;
	this->port = port;
}


// Functions for peer
Peer::Peer(QString hostName, QHostAddress ipAddress, quint16 port)
{
	this->hostName = hostName;
	this->ipAddress = ipAddress;
	this->port = port;
}


QString Peer::getHostName() {
	return hostName;
}


void Peer::setHostName(QString name) {
	hostName = name;
}


QHostAddress Peer::getAddress() {
	return ipAddress;
}


void Peer::setAddress(QHostAddress address) {
	ipAddress = address;
}


quint16 Peer::getPort() {
	return port;
}


void Peer::setPort(quint16 portNum) {
	port = portNum;
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

	// Private chat window and hop limit
	privateChat = new PrivateChat("");
	MultiLineEdit *privateChatEditor = privateChat->getTextLine();
	hopLimit = 5;

	// Set chatCounter to 1 and create the statusMap and set noForward flag to default false
	QVariantMap wantMap;
	statusMap.insert("Want", wantMap);
	chatCounter = 1;
	noForward = false;

	// Add local peers
	int portMin = socket->getMyPortMin();
	int portMax = socket->getMyPortMax();
	int myPort = socket->getMyPortVal();
	for(int i=portMin; i<=portMax; i++) {
		if(i != myPort) {
			Peer* newPeer = new Peer(QHostInfo::localHostName(), QHostAddress::LocalHost, i);
			newPeer->setHostName(QHostInfo::localHostName());
			newPeer->setAddress(QHostAddress::LocalHost);
			newPeer->setPort(i);
			peerLst.push_back(*newPeer);

			QHostAddress address = QHostAddress::LocalHost;
			QString peerKey = address.toString() + ":" + QString::number(i);
			peerCheck.insert(peerKey);
		}
	}

	// Add command line peers
	QStringList args = QCoreApplication::arguments();
	if(args.size() > 1) {
		for(int i=1; i < args.size(); i++) {
			if(args[i] == "-noforward") {
				qDebug() << "NO FORWARD!!!" << endl;
				noForward = true;
			}
			else {
				addPeer(args[i]);
			}
		}
	}

	// Create a unique ID for this instance of MessageSender
	qint64 seedVal = QDateTime::currentMSecsSinceEpoch();
	qsrand(seedVal);
	QString idVal = QString::number(qrand());
	QString hostName = QHostInfo::localHostName();
	originID = hostName + idVal;

	// Get reference to the text editors and private message list
	MultiLineEdit* textline = chat->getTextLine();
	MultiLineEdit* addPeersLine = chat->getAddPeersLine();
	QListWidget *privateMsgList = chat->getPrivateMsgList();
	addInitialPrivateMsgPeers();

	// Create timer to send status every 10 seconds
	timer = new QTimer(this);

	// Create routing timer to send route rumor every minute
	routeRumorTimer = new QTimer(this);


	// ******** Signal->Slot connections ************************************************

	// User presses return after entering chat message
	connect(textline, SIGNAL(returnPressed()),
		this, SLOT(gotReturnPressed()));

	// User presses return after enter a "host:port" to add a peer
	connect(addPeersLine, SIGNAL(returnPressed()),
		this, SLOT(addGuiPeer()));

	// User double clicks a peer to start a private message
	connect(privateMsgList, SIGNAL(itemActivated(QListWidgetItem *)), this, SLOT(setupPrivateMessage(QListWidgetItem *)));

	// node receives a message
	connect(socket, SIGNAL(readyRead()),
		this, SLOT(onReceive()));

	// Route rumor message timer timeout
	connect(routeRumorTimer, SIGNAL(timeout()), this, SLOT(sendRouteRumorTimeout()));

	// Rumor Mongering timer timeout
	connect(timer, SIGNAL(timeout()), this, SLOT(sendTimeoutStatus()));

	// User sends a private message
	connect(privateChatEditor, SIGNAL(returnPressed()), this, SLOT(sendPrivateMessage()));
	// ********************************************************************************



	timer->start(10000);

	// Send route rumor message to peers
	sendRouteRumorTimeout();
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
void MessageSender::updateStatusMap(QString origin, int seqNo) {

	qDebug() << "Updating Status Map with " << origin << " " << QString::number(seqNo) << endl;
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
}


// Slot method for when return is pressed
void MessageSender::gotReturnPressed()
{
	qDebug() << "Got ReturnPressed Slot" << endl;
	MultiLineEdit *textline = chat->getTextLine();

	// Initially, just echo the string locally.
	// Insert some networking code here...
	qDebug() << "FIX: send message to other peers: " << textline->toPlainText();
	

	// Serialize the msg written in the text editor
	QString str = textline->toPlainText();
	QVariantMap map;
	map.insert("ChatText", str);
	map.insert("Origin", originID);
	map.insert("SeqNo", chatCounter);
	QByteArray serializedMsg = getSerialized(map);

	// Add our own message to our msgMap & update status map
	QString key = originID + QString::number(chatCounter);
	msgMap.insert(key, map);
	chat->getTextView()->append(str);
	updateStatusMap(originID, chatCounter);

	int numPeers = peerLst.size();
	qDebug() << "I HAVE " << QString::number(numPeers) << " PEERS!" << endl;

	for (int i = 0; i < numPeers; i++) {
			Peer tempPeer = peerLst[i];
			qDebug() << "SENT: " << textline->toPlainText() << "TO ADDRESS: " << tempPeer.getAddress() << " TO PORT: " << tempPeer.getPort()<< endl;
			socket->writeDatagram(serializedMsg, tempPeer.getAddress(), tempPeer.getPort());
	}
	// Send message to all peers

	updateChatCounter();
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
	quint16 *senderPort = new quint16();
	QHostAddress *senderAddress = new QHostAddress();
	QVariantMap receivedMap;

	// Read in msg to serializedMsg. Deserialize msg and store in receivedMap
	socket->readDatagram(serializedMsg->data(), incomingSize, senderAddress, senderPort);
	QDataStream stream(serializedMsg, QIODevice::ReadOnly);
	stream >> receivedMap;

	qDebug() << "Receiving message from " << *senderAddress << " " << *senderPort << endl;


	// If receiving message from an unregistered peer, add to our peer list
	QString peerKey = senderAddress->toString() + ":" + QString::number(*senderPort);
	if(!peerCheck.contains(peerKey)) {
		addPeer(peerKey);
	}

	bool isDirectRoute = true;
	// If message contains LastIP/LastPort then add node to peer list
	if(receivedMap.contains("LastIP") && receivedMap.contains("LastPort")) {
		isDirectRoute = false;
		qDebug() << "LastIP and LastPort Found. Add new peer!!!" << endl;
		

		quint32 addressNum = receivedMap.value("LastIP").toUInt();
		QHostAddress hostAddress = QHostAddress(addressNum);
		QString finalAddress = hostAddress.toString();
		QString port = QString::number(receivedMap.value("LastPort").toInt());
		QString peerKey = finalAddress + ":" + port;

		qDebug() << "LastIP " << finalAddress << endl;
		qDebug() << "LastPort " << port << endl;

		addPeer(peerKey);
	}

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

						// if noForward flag is set then only send route rumors (messages without chat text)
						if(!noForward || !msgToSend.contains("ChatText")) {
							sendLst.push_back(msgToSend);
						}
					}
				}
				// Send a status message to get missing messages from sender
				else if(numMsgLocal < numMsgSender) {
					sendStatus = true;
				}
				else {
					qDebug() << "Everything same do nothing" << endl;
				}

			}
		}

		// Check sender's status map. If we are missing msgs, send status
		for(auto k: senderStatus.keys()) {
			if(!senderStatus.contains(k)) {
				sendStatus = true;
			}
		}

		// Send all messages in the send list
		for(int i=0; i<sendLst.size(); i++) {
			qDebug() << "SENDING! " << sendLst[i] << endl;
			QByteArray byteArrayToSender = getSerialized(sendLst[i]);
			socket->writeDatagram(byteArrayToSender, *senderAddress, *senderPort);
		}

		// Send our status
		if(sendStatus) {
			QByteArray byteArrayToSender= getSerialized(statusMap);
			socket->writeDatagram(byteArrayToSender, *senderAddress, *senderPort);
		}
	}
	// Private Msg
	else if(receivedMap.contains("Dest")) {

		// Get info from message
		QString dest = receivedMap["Dest"].toString();
		QString origin = receivedMap["Origin"].toString();
		quint32 hopLimit = receivedMap["HopLimit"].toInt();
		QString msg = receivedMap["ChatText"].toString();

		// decrement hopLimit
		hopLimit--;

		// If the private msg was sent here then display it
		if(dest == originID) {
			qDebug() << "Received my private message from " << origin << endl;
			chat->getTextView()->append(msg);
		}
		// Forward the message along to the next hop if noForward flag is not set and hops remain
		else if(routeTable.contains(dest) && hopLimit > 0 && !noForward) {
			qDebug() << "Private message not for me. Hops Remaining: " << QString::number(hopLimit) << endl;
			QPair<QHostAddress, quint16> routingInfo = routeTable.value(dest);
			receivedMap["HopLimit"] = hopLimit;
			QByteArray byteArrayToSender= getSerialized(receivedMap);
			socket->writeDatagram(byteArrayToSender, routingInfo.first, routingInfo.second);
		}
	}
	// Rumor Message
	else {
		QString origin = receivedMap["Origin"].toString();
		qDebug() << "is RUMOR from " << origin << endl;
		int seqNo = receivedMap["SeqNo"].toInt();
		QString key = origin + receivedMap["SeqNo"].toString();
		qDebug() << "Key is " << key << endl;


		// Add LastIP and LastPort Keys
		quint32 LastIP = senderAddress->toIPv4Address();
		quint16 LastPort = *senderPort; 
		receivedMap.insert("LastIP", LastIP);
		receivedMap.insert("LastPort", LastPort);

		// If new msg then start mongering with random neighbor & send status
		if(!msgMap.contains(key)) {
			
			// Add msg to msgMap
			msgMap.insert(key, receivedMap);

			// Update Status Map
			updateStatusMap(origin, seqNo);

			// If it is a chat message then display the text
			if(receivedMap.contains("ChatText")) {

				QString msg = receivedMap["ChatText"].toString();
				chat->getTextView()->append(msg);

				qDebug() << "new chat message: " << key << " " << msg << endl;
			}
			else {
				qDebug() << "Is a route rumor message " << endl;
			}

			// Update routing table
			updateRoutingTable(origin, *senderAddress, *senderPort);

			// Start mongering if noForward flag is NOT set
			QByteArray byteArrayToSender;
			if(!noForward) {
				Peer neighbor = getNeighbor();
				int tempPort = neighbor.getPort();
				QHostAddress address = neighbor.getAddress();
				byteArrayToSender = getSerialized(receivedMap);
				socket->writeDatagram(byteArrayToSender, address, tempPort);
			}

			// Send status back to sender
			qDebug() << "Sending status to " << *senderAddress << *senderPort << endl;
			byteArrayToSender = getSerialized(statusMap);
			socket->writeDatagram(byteArrayToSender, *senderAddress, *senderPort);
		}
		else {
			// update routing table is this is a direct route
			if(isDirectRoute) {
				updateRoutingTable(origin, *senderAddress, *senderPort);
			}
			qDebug() << "Already Have this message" << endl;
		}
	}
}


// Sandbox version of getNeighbor. Returns either port+1 or port-1.
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


// Retrieve a random neighbor or -1 if current node has no peers
Peer MessageSender::getNeighbor() {
	int size = peerLst.size();
	// if(size) {
		qint64 seedVal = QDateTime::currentMSecsSinceEpoch();
		qsrand(seedVal);
		int randVal = qrand()%size;
		qDebug() << "I AM " << socket->getMyPortVal() << endl;
		qDebug() << "Index is " << randVal << endl;
		qDebug() << "My Neighbor is "<< peerLst[randVal].getHostName()<< " " << peerLst[randVal].getPort() << endl;
		// return peerLst[randVal].getPort();
		return peerLst[randVal];
	// }
	// return;
	
}


// Slot for when anti-entropy timer expires
void MessageSender::sendTimeoutStatus() {
	qDebug() << "TIMEOUT!!!" <<endl;
	Peer neighbor = getNeighbor();
	int port = neighbor.getPort();
	QHostAddress address = neighbor.getAddress();
	qDebug() << "Sending timeout msg to " << address.toString() << " " << QString::number(port) <<endl;
	
	QByteArray byteArrayToSender= getSerialized(statusMap);
	socket->writeDatagram(byteArrayToSender, address, port);
	timer->start(10000);
}

// Slot for when route rumor timer expires
void MessageSender::sendRouteRumorTimeout() {
	qDebug() << "Route Timeout" <<endl;

	QVariantMap map;
	map.insert("Origin", originID);
	map.insert("SeqNo", chatCounter);

	// Add our own message to our msgMap & update status map
	QString key = originID + QString::number(chatCounter);
	msgMap.insert(key, map);
	updateStatusMap(originID, chatCounter);

	QByteArray serializedMsg = getSerialized(map);

	int numPeers = peerLst.size();

	qDebug() << QString::number(numPeers) <<endl;
	for (int i = 0; i < numPeers; i++) {
		Peer tempPeer = peerLst[i];
		qDebug() << "Sending route rumor to port " << tempPeer.getPort() <<endl;
		socket->writeDatagram(serializedMsg, tempPeer.getAddress(), tempPeer.getPort());
	}
	updateChatCounter();
	routeRumorTimer->start(60000);
}

// Method to serialize text sent by a peerster node
QByteArray MessageSender::getSerialized(QVariantMap map) {
	QByteArray out;
	QDataStream stream(&out, QIODevice::WriteOnly);
	stream << map;
	return out;
}


// Attempt to add a peer using input as host:port
void MessageSender::addPeer(QString input) {
	QStringList tempStr = input.split(':');
	qDebug() << "Adding a new peer " << input << endl;

	// If more or less than one ':' is in the string, invalid format
	if(tempStr.size() == 2) {
		QHostAddress ipTest;

		// Attempt to parse and convert the portnumber
		bool portTest;
		quint16 portNum = tempStr[1].toUShort(&portTest, 10);
		if(!portTest) return;

		// Check if the host is an ip address
		if(ipTest.setAddress(tempStr[0])) {
			Peer *newPeer = new Peer(ipTest, portNum);
			peerLst.push_back(*newPeer);

			QString peerKey = ipTest.toString() + ":" + QString::number(portNum);
			peerCheck.insert(peerKey);
			return;
		}
		// Assume host is a host name and do a lookup
		else {			
			int id = QHostInfo::lookupHost(tempStr[0],this, SLOT(peerLookup(QHostInfo)));
			portMap.insert(QString::number(id), portNum);
			qDebug() << "ID is " << id << endl;
		}
	}
}


// Slot for adding peer through UI
void MessageSender::addGuiPeer()
 {
 	MultiLineEdit *addPeersLine = chat->getAddPeersLine();
 	addPeer(addPeersLine->toPlainText());
 	addPeersLine->clear();
 }


// Slot returning result of the host lookup
void MessageSender::peerLookup(QHostInfo host) {

	// Host lookup failed. Display error
	if(host.error() != QHostInfo::NoError){
		qDebug() << "Bad Host Name" << endl;
	}
	else if(!host.addresses().isEmpty()) {
		QHostAddress hostAddress = host.addresses().first();
		qDebug() << "Adding host address " << hostAddress << endl;

		qDebug() << "LOOKUP ID " << host.lookupId() << endl;

		// Retrieve the port number from the portmap using the lookupId as the key
		int portNum = portMap.value(QString::number(host.lookupId())).toInt();
		qDebug() << "PORTNUM " << portNum << endl;

		// Add a new peer
		Peer *newPeer = new Peer(host.hostName(), hostAddress, portNum);
		peerLst.push_back(*newPeer);

		QString peerKey = hostAddress.toString() + ":" + QString::number(portNum);
		peerCheck.insert(peerKey);
	}
}


void MessageSender::updateRoutingTable(QString origin, QHostAddress address, quint16 port) {
	qDebug() << "Routing Table Insert: << <" << origin << ", <" << address.toString() << ", " << QString::number(port) << ">>" << endl;

	// Create Address, Port pair
	QPair<QHostAddress, quint16> pair;
	pair.first = address;
	pair.second = port;

	// If this is a new node, add it to our private msg list
	if(!routeTable.contains(origin)) {
		chat->getPrivateMsgList()->addItem(origin);
	}

	// add origin, pair to the route table (insert replaces old val if key already exists in table)
	routeTable.insert(origin, pair);
}


void MessageSender::setupPrivateMessage(QListWidgetItem *listItem) {
	qDebug() << "PRIVATE MESSAGE TO " << listItem->text() << endl;
	privateChat->setDest(listItem->text());
	privateChat->setLabel(listItem->text());
	privateChat->show();
	privateChat->activateWindow();
}


QVariantMap MessageSender::createPrivateMessage() {
	QVariantMap map;
	QString message = privateChat->getTextLine()->toPlainText();
	QString dest = privateChat->getDest();

	map.insert("ChatText", message);
	map.insert("Dest", dest);
	map.insert("Origin", originID);
	map.insert("HopLimit", hopLimit);

	return map;
}


void MessageSender::sendPrivateMessage() {
	QVariantMap map = createPrivateMessage();

	QString dest = privateChat->getDest();

	if(routeTable.contains(dest)) {
		QPair<QHostAddress, quint16> routingInfo = routeTable.value(dest);
		QByteArray byteArrayToSender= getSerialized(map);
		socket->writeDatagram(byteArrayToSender, routingInfo.first, routingInfo.second);
	}

	// Clear the text line
	MultiLineEdit *textline = privateChat->getTextLine();
	textline->clear();

}



// *************** DELETE THIS ***************
void MessageSender::addInitialPrivateMsgPeers() {
	qDebug() << "ADDING INITIAL PM PEERS" <<endl;
	QHash<QString, QPair<QHostAddress, quint16>>::iterator i;
	for(i = routeTable.begin(); i != routeTable.end(); i++) {
		qDebug() << "HERE" << endl;
		qDebug() << i.key();
	}
}
// *********************************************


PrivateChat::PrivateChat(QString dest) {
	this->dest = dest;

	textline = new MultiLineEdit(this);
	textline->setFocus();
	textline->setMaximumHeight(50);

	QString label = "Your private message to " + dest;

	privateMessageLabel = new QLabel();
	privateMessageLabel->setText(label);

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->addWidget(privateMessageLabel);
	mainLayout->addWidget(textline);
	setLayout(mainLayout);
}


MultiLineEdit* PrivateChat::getTextLine() {
	return textline;
}


QString PrivateChat::getDest() {
	return dest;
}


void PrivateChat::setDest(QString dest) {
	this->dest = dest;
}


void PrivateChat::setLabel(QString label) {
	this->privateMessageLabel->setText("Your private message to " + label);
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

