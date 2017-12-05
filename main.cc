
#include <unistd.h>

#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>

#include "main.hh"

ChatDialog::ChatDialog()
{
	setWindowTitle("Peerster");

	// Successor
	QLabel *successorLabel = new QLabel();
	successorLabel->setText("Successor");
	successorGui = new QTextEdit();
	successorGui->setReadOnly(true);
	successorGui->setMaximumHeight(40);
	successorGui->setMaximumWidth(40);

	// // Predecessor
	QLabel *predecessorLabel = new QLabel();
	predecessorLabel->setText("Predecessor");
	predecessorGui = new QTextEdit();
	predecessorGui->setReadOnly(true);
	predecessorGui->setMaximumHeight(40);
	predecessorGui->setMaximumWidth(40);

	// Create the share file button
	shareFileButton = new QPushButton("Share a File");
	
	// Create the share file button
	displayTableButton = new QPushButton("Show Finger Table");

	//Join Chord text editor
	joinChordLine = new MultiLineEdit();
	joinChordLine->setMaximumHeight(50);
	QLabel* chordLabel = new QLabel();
	chordLabel->setText("Join a peer's chord with host:port");

	// Download File text editor
	downloadFileLine = new MultiLineEdit();
	downloadFileLine->setMaximumHeight(50);
	QLabel *downloadFileLabel = new QLabel();
	downloadFileLabel->setText("Download a file with targetNodeID:hexDataHash");

	// File Search text editor
	fileSearchLine = new MultiLineEdit();
	fileSearchLine->setMaximumHeight(50);
	QLabel *fileSearchLabel = new QLabel();
	fileSearchLabel->setText("Search for files (separate keywords by spaces)");

	// File Search Results List
	QLabel *fileSearchResultsLabel = new QLabel();
	fileSearchResultsLabel->setText("File Search Results");
	fileSearchResultsList = new QListWidget();

	// Text editor for sending messages and adding new friends
	QVBoxLayout *editingLayout = new QVBoxLayout();
	editingLayout->addWidget(chordLabel);
	editingLayout->addWidget(joinChordLine);

	QHBoxLayout *bottomLayout = new QHBoxLayout();
	bottomLayout->addLayout(editingLayout);

	QVBoxLayout *fileLayout = new QVBoxLayout();
	fileLayout->addWidget(shareFileButton);
	fileLayout->addWidget(downloadFileLabel);
	fileLayout->addWidget(downloadFileLine);
	fileLayout->addWidget(fileSearchLabel);
	fileLayout->addWidget(fileSearchLine);
	fileLayout->addWidget(fileSearchResultsLabel);
	fileLayout->addWidget(fileSearchResultsList);

	QHBoxLayout *predInfo = new QHBoxLayout();
	predInfo->addWidget(predecessorLabel);
	predInfo->addWidget(predecessorGui);

	QHBoxLayout *succInfo = new QHBoxLayout();
	succInfo->addWidget(successorLabel);
	succInfo->addWidget(successorGui);

	QVBoxLayout *nodeInfo = new QVBoxLayout();
	nodeInfo->addLayout(predInfo);
	nodeInfo->addLayout(succInfo);
	nodeInfo->addWidget(displayTableButton);

	QHBoxLayout *topLayout = new QHBoxLayout();
	topLayout->addLayout(nodeInfo);
	topLayout->addLayout(fileLayout);

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->addLayout(topLayout);
	mainLayout->addLayout(bottomLayout);
	setLayout(mainLayout);
}

// Return the predecessor Gui
QTextEdit *ChatDialog::getPredecessorGui() {
	return predecessorGui;
}

// Return the successor Gui
QTextEdit *ChatDialog::getSuccessorGui() {
	return successorGui;
}


// Return the join Chord text line of the ChatDialog
MultiLineEdit *ChatDialog::getJoinChordLine() {
	return joinChordLine;
}


// Return the download file text line of the ChatDialog
MultiLineEdit *ChatDialog::getDownloadFileLine() {
	return downloadFileLine;
}

// Return the file search text line of the ChatDialog
MultiLineEdit *ChatDialog::getFileSearchLine() {
	return fileSearchLine;
}


// Return the file search results list of the ChatDialog
QListWidget *ChatDialog::getFileSearchResultsList() {
	return fileSearchResultsList;
}


// Return the share file button of the ChatDialog
QPushButton *ChatDialog::getShareFileButton() {
	return shareFileButton;
}

// Return the demo finger table button of the ChatDialog
QPushButton *ChatDialog::getDisplayTableButton() {
	return displayTableButton;
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

	// File Dialog Window
	fileDialog = new QFileDialog();
	fileDialog->setFileMode(QFileDialog::ExistingFiles);

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

	//Create a chord fingerTable - id maps to size 3 list (start, end, successor)
	fingerTable = new QHash<QByteArray, QList<QByteArray>>();

	//Create a chord fileTable - id maps to all the blocks of a file
	fileTable = new QHash<QByteArray, QList<QByteArray>>();

	//Create and don't show visual table
	visualTable = new QTableWidget(7, 5, this);

	// Create a unique ID for this instance of MessageSender
	qint64 seedVal = QDateTime::currentMSecsSinceEpoch();
	qsrand(seedVal);
	QString idVal = QString::number(qrand());
	QString hostName = QHostInfo::localHostName();
	originID = hostName + idVal;
	QCA::Initializer qcainit;

	QByteArray nodeHash = QCA::Hash("sha1").hash(originID.toLatin1()).toByteArray();
	QDataStream in(nodeHash.right(2));
	in.setByteOrder(QDataStream::BigEndian);
	quint16 result;
	in >> result;
	nodeID = result % 256;
	updateNum = (nodeID + 1) % 256;
	entryNum = 1;
	chat->setWindowTitle("Node " + QString::number(nodeID));



	qDebug() << "My OriginID is " << originID << endl;
	qDebug() << "My nodeID TEST is " << QString::number(nodeID) << endl;

	// Add command line peers
	QStringList args = QCoreApplication::arguments();
	createFingerTable();

	// Create a timer for chord stabilization
	stabilizeTimer = new QTimer(this);

	// Timer to check the status of this node's predecessor
	checkPredTimer = new QTimer(this);

	// Timer waiting for a response from predecessor
	predResponseTimer = new QTimer(this);

	// Timer to update fingerTable
	fingerTableTimer = new QTimer(this);

	// Timer to detect a failed successor
	successorFailTimer = new QTimer(this);

	// Get references to private members of ChatDialog
	MultiLineEdit* downloadFileLine = chat->getDownloadFileLine();
	MultiLineEdit* fileSearchLine = chat->getFileSearchLine();
	MultiLineEdit* joinChordLine = chat->getJoinChordLine();
	QListWidget *fileSearchResultsList = chat->getFileSearchResultsList();
	QPushButton *shareFileButton = chat->getShareFileButton();
	QPushButton *displayTableButton = chat->getDisplayTableButton();
	successor.first = 257;
	predecessor.first = 257;
	rNearest.append(successor);
	chat->getSuccessorGui()->append(QString::number(257));
	chat->getPredecessorGui()->append(QString::number(257));

	// ******** Signal->Slot connections ***********************************************

	// User presses return after entering a "host:port" to join a peer's chord
	connect(joinChordLine, SIGNAL(returnPressed()), this, SLOT(joinGuiChord()));

	// User presses return after entering a "targetNodeID:hexDataHash" to download a file
	connect(downloadFileLine, SIGNAL(returnPressed()), this, SLOT(downloadFile()));

	// User double clicks a file to start a download
	connect(fileSearchResultsList, SIGNAL(itemActivated(QListWidgetItem *)), this, SLOT(startFileDownload(QListWidgetItem *)));

	// node receives a message
	connect(socket, SIGNAL(readyRead()), this, SLOT(onReceive()));

	// User clicks the share file button
	connect(shareFileButton, SIGNAL(clicked()), this, SLOT(openFileDialog()));
	
	// User clicks the display table button
	connect(displayTableButton, SIGNAL(clicked()), this, SLOT(displayTable()));

	// User selects a file(s) to share
	connect(fileDialog, SIGNAL(filesSelected(const QStringList &)), this, SLOT(getFileMetadata(const QStringList &)));

	// Run chord stabilization protocol
	connect(stabilizeTimer, SIGNAL(timeout()), this, SLOT(stabilizeNode()));

	// Check the state of this node's predecessor
	connect(checkPredTimer, SIGNAL(timeout()), this, SLOT(checkPredecessor()));

	// Node predecessor did not respond in time. Remove it.
	connect(predResponseTimer, SIGNAL(timeout()), this, SLOT(deadPredecessor()));

	// Update the successor for every interval in our table
	connect(fingerTableTimer, SIGNAL(timeout()), this, SLOT(updateTable()));

	// Connect the successor failure timer to the update protocol
	connect(successorFailTimer, SIGNAL(timeout()), this, SLOT(failureProtocol()));

	fingerTableTimer->start(5000);

	stabilizeTimer->start(10000);

	checkPredTimer->start(10000);
	// ********************************************************************************
}


QString MessageSender::getOriginID() {
	return originID;
}

bool MessageSender::createFingerTable() {
	qDebug() << "My Node ID is "<< QString::number(nodeID);
	int start = 1;
	for (int i = 0; i < 8; i++) {
		fingerTable->insert(QByteArray::number((nodeID + start) % 256), QList<QByteArray>() << QByteArray::number((nodeID + start) % 256)
		<< QByteArray::number((nodeID + start * 2) % 256) << QByteArray::number(257) << QByteArray::number(257) << QByteArray::number(257));
		start *= 2;
	}
	for (auto i = fingerTable->begin(); i != fingerTable->end(); i++) {
		qDebug() << i.key() << i.value() << endl;
	}
	return true;
}


// Run chord stabilization protocol
void MessageSender::stabilizeNode() {
	// Return if we aren't even in a chord network
	if (successor.first == 257 && predecessor.first == 257) {
		qDebug() << "Not in a chord network. Returning" << endl;
		return;
	}
	QPair<QHostAddress, quint16> succInfo = this->successor.second;
	qDebug() << "Stabilizing: checking if my successor is " << QString::number(this->successor.first);
	// Request the predecessor of our successor
	QVariantMap predRequestMap;
	predRequestMap.insert("predecessorRequest", 1);
	successorFailTimer->start(5000);
	qDebug() << succInfo;
	socket->writeDatagram(getSerialized(predRequestMap), succInfo.first, succInfo.second);
}


// Got our successor's predecessor. Continue stabilization
void MessageSender::stabilizePredecessor(QVariantMap map) {

	int tempNodeID = map["nodeID"].toInt();
	int succID = this->successor.first;

	qDebug() << "Got our successor's predecessor!" << endl;
	qDebug() << "Checking if it our new successor is " << QString::number(tempNodeID) << endl;

	QPair<QHostAddress, quint16> tempNodeInfo;
	tempNodeInfo.first = QHostAddress(map["nodeAddress"].toInt());
	tempNodeInfo.second = map["nodePort"].toInt();

	QPair<int, QPair<QHostAddress, quint16>> tempNode(tempNodeID, tempNodeInfo);

	// new node has been inserted between us and our old successor. Make this node new successor, make our old successor the secondSucessor
	if((this->nodeID < tempNodeID && tempNodeID < succID) || (this->nodeID > tempNodeID && tempNodeID < succID && succID < nodeID)
	|| (this->nodeID < tempNodeID && tempNodeID > succID && succID < nodeID)) {
		chat->getSuccessorGui()->clear();
		chat->getSuccessorGui()->append(QString::number(tempNodeID));
		QPair<int, QPair<QHostAddress, quint16>> oldSuccessor = this->successor;
		this->successor = tempNode;
		rNearest.clear();
		rNearest.append(this->successor);
		rNearest.append(oldSuccessor);
		successorFailTimer->stop();
	}

	// new node is not within us and our old successor. Update our secondSuccessor to be our successor's successor
	else {
		int nextSuccessorID = map["nextSuccessorID"].toInt();
		QPair<QHostAddress, quint16> nextSuccessorInfo;
		nextSuccessorInfo.first = QHostAddress(map["nextSuccessorAddress"].toInt());
		nextSuccessorInfo.second = map["nextSuccessorPort"].toInt();
		QPair<int, QPair<QHostAddress, quint16>> nextSuccessorNode(nextSuccessorID, nextSuccessorInfo);
		rNearest.clear();
		rNearest.append(this->successor);
		rNearest.append(nextSuccessorNode);
	}
	qDebug() << "My successor List";
	for (auto k: rNearest) {
		qDebug() << k;
	}
}


// Check the status of this node's predecessor
void MessageSender::checkPredecessor() {

	qDebug() << "send msg to pred to see if still alive" << endl;

	// If predecessor exists check if it is alive
	if(predecessor.first != 257) {
		QPair<QHostAddress, quint16> predInfo = this->predecessor.second;

		QVariantMap checkMap;
		checkMap.insert("predecessorStatusRequest", 1);

		socket->writeDatagram(getSerialized(checkMap), predInfo.first, predInfo.second);

		// Wait 5 seconds for a response
		predResponseTimer->start(5000);
	}
}


// No response from predecessor. Assume dead.
void MessageSender::deadPredecessor() {
	qDebug() << "My predecessor "<< QString::number(this->predecessor.first) << " is dead";
	this->predecessor.first = 257;
	chat->getPredecessorGui()->clear();
	chat->getPredecessorGui()->append(QString::number(predecessor.first));
	// Restart timer to check status of predecessor
	checkPredTimer->start(10000);
}


// Slot method to update fingerTable
void MessageSender::updateTable() {
	// May have to stop all the other timers right quick
	qDebug() << QString::number(entryNum) << "  " << QString::number(updateNum);
	// Don't do anything if we don't have a successor/predecessor
	if (entryNum != 1 && (nodeID + entryNum) % 256 > updateNum) return;
	if (successor.first == 257 && predecessor.first == 257) return;
	entryNum *= 2;
	QVariantMap updateFingerMap;
	updateFingerMap.insert("updateFinger", nodeID);
	updateFingerMap.insert("updateNode", updateNum);
	qDebug() << "Trying to update " << updateNum;
	QByteArray updateFingerMsg = getSerialized(updateFingerMap);
	socket->writeDatagram(updateFingerMsg, this->successor.second.first, this->successor.second.second);
	// for (auto k: fingerTable->keys()) {
	// 	QVariantMap updateFingerMap;
	// 	updateFingerMap.insert("updateFinger", nodeID);
	// 	updateFingerMap.insert("updateNode", k.toInt());
	// 	qDebug() << "Trying to update " << k.toInt();
	// 	QByteArray updateFingerMsg = getSerialized(updateFingerMap);
	// 	socket->writeDatagram(updateFingerMsg, this->successor.second.first, this->successor.second.second);
	// }
}

// Slot method to display fingerTable
void MessageSender::displayTable() {
	visualTable->clear();
	int start = 1;
	for (int i = 0; i < 8; i++) {
		QByteArray key = QByteArray::number((nodeID + start) % 256);
		for (int j = 0; j < (*fingerTable)[key].size(); j++) {
			QTableWidgetItem t = QTableWidgetItem((*fingerTable)[key][j].toString());
			visualTable->setItem(i, j, &t);
		}
		start *= 2;
	}
	visualTable->show();
}

// Slot method to update fingerTable
void MessageSender::failureProtocol() {
	qDebug() << "SHIT";
	if (!rNearest.size() || rNearest[0].first == 257) return;
	rNearest.removeFirst();
	if (!rNearest.size()) return;
	successor.first = rNearest[0].first;
	successor.second.first = rNearest[0].second.first;
	successor.second.second = rNearest[0].second.second;
	chat->getSuccessorGui()->clear();
	chat->getSuccessorGui()->append(QString::number(successor.first));
	//add for stabilize monitoring rNearest successors
}


// Send message to all peers
void MessageSender::sendToPeers(QByteArray data) {
	int numPeers = peerLst.size();
	qDebug() << "I HAVE " << QString::number(numPeers) << " PEERS!" << endl;

	for (int i = 0; i < numPeers; i++) {
			Peer tempPeer = peerLst[i];
			socket->writeDatagram(data, tempPeer.getAddress(), tempPeer.getPort());
	}

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
	qDebug() << receivedMap;


	// If receiving message from an unregistered peer, add to our peer list
	// QString peerKey = senderAddress->toString() + ":" + QString::number(*senderPort);
	// if(!peerCheck.contains(peerKey)) {
	// 	addPeer(peerKey);
	// }

	// Rehash nodeID if collision with existing node
	if (receivedMap.contains("collision")) {
		QString idVal = QString::number(qrand());
		QString hostName = QHostInfo::localHostName();
		originID = hostName + idVal;
		QCA::Initializer qcainit;

		QByteArray nodeHash = QCA::Hash("sha1").hash(originID.toLatin1()).toByteArray();
		QDataStream in(nodeHash.right(2));
		in.setByteOrder(QDataStream::BigEndian);
		quint16 result;
		in >> result;
		nodeID = result % 256;
		updateNum = (nodeID + 1) % 256;
		entryNum = 1;
		chat->setWindowTitle("Node " + QString::number(nodeID));
		QVariantMap newNodeMap;
		newNodeMap.insert("updateNode", nodeID);
		QByteArray newNodeMsg = getSerialized(newNodeMap);
		socket->writeDatagram(newNodeMsg, *senderAddress, *senderPort);
		return;
	}

	// If we receive an updateFinger response, update our table
	if (receivedMap.contains("updateFinger") && receivedMap["updateFinger"].toInt() == nodeID && receivedMap.contains("successorID")) {
		QByteArray updateKey = QByteArray::number(receivedMap["updateNode"].toInt());
		QByteArray successorID = QByteArray::number(receivedMap["successorID"].toInt());
		QByteArray successorAddress = QByteArray::number(QHostAddress(receivedMap["successorAddress"].toInt()).toIPv4Address());
		QByteArray successorPort = QByteArray::number(receivedMap["successorPort"].toInt());
		QList<QByteArray> newEntry;
		newEntry << updateKey << (*fingerTable)[updateKey][1] << successorID << successorAddress << successorPort;
		fingerTable->insert(updateKey, newEntry);
		qDebug() << "Updating table: " << newEntry;
		qDebug() << QHostAddress(receivedMap["successorAddress"].toInt());
		qDebug() << receivedMap["successorPort"].toInt();
		updateNum = (nodeID + entryNum) % 256;
		if (entryNum == 256) {
			for (auto i = fingerTable->begin(); i != fingerTable->end(); i++) {
				qDebug() << i.key() << i.value() << endl;
			}
			// fingerTableTimer->stop();
			entryNum = 1;
			updateNum = (nodeID + 1) % 256;
		}
	}
	// If a new node receives its successor details
	else if (receivedMap.contains("updateNode") && receivedMap.contains("successorID")) {
		successor.first = receivedMap["successorID"].toInt();
		// If we are the first node to join a 1-chord node
		if (receivedMap.contains("creator")) {
			successor.second.first = *senderAddress;
			successor.second.second = *senderPort;
			predecessor.first = receivedMap["successorID"].toInt();
			predecessor.second.first = *senderAddress;
			predecessor.second.second = *senderPort;
			qDebug() << "My successor " << successor;
			qDebug() << "My predecessor " << predecessor;
		}
		else {
			successor.second.first = QHostAddress(receivedMap["successorAddress"].toInt());
			successor.second.second = receivedMap["successorPort"].toInt();
		}
		qDebug() << "My successor is " << QString::number(successor.first);
		chat->getSuccessorGui()->clear();
		chat->getSuccessorGui()->append(QString::number(successor.first));
		chat->getPredecessorGui()->clear();
		chat->getPredecessorGui()->append(QString::number(predecessor.first));
		successorFailTimer->stop();
		predResponseTimer->stop();
		return;
	}
	// If a chord node receives a forwarded message to find a new node's successor
	else if (receivedMap.contains("updateNode") && receivedMap.contains("findSuccessor")) {
		//call successor with this newChord
		handleFindSuccessor(receivedMap);
		return;

	}
	// If a chord node receives a forwarded message to find its closest predecessor to a new node
	else if (receivedMap.contains("updateNode") && receivedMap.contains("findClosestPredecessor")) {
		qDebug() << "supposed to find closest predecessor";
		QByteArray closestPredecessor = findClosestPredecessor(receivedMap["updateNode"].toInt());
		if (closestPredecessor.toInt() == nodeID) {
			receivedMap.remove("findClosestPredecessor");
			receivedMap.insert("findSuccessor", 1);
			handleFindSuccessor(receivedMap);
			return;
		}
		// Shouldn't have to check, but its possible that you yourself are the closest predecessor
		receivedMap.remove("findClosestPredecessor");
		receivedMap.insert("findSuccessor", 1);
		QByteArray findSuccessorMsg = getSerialized(receivedMap);
		socket->writeDatagram(findSuccessorMsg, QHostAddress((*fingerTable)[closestPredecessor][3].toInt()), (*fingerTable)[closestPredecessor][4].toInt());
		return;
	}

	// If message is from a new node joining the chord, first check your own successors.
	// else change message for your successors to find the new node's successor
	else if (receivedMap.contains("updateNode")) {
		if (receivedMap["updateNode"].toInt() == successor.first || receivedMap["updateNode"].toInt() == nodeID) {
			QVariantMap collision;
			collision.insert("collision", 1);
			socket->writeDatagram(getSerialized(collision), QHostAddress(receivedMap["originAddress"].toInt()), receivedMap["originPort"].toInt());
			return;
		}
		// The creator node was finally joined by another node - make this node your successor and predecesssor - 2 node chord
		if (successor.first == 257 && predecessor.first == 257) {
			successor.first = receivedMap["updateNode"].toInt();
			predecessor.first = receivedMap["updateNode"].toInt();
			successor.second.first = *senderAddress;
			successor.second.second = *senderPort;
			predecessor.second.first = *senderAddress;
			predecessor.second.second = *senderPort;
			receivedMap.insert("successorID", nodeID);
			// So the joining node knows to add you as its successor
			receivedMap.insert("creator", 1);
			QByteArray newNodeSuccessorMsg = getSerialized(receivedMap);
			qDebug() << "My successor " << successor;
			qDebug() << "My predecessor " << predecessor;
			chat->getPredecessorGui()->clear();
			chat->getPredecessorGui()->append(QString::number(receivedMap["updateNode"].toInt()));
			chat->getSuccessorGui()->clear();
			chat->getSuccessorGui()->append(QString::number(receivedMap["updateNode"].toInt()));
			socket->writeDatagram(newNodeSuccessorMsg, *senderAddress, *senderPort);
			return;
		}
		else if (findSuccessor(receivedMap["updateNode"].toInt())) {
			//send our successor to the new node - i.e the sender
			receivedMap.insert("successorID", successor.first);
			receivedMap.insert("successorAddress", successor.second.first.toIPv4Address());
			receivedMap.insert("successorPort", successor.second.second);
			QByteArray newNodeSuccessorMsg = getSerialized(receivedMap);
			socket->writeDatagram(newNodeSuccessorMsg, *senderAddress, *senderPort);
			return;
		}
		//have our successor find its closest predecessor, then the closest predecessor will find successor
		receivedMap.insert("findClosestPredecessor", 1);
		receivedMap.insert("originAddress", senderAddress->toIPv4Address());
		receivedMap.insert("originPort", *senderPort);
		QByteArray findClosestPredMsg = getSerialized(receivedMap);
		socket->writeDatagram(findClosestPredMsg, successor.second.first, successor.second.second);
		return;
	}

	// Node is requesting our status. Respond that we're alive
	else if(receivedMap.contains("predecessorStatusRequest")) {
		qDebug() << "Successor is requesting our status" << endl;
		QVariantMap predStatusReply;
		predStatusReply.insert("predecessorStatusReply", 1);
		socket->writeDatagram(getSerialized(predStatusReply), *senderAddress, *senderPort);
	}

	// Got a reply to predecessor check. Predecessor is still alive
	else if(receivedMap.contains("predecessorStatusReply")) {
		qDebug() << "Predecessor is still alive!" << endl;
		// Stop the predResponseTimer since we got a reply
		this->predResponseTimer->stop();

		// Restart checkPredTimer
		this->checkPredTimer->start(10000);
	}

	// Received a request for our predecessor. Send pred info back
	else if(receivedMap.contains("predecessorRequest")) {

		QVariantMap predReply;

		// No predecessor exists (Send 257)
		if(predecessor.first == 257) {
			predReply.insert("predecessorReply", 257);
		}
		// Predecessor exists (Send 1 and routing info)
		else {
			quint32 predID = predecessor.first;
			QPair<QHostAddress, quint16> predInfo = predecessor.second;
			qDebug() << "Got a request for my predecessor  sending my pred " << QString(predID) << endl;
			predReply.insert("predecessorReply", 1);
			predReply.insert("nodeID", predID);
			predReply.insert("nodeAddress", predInfo.first.toIPv4Address());
			predReply.insert("nodePort", predInfo.second);
			predReply.insert("nextSuccessorID", successor.first);
			predReply.insert("nextSuccessorAddress", successor.second.first.toIPv4Address());
			predReply.insert("nextSuccessorPort", successor.second.second);
		}
		qDebug() << "I am sending my predecessor AND successor back to the sender/potential predecessor";
		socket->writeDatagram(getSerialized(predReply), *senderAddress, *senderPort);
	}

	// Received a predecessor reply. Part of stabilization protocol
	else if(receivedMap.contains("predecessorReply")) {

		qDebug() << "Got pred from succ. Check if it is our new succ then check if we are our succ's new pred" << endl;

		successorFailTimer->stop();
		// If Successor has a predecessor run stabilization protocol
		if(receivedMap["predecessorReply"].toInt() != 257) {
			stabilizePredecessor(receivedMap);
		}

		// Get routing info for successor
		QPair<QHostAddress, quint16> succInfo = successor.second;

		// Tell our successor to check if we are its predecessor
		qDebug() << "sending predTest" <<endl;

		QVariantMap predCheck;
		predCheck.insert("predecessorTest", 1);
		predCheck.insert("nodeID", this->nodeID);
		socket->writeDatagram(getSerialized(predCheck), succInfo.first, succInfo.second);
	}

	// Node thinks it might be our predecessor. Check if this is true and stabilize accordingly
	// Last step in successor/predecessor stabilization. Reset timer at end
	else if(receivedMap.contains("predecessorTest")) {
		int tempNodeID = receivedMap["nodeID"].toInt();
		QPair<QHostAddress, quint16> tempNodeInfo(*senderAddress, *senderPort);
		QPair<int, QPair<QHostAddress, quint16>> tempNode(tempNodeID, tempNodeInfo);

		qDebug() << "checking if my new pred is node " << QString::number(tempNodeID) << endl;

		// If predecessor doesn't exist or tempNode falls btw old predecessor and us then update
		if((predecessor.first == 257) || (tempNodeID > predecessor.first && tempNodeID < nodeID) || (predecessor.first > tempNodeID && tempNodeID < nodeID && nodeID < predecessor.first)
		|| (predecessor.first < tempNodeID && tempNodeID > nodeID && predecessor.first > nodeID)) {
			chat->getPredecessorGui()->clear();
			chat->getPredecessorGui()->append(QString::number(tempNodeID));
			predResponseTimer->stop();
			this->predecessor = tempNode;
		}
		else {
			qDebug() << "Nope. Not my predecessor" << endl;
		}

		qDebug() << "Done stabilizing" << endl;
		qDebug() << "Successor: " << QString::number(this->successor.first) << endl;
		qDebug() << "Predecessor: " << QString::number(this->predecessor.first) << endl;

		// Stabilization of successor/predecessor complete. Reset timer
		stabilizeTimer->start(10000);
	}

	// If message contains LastIP/LastPort then add node to peer list
	else if(receivedMap.contains("LastIP") && receivedMap.contains("LastPort")) {

		quint32 addressNum = receivedMap.value("LastIP").toUInt();
		QHostAddress hostAddress = QHostAddress(addressNum);
		QString finalAddress = hostAddress.toString();
		QString port = QString::number(receivedMap.value("LastPort").toInt());
		QString peerKey = finalAddress + ":" + port;

		qDebug() << "LastIP " << finalAddress << endl;
		qDebug() << "LastPort " << port << endl;

		addPeer(peerKey);
	}

	// Private Msg or Block Msg
	else if(receivedMap.contains("Dest")) {

		// Get info from message
		QString dest = receivedMap["Dest"].toString();
		QString origin = receivedMap["Origin"].toString();
		QString senderOrigin = origin;
		// Check if the message was sent here
		if(dest == originID) {
			// Block Requests
			if(receivedMap.contains("BlockRequest")) {
				handleBlockRequestMessage(receivedMap, senderOrigin);
			}
			// Block Reply
			else if(receivedMap.contains("BlockReply")) {
				handleBlockReplyMessage(receivedMap, senderOrigin);
			}
			// Search Reply
			else if(receivedMap.contains("SearchReply")) {
				qDebug() << "Got search reply" << endl;
				handleSearchReplyMessage(receivedMap);

			}
		}
		// Forward the message along to the next hop if noForward flag is not set and hops remain
		else if(routeTable.contains(dest)) {
			QPair<QHostAddress, quint16> routingInfo = routeTable.value(dest);
			QByteArray byteArrayToSender= getSerialized(receivedMap);
			socket->writeDatagram(byteArrayToSender, routingInfo.first, routingInfo.second);
		}
	}
	// Change this for searching in chord
	else if(receivedMap.contains("Search")) {
		qDebug() << "Got search request" << endl;
		QString senderOrigin = receivedMap["Origin"].toString();
		QString searchStr = receivedMap["Search"].toString();
		localFileSearch(searchStr, senderOrigin);
	}
	// Rumor Message
	else {
		handleRumorMessage(receivedMap, senderAddress, senderPort);
	}
}


void MessageSender::localFileSearch(QString searchStr, QString dest) {
	qDebug() << "Searching Locally for  " << searchStr << endl;
	QStringList keywords = searchStr.split(' ');
	int numKeywords = keywords.size();

	QVariantList fileNameLst;
	QVariantList fileIdList;

	for(auto key: fileMetadata.keys()) {
		QVariantMap metaMap = fileMetadata.value(key).toMap();
		QString fileName = metaMap["fileName"].toString();

		for(int i=0; i < numKeywords; i++) {
			if(fileName.contains(keywords[i])) {
				fileNameLst.append(fileName);
				fileIdList.append(key);
			}
		}
	}

	if(!fileNameLst.isEmpty()) {
		qDebug() << "Found files!!!" << endl;
		QVariantMap searchReplyMap;
		searchReplyMap.insert("Dest", dest);
		searchReplyMap.insert("Origin", originID);
		searchReplyMap.insert("SearchReply", searchStr);
		searchReplyMap.insert("MatchNames", fileNameLst);
		searchReplyMap.insert("MatchIDs", fileIdList);
		sendPointToPoint(searchReplyMap);
	}
	else {
		qDebug() << "No results found here" << endl;
	}
}

// Protocol for handling find successor request
void MessageSender::handleFindSuccessor(QVariantMap receivedMap) {
	if (receivedMap["updateNode"].toInt() == successor.first || receivedMap["updateNode"].toInt() == nodeID) {
		QVariantMap collision;
		collision.insert("collision", 1);
		socket->writeDatagram(getSerialized(collision), QHostAddress(receivedMap["originAddress"].toInt()), receivedMap["originPort"].toInt());
		return;
	}
	if (findSuccessor(receivedMap["updateNode"].toInt())) {
			receivedMap.insert("successorID", successor.first);
			receivedMap.insert("successorAddress", successor.second.first.toIPv4Address());
			receivedMap.insert("successorPort", successor.second.second);
			QByteArray newNodeSuccessorMsg = getSerialized(receivedMap);
			socket->writeDatagram(newNodeSuccessorMsg, QHostAddress(receivedMap["originAddress"].toInt()), receivedMap["originPort"].toInt());
			return;
		}
	else {
		receivedMap.remove("findSuccessor");
		receivedMap.insert("findClosestPredecessor", 1);
		QByteArray findClosestPredMsg = getSerialized(receivedMap);
		socket->writeDatagram(findClosestPredMsg, successor.second.first, successor.second.second);
		return;
	}
}


// Protocol for handling received rumor message
void MessageSender::handleRumorMessage(QVariantMap receivedMap, QHostAddress *senderAddress, quint16 *senderPort) {
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

		// If it is a chat message then display the text
		if(receivedMap.contains("ChatText")) {

			QString msg = receivedMap["ChatText"].toString();
			// chat->getTextView()->append(msg);

			qDebug() << "new chat message: " << key << " " << msg << endl;
		}
		else {
			qDebug() << "Is a route rumor message " << endl;
		}

		// Start mongering if noForward flag is NOT set
		QByteArray byteArrayToSender;
		Peer neighbor = getNeighbor();
		int tempPort = neighbor.getPort();
		QHostAddress address = neighbor.getAddress();
		byteArrayToSender = getSerialized(receivedMap);
		socket->writeDatagram(byteArrayToSender, address, tempPort);
	}
}


// Protocol for handling block reply messages
void MessageSender::handleBlockReplyMessage(QVariantMap receivedMap, QString senderOrigin) {
	qDebug() << "IS BLOCK REPLY!!!" << endl;
	QByteArray hashVal = receivedMap["BlockReply"].toByteArray();
	QByteArray receivedData = receivedMap["Data"].toByteArray();

	qDebug() << "DATA RECEIVED! " << receivedData << endl;
	qDebug() << "HASH RECEIVED! " << hashVal.toHex() << endl;
	QByteArray dataHash = QCA::Hash("sha1").hash(receivedData).toByteArray();

	qDebug() << "Data hash! " << dataHash.toHex() << endl;
	if(QCA::Hash("sha1").hash(receivedData).toByteArray() == hashVal) {
		qDebug() << "GOOD MESSAGE" << endl;
		if(fileReceiving.isEmpty()) {
			qDebug() << "is a metafile" << endl;
			fileMetadata.insert(hashVal, receivedData);
			fileReceiving = receivedData;
			QVariantMap blockRequest = createBlockRequest(senderOrigin, originID);
			sendPointToPoint(blockRequest);
		}
		else {
			// if(!fileMetadata.contains(hashVal) && !fileHash.contains(hashVal)) {

			// }
			// else {
			// 	qDebug() << "Dup data message" << endl;
			// }
			qDebug() << "is file data" << endl;
			qDebug() << "data received: " << receivedData.toHex() << endl;
			fileReceiving.remove(0, 20);
			if(fileHash.contains(hashVal)) {
				QVariantList hashValList;
				hashValList.append(fileHash[hashVal].toByteArray());
				hashValList.append(receivedData);
				fileHash[hashVal] = hashValList;
			}
			else{
				fileHash.insert(hashVal, receivedData);
			}

			// Request next 20 if possible else reset value of requesting file to null
			if(!fileReceiving.isEmpty()) {
				QVariantMap blockRequest = createBlockRequest(senderOrigin, originID);
				sendPointToPoint(blockRequest);
			}
		}


	}
	else {
		qDebug() << "Message Different from Hash" << endl;
	}
}


// Protocol for handling block request messages
void MessageSender::handleBlockRequestMessage(QVariantMap receivedMap, QString senderOrigin) {
	qDebug() << "IS BLOCK REQUEST!!!" << endl;
	QByteArray hashVal = receivedMap["BlockRequest"].toByteArray();
	qDebug() << "hashVal " << hashVal.toHex() << endl;
	if(fileMetadata.contains(hashVal)) {
		qDebug() << "is meta" << endl;
		QVariantMap fileMeta = fileMetadata[hashVal].toMap();
		qDebug() << "file requested is " << fileMeta["fileName"].toString();
		QVariantMap blockReply = createBlockReply(senderOrigin, originID, hashVal, fileMeta["metaFile"].toByteArray());

		// Support for large files
		if(fileMeta.contains("inception")) {
			blockReply.insert("inception", fileMeta["inception"].toInt());
		}

		sendPointToPoint(blockReply);
	}
	else if(fileHash.contains(hashVal)) {
		qDebug() << "is block request" << endl;
		QVariantList hashValList = fileHash[hashVal].toList();
		QByteArray dataToSend;
		if(hashValList.isEmpty()) {
			dataToSend = fileHash[hashVal].toByteArray();
		}
		else {
			for(int i=0; i<hashValList.size(); i++) {
				dataToSend = hashValList[i].toByteArray();

				if(QCA::Hash("sha1").hash(dataToSend).toByteArray() == hashVal) {
					break;
				}
			}
		}

		QVariantMap blockReply = createBlockReply(senderOrigin, originID, hashVal, dataToSend);
		sendPointToPoint(blockReply);
	}
	else {
		qDebug() << "is nothing" << endl;
	}
}


void MessageSender::handleSearchReplyMessage(QVariantMap receivedMap) {
	qDebug() << "IS Search Reply!!!" << endl;
	QString searchStr = receivedMap["SearchReply"].toString();
	QString dest = receivedMap["Origin"].toString();
	if(searchStr == currentSearch) {
		QVariantList fileNames = receivedMap["MatchNames"].toList();
		QVariantList fileIDs = receivedMap["MatchIDs"].toList();

		int numFiles = fileNames.size();
		for(int i=0; i < numFiles; i++) {

			// Hash file name to metafile and node that contains file
			QVariantList fileInfo;
			QString file = fileNames[i].toString();

			if(!searchResultsMap.contains(file)) {
				fileInfo.append(dest);
				fileInfo.append(fileIDs[i].toByteArray());
				searchResultsMap.insert(file, fileInfo);

				chat->getFileSearchResultsList()->addItem(file);
			}
		}
	}
}


// Create block reply message
QVariantMap MessageSender::createBlockReply(QString dest, QString origin, QByteArray dataHash, QByteArray data) {
	QVariantMap blockReplyMap;
	blockReplyMap.insert("Dest", dest);
	blockReplyMap.insert("Origin", origin);
	blockReplyMap.insert("BlockReply", dataHash);
	blockReplyMap.insert("Data", data);

	return blockReplyMap;
}


// Create a block request using the first 20 bytes from fileReceiving as the requested data hash
QVariantMap MessageSender::createBlockRequest(QString dest, QString origin) {
	QByteArray requestData(fileReceiving, 20);

	QVariantMap blockRequestMap;
	blockRequestMap.insert("Dest", dest);
	blockRequestMap.insert("Origin", origin);
	blockRequestMap.insert("BlockRequest", requestData);

	return blockRequestMap;
}


// Create chord search message
QVariantMap MessageSender::createSearchRequest() {
	QVariantMap searchMap;
	searchMap.insert("Origin", this->originID);
	searchMap.insert("Search", this->currentSearch);
	return searchMap;
}

// Create a block request requesting dataHash
QVariantMap MessageSender::createBlockRequest(QString dest, QString origin, QByteArray dataHash) {

	QVariantMap blockRequestMap;
	blockRequestMap.insert("Dest", dest);
	blockRequestMap.insert("Origin", origin);
	blockRequestMap.insert("BlockRequest", dataHash);

	return blockRequestMap;
}

// Find the successor for the given ID
bool MessageSender::findSuccessor(quint32 newNode) {
	// The node's successor is this current node's successor
	if (successor.first != 257 && ((nodeID < newNode && newNode < successor.first) || (nodeID < newNode && newNode > successor.first && successor.first < nodeID)
	|| (nodeID > newNode && newNode < successor.first && nodeID > successor.first))) {
		qDebug() << "At node " << QString::number(nodeID) << ", " << QString::number(newNode) << "'s successor is " << QString::number(successor.first);
		return true;
	}
	qDebug() << "At node " << QString::number(nodeID) << ", " << QString::number(newNode) << "'s successor is NOT " << QString::number(successor.first);
	return false;
}

QByteArray MessageSender::findClosestPredecessor(quint32 newNode) {
	int i = 128;
	while (i >= 1) {
		QByteArray fingerKey = QByteArray::number((nodeID + i) % 256);
		qDebug() << fingerKey;
		quint32 successorID = (*fingerTable)[fingerKey][2].toInt();
		if (successorID != 257 && ((nodeID < successorID && successorID < newNode) || (nodeID < successorID && successorID > newNode && newNode < nodeID)
		|| (nodeID > successorID && successorID < newNode && newNode < nodeID))) {
			return fingerKey;
		}
		i /= 2;
	}
	return QByteArray::number(nodeID);
}


// Retrieve a random neighbor or 1 if current node has no peers
Peer MessageSender::getNeighbor() {
	int size = peerLst.size();
	qint64 seedVal = QDateTime::currentMSecsSinceEpoch();
	qsrand(seedVal);
	int randVal = qrand()%size;
	qDebug() << "My Neighbor is "<< peerLst[randVal].getHostName()<< " " << peerLst[randVal].getPort() << endl;
	return peerLst[randVal];
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

// Attempt to join a chord using a peer's host:port
void MessageSender::joinChord(QString input) {
	QStringList tempStr = input.split(':');
	qDebug() << "Joining a chord" << input << endl;

	// If more or less than one ':' is in the string, invalid format
	if(tempStr.size() == 2) {
		QHostAddress ipTest;

		// Attempt to parse and convert the portnumber
		bool portTest;
		quint16 portNum = tempStr[1].toUShort(&portTest, 10);
		if(!portTest) return;

		// Check if the host is an ip address
		if(ipTest.setAddress(tempStr[0])) {
			QVariantMap newNodeMap;
			newNodeMap.insert("updateNode", nodeID);
			QByteArray newNodeMsg = getSerialized(newNodeMap);
			socket->writeDatagram(newNodeMsg, ipTest, portNum);
			return;
		}
		// Assume host is a host name and do a lookup
		else {
			int id = QHostInfo::lookupHost(tempStr[0],this, SLOT(chordLookup(QHostInfo)));
			portMap.insert(QString::number(id), portNum);
			qDebug() << "ID is " << id << endl;
		}
	}
}


// Attempt to download a file using input as targetNodeID:hexadecimalBlocklistHash
void MessageSender::downloadFile() {

	MultiLineEdit *downloadFileLine = chat->getDownloadFileLine();
 	QString input = downloadFileLine->toPlainText();
 	downloadFileLine->clear();

	QString targetNodeID = input.section(':', 0, 0);
	QString hashString = input.section(":", 1, 1);
	QByteArray hashVal = QByteArray::fromHex(hashString.toLatin1());
	qDebug() << "Downloading a file. targetNodeID: " << targetNodeID << " HashVal " << hashString << endl;

	// Create block request
	QVariantMap blockRequest = createBlockRequest(targetNodeID, originID, hashVal);
	QByteArray serialBlockRequest = getSerialized(blockRequest);

	// Send block request to all peers
	sendToPeers(serialBlockRequest);
}


// Slot for joining peer's chord through UI
void MessageSender::joinGuiChord()
 {

 	// createFingerTable();
 	MultiLineEdit *joinChordLine = chat->getJoinChordLine();
 	joinChord(joinChordLine->toPlainText());
 	joinChordLine->clear();
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

// Slot returning result of the chord lookup
void MessageSender::chordLookup(QHostInfo host) {

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

		// Send request to get your successor
		QVariantMap newNodeMap;
		newNodeMap.insert("updateNode", nodeID);
		QByteArray newNodeMsg = getSerialized(newNodeMap);
		socket->writeDatagram(newNodeMsg, hostAddress, portNum);
	}
}


void MessageSender::startFileDownload(QListWidgetItem *listItem) {
	QString fileName = listItem->text();
	qDebug() << "Downloading File " << fileName << endl;

	QVariantList fileInfo = searchResultsMap[fileName].toList();
	QString dest = fileInfo[0].toString();
	QByteArray metaFile = fileInfo[1].toByteArray();

	QVariantMap blockRequestMap = createBlockRequest(dest, originID, metaFile);
	sendPointToPoint(blockRequestMap);
}


void MessageSender::openFileDialog() {
	fileDialog->show();
	fileDialog->activateWindow();
}


void MessageSender::sendPointToPoint(QVariantMap map) {
	QString dest = map["Dest"].toString();
	qDebug() << "Sending p2p to " << dest << endl;
	if(routeTable.contains(dest)) {
		QPair<QHostAddress, quint16> routingInfo = routeTable.value(dest);
		QByteArray byteArrayToSender= getSerialized(map);
		socket->writeDatagram(byteArrayToSender, routingInfo.first, routingInfo.second);
	}
	else {
		qDebug() << "cant send p2p :(" << endl;
	}
}


// Collect the metadata from the files in fileList
void MessageSender::getFileMetadata(const QStringList &fileList) {
	qDebug() << "GET METADATA!!!" << endl;
	qDebug() << fileList << endl;
	int size = fileList.size();
	for(int i=0; i < size; i++) {
		QCA::Hash shaHash("sha1");
		QFile file(fileList[i]);
		QByteArray output;
	    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
	        continue;
	    }
	    QDataStream in(&file);
	    int bytesRead;
	    int totalBytes = 0;
	    char *dataStore = (char *)calloc(8000, 1);

	    do {
	    	bytesRead = in.readRawData(dataStore, 8000);
	    	QByteArray hashVal = QCA::Hash("sha1").hash(dataStore).toByteArray();

	    	// add hash and block to the fileHash table
	    	if(fileHash.contains(hashVal)) {
	    		QVariantList hashValList;
	    		hashValList.append(fileHash[hashVal].toByteArray());
	    		hashValList.append(dataStore);
	    		fileHash[hashVal] = hashValList;
	    	}
	    	else {
	    		fileHash[hashVal] = dataStore;
	    	}

	    	qDebug() << "Hashing " << hashVal.toHex() << endl << "TO" << fileHash[hashVal].toByteArray() << endl;

	    	// Append to hash metafile
 	    	output.append(hashVal);
	    	totalBytes += bytesRead;
		}
		while(bytesRead == 8000);

		QByteArray hashedMetafile = QCA::Hash("sha1").hash(output).toByteArray();
		qDebug() << hashedMetafile.toHex() << endl;

		// Insert the metadata into the fileMetadata table
		QVariantMap metadataMap;
		metadataMap.insert("fileName", fileList[i]);
		metadataMap.insert("fileSize", totalBytes);
		metadataMap.insert("metaFile", output);
		fileMetadata.insert(hashedMetafile, metadataMap);
	}
}


int main(int argc, char **argv)
{
	// Initialize Qt toolkit
	QApplication app(argc,argv);

	// Create an instance of messageSender (super class containing chatDialog & socket)
	MessageSender msgSend;

	// Init Crypto
	QCA::Initializer qcainit;

	// Enter the Qt main loop; everything else is event driven
	return app.exec();
}
