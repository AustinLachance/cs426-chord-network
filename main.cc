
#include <unistd.h>

#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>

#include "main.hh"

ChatDialog::ChatDialog()
{
	setWindowTitle("Peerster");

	// Window displaying received chat messages
	textview = new QTextEdit();
	textview->setReadOnly(true);

	// Text editor for sending public messages to the network
	textline = new MultiLineEdit();
	textline->setFocus();
	textline->setMaximumHeight(50);
	QLabel *editorLabel = new QLabel();
	editorLabel->setText("Send messages here!");

	// Create the share file button
	shareFileButton = new QPushButton("Share a File");
	
	// Add Peers text editor
	addPeersLine = new MultiLineEdit();
	addPeersLine->setMaximumHeight(50);
	QLabel *peersLabel = new QLabel();
	peersLabel->setText("Add peer with host:port");
	
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
	editingLayout->addWidget(editorLabel);
	editingLayout->addWidget(textline);
	editingLayout->addWidget(peersLabel);
	editingLayout->addWidget(addPeersLine);
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


	QHBoxLayout *topLayout = new QHBoxLayout();
	topLayout->addWidget(textview);
	topLayout->addLayout(fileLayout);

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->addLayout(topLayout);
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
	nodeID = QCA::Hash("sha1").hash(originID).toByteArray() % 5;

	qDebug() << "My OriginID is " << originID << endl;
	
	//Create a chord fingerTable - id maps to size 3 list (start, end, successor)
	createFingerTable();
	
	//Create a chord fileTable - id maps to all the blocks of a file
	fileTable = new QHash<QByteArray, QList<QByteArray>>();

	// Get references to private members of ChatDialog
	MultiLineEdit* textline = chat->getTextLine();
	MultiLineEdit* addPeersLine = chat->getAddPeersLine();
	MultiLineEdit* downloadFileLine = chat->getDownloadFileLine();
	MultiLineEdit* fileSearchLine = chat->getFileSearchLine();
	QListWidget *fileSearchResultsList = chat->getFileSearchResultsList();
	QPushButton *shareFileButton = chat->getShareFileButton();

	// ******** Signal->Slot connections ************************************************

	// User presses return after entering chat message
	connect(textline, SIGNAL(returnPressed()), this, SLOT(gotReturnPressed()));

	// User presses return after entering a "host:port" to add a peer
	connect(addPeersLine, SIGNAL(returnPressed()),this, SLOT(addGuiPeer()));

	// User presses return after entering a "targetNodeID:hexDataHash" to download a file
	connect(downloadFileLine, SIGNAL(returnPressed()), this, SLOT(downloadFile()));

	// User presses return after entering keyword(s) to search for files
	connect(fileSearchLine, SIGNAL(returnPressed()), this, SLOT(sendInitialSearchRequest()));

	// User double clicks a file to start a download
	connect(fileSearchResultsList, SIGNAL(itemActivated(QListWidgetItem *)), this, SLOT(startFileDownload(QListWidgetItem *)));

	// node receives a message
	connect(socket, SIGNAL(readyRead()), this, SLOT(onReceive()));

	// User clicks the share file button
	connect(shareFileButton, SIGNAL(clicked()), this, SLOT(openFileDialog()));

	// User selects a file(s) to share
	connect(fileDialog, SIGNAL(filesSelected(const QStringList &)), this, SLOT(getFileMetadata(const QStringList &)));
	// ********************************************************************************
}


QString MessageSender::getOriginID() {
	return originID;
}

bool MessageSender::createFingerTable() {
	fingerTable = new QHash<QByteArray, QList<QByteArray>>();
	QByteArray start = 1;
	for (QByteArray i = 0; i < 5; i++) {
		fingerTable->insert((nodeID + start) % 5, QList<QByteArray>() << (nodeID + start) % 5 << (nodeID + start * 2) % 2 << -1);
	}
}

// Slot method for when return is pressed
void MessageSender::gotReturnPressed()
{
	qDebug() << "Got ReturnPressed Slot" << endl;
	MultiLineEdit *textline = chat->getTextLine();

	// Serialize the msg written in the text editor
	QString str = textline->toPlainText();
	QVariantMap map;
	map.insert("ChatText", str);
	map.insert("Origin", originID);
	QByteArray serializedMsg = getSerialized(map);

	// Add our own message to our msgMap & update status map
	msgMap.insert(key, map);
	chat->getTextView()->append(str);

	// Send message to all peers
	sendToPeers(serializedMsg);

	textline->clear();
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


	// If receiving message from an unregistered peer, add to our peer list
	QString peerKey = senderAddress->toString() + ":" + QString::number(*senderPort);
	if(!peerCheck.contains(peerKey)) {
		addPeer(peerKey);
	}

	// If message contains LastIP/LastPort then add node to peer list
	if(receivedMap.contains("LastIP") && receivedMap.contains("LastPort")) {
		
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
			chat->getTextView()->append(msg);

			qDebug() << "new chat message: " << key << " " << msg << endl;
		}
		else {
			qDebug() << "Is a route rumor message " << endl;
		}

		// Start mongering if noForward flag is NOT set
		QByteArray byteArrayToSender;
		if(!noForward) {
			Peer neighbor = getNeighbor();
			int tempPort = neighbor.getPort();
			QHostAddress address = neighbor.getAddress();
			byteArrayToSender = getSerialized(receivedMap);
			socket->writeDatagram(byteArrayToSender, address, tempPort);
		}
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
			

			// Build the new metafile to support large files
			if(this->numInception > 0) {
				fileBuilder.append(receivedData);
			}

			// Request next 20 if possible else reset value of requesting file to null
			if(!fileReceiving.isEmpty()) {
				QVariantMap blockRequest = createBlockRequest(senderOrigin, originID);
				sendPointToPoint(blockRequest);
			}
			// see if this was a nested metafile. if so, construct next metafile from received blocks and begin requesting again
			else if(this->numInception > 0) {
				
				this->numInception--;
				qDebug() << "level complete. Do the next one. Inception level = "<< QString::number(this->numInception) << endl;

				qDebug() << fileBuilder.toHex() << endl << endl;
				fileReceiving = fileBuilder;
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


// Retrieve a random neighbor or -1 if current node has no peers
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

