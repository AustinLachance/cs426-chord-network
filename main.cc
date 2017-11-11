
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

	// Private Message List
	QLabel *privateMsgLabel = new QLabel();
	privateMsgLabel->setText("Slide into the DMs");
	privateMsgList = new QListWidget();

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

	// List of friends that you can send private messages
	QVBoxLayout *privateMsgLayout = new QVBoxLayout();
	privateMsgLayout->addWidget(privateMsgLabel);
	privateMsgLayout->addWidget(privateMsgList);

	QHBoxLayout *bottomLayout = new QHBoxLayout();
	bottomLayout->addLayout(editingLayout);
	bottomLayout->addLayout(privateMsgLayout);

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

QListWidget *ChatDialog::getPrivateMsgList() {
	return privateMsgList;
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

	// Private chat window
	privateChat = new PrivateChat("");
	MultiLineEdit *privateChatEditor = privateChat->getTextLine();

	// Hop limit and budget
	this->hopLimit = 5;
	this->budget = 2;
	this->maxBudget = 100;
	this->maxResults = 10;
	this->numInception = 0;

	// File Dialog Window
	fileDialog = new QFileDialog();
	fileDialog->setFileMode(QFileDialog::ExistingFiles);

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

	qDebug() << "My OriginID is " << originID << endl;

	// Get references to private members of ChatDialog
	MultiLineEdit* textline = chat->getTextLine();
	MultiLineEdit* addPeersLine = chat->getAddPeersLine();
	MultiLineEdit* downloadFileLine = chat->getDownloadFileLine();
	MultiLineEdit* fileSearchLine = chat->getFileSearchLine();
	QListWidget *privateMsgList = chat->getPrivateMsgList();
	QListWidget *fileSearchResultsList = chat->getFileSearchResultsList();
	QPushButton *shareFileButton = chat->getShareFileButton();
	// addInitialPrivateMsgPeers();

	// Create timer to send status every 10 seconds
	timer = new QTimer(this);

	// Create routing timer to send route rumor every minute
	routeRumorTimer = new QTimer(this);

	// Create search request timer to send a new search request with larger budget every second
	searchRequestTimer = new QTimer(this);


	// ******** Signal->Slot connections ************************************************

	// User presses return after entering chat message
	connect(textline, SIGNAL(returnPressed()), this, SLOT(gotReturnPressed()));

	// User presses return after entering a "host:port" to add a peer
	connect(addPeersLine, SIGNAL(returnPressed()),this, SLOT(addGuiPeer()));

	// User presses return after entering a "targetNodeID:hexDataHash" to download a file
	connect(downloadFileLine, SIGNAL(returnPressed()), this, SLOT(downloadFile()));

	// User presses return after entering keyword(s) to search for files
	connect(fileSearchLine, SIGNAL(returnPressed()), this, SLOT(sendInitialSearchRequest()));

	// User double clicks a peer to start a private message
	connect(privateMsgList, SIGNAL(itemActivated(QListWidgetItem *)), this, SLOT(setupPrivateMessage(QListWidgetItem *)));

	// User double clicks a file to start a download
	connect(fileSearchResultsList, SIGNAL(itemActivated(QListWidgetItem *)), this, SLOT(startFileDownload(QListWidgetItem *)));

	// node receives a message
	connect(socket, SIGNAL(readyRead()), this, SLOT(onReceive()));

	// Route rumor message timer timeout
	connect(routeRumorTimer, SIGNAL(timeout()), this, SLOT(sendRouteRumorTimeout()));

	// Rumor Mongering timer timeout
	connect(timer, SIGNAL(timeout()), this, SLOT(sendTimeoutStatus()));

	// Resend search request timer timemout
	connect(searchRequestTimer, SIGNAL(timeout()), this, SLOT(updateSearchRequestBudget()));

	// User sends a private message
	connect(privateChatEditor, SIGNAL(returnPressed()), this, SLOT(sendPrivateMessage()));

	// User clicks the share file button
	connect(shareFileButton, SIGNAL(clicked()), this, SLOT(openFileDialog()));

	// User selects a file(s) to share
	connect(fileDialog, SIGNAL(filesSelected(const QStringList &)), this, SLOT(getFileMetadata(const QStringList &)));
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

	// Send message to all peers
	sendToPeers(serializedMsg);

	updateChatCounter();
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
		handleStatusMessage(receivedMap, senderAddress, senderPort);
	}
	// Private Msg or Block Msg
	else if(receivedMap.contains("Dest")) {

		// Get info from message
		QString dest = receivedMap["Dest"].toString();
		QString origin = receivedMap["Origin"].toString();
		QString senderOrigin = origin;
		quint32 hopsLeft = receivedMap["HopLimit"].toInt();

		// decrement hopLimit
		hopsLeft--;

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
			// Private Msg
			else {
				qDebug() << "Received my private message from " << origin << endl;
				QString msg = receivedMap["ChatText"].toString();
				chat->getTextView()->append(msg);
			}
		}
		// Forward the message along to the next hop if noForward flag is not set and hops remain
		else if(routeTable.contains(dest) && hopsLeft > 0 && !noForward) {
			qDebug() << "Private message not for me. Hops Remaining: " << QString::number(hopLimit) << endl;
			QPair<QHostAddress, quint16> routingInfo = routeTable.value(dest);
			receivedMap["HopLimit"] = hopsLeft;
			QByteArray byteArrayToSender= getSerialized(receivedMap);
			socket->writeDatagram(byteArrayToSender, routingInfo.first, routingInfo.second);
		}
	}
	// Search Request
	else if(receivedMap.contains("Search")) {
		qDebug() << "Got search request" << endl;
		QString senderOrigin = receivedMap["Origin"].toString();
		QString searchStr = receivedMap["Search"].toString();
		localFileSearch(searchStr, senderOrigin);
		budgetSearchRequest(receivedMap);
	}
	// Rumor Message
	else {
		handleRumorMessage(receivedMap, senderAddress, senderPort, isDirectRoute);
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
		searchReplyMap.insert("HopLimit", hopLimit);
		searchReplyMap.insert("SearchReply", searchStr);
		searchReplyMap.insert("MatchNames", fileNameLst);
		searchReplyMap.insert("MatchIDs", fileIdList);
		sendPointToPoint(searchReplyMap);
	}
	else {
		qDebug() << "No results found here" << endl;
	}
}


// Protocol for when a status message is received 
void MessageSender::handleStatusMessage(QVariantMap receivedMap, QHostAddress *senderAddress, quint16 *senderPort) {
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
		if(!myStatus.contains(k)) {
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


// Protocol for handling received rumor message
void MessageSender::handleRumorMessage(QVariantMap receivedMap, QHostAddress *senderAddress, quint16 *senderPort, bool isDirectRoute) {
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

			// Support for large files. numInception represents how many nested metadata files there are
			if(receivedMap.contains("inception")) {
				this->numInception = receivedMap["inception"].toInt();
				qDebug() << "IS INCEPTION!! " << QString::number(this->numInception) << " Levels" << endl;
			}
			else {
				this->numInception = 0;
			}

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
	blockReplyMap.insert("HopLimit", hopLimit);
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


// Create search request using this->currentSearch as the keyword string to send to peers
QVariantMap MessageSender::createSearchRequest() {
	QVariantMap searchMap;
	searchMap.insert("Origin", this->originID);
	searchMap.insert("Search", this->currentSearch);
	searchMap.insert("Budget", this->budget);
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


void MessageSender::sendInitialSearchRequest() {
	// Reset budget to default value & searchResultsMap to empty map
	this->budget = 2;
	searchResultsMap.clear();
	chat->getFileSearchResultsList()->clear();

	MultiLineEdit *fileSearchLine = chat->getFileSearchLine();
	QString searchStr = fileSearchLine->toPlainText();
	this->currentSearch = searchStr;
 	fileSearchLine->clear();

 	qDebug() << "SEARCHING FOR " << searchStr << endl;

 	QVariantMap searchMap = createSearchRequest();
 	budgetSearchRequest(searchMap);

 	this->searchRequestTimer->start(1000);
}


void MessageSender::budgetSearchRequest(QVariantMap searchMap) {
	
	quint32 currentBudget = searchMap["Budget"].toInt() - 1;
	if(currentBudget == 0) return;

	int numPeers = peerLst.size();
	int baseBudget = currentBudget/numPeers;
	int numPeersWithBigBudget = currentBudget % numPeers;

	QSet<QString> peersVisited;

	// Send randomly to numPeersWithBigBudget the search request using baseBudget + 1 
	searchMap["Budget"] = baseBudget+1;
	for(int i=0; i < numPeersWithBigBudget; i++) {
		Peer curPeer = getNeighbor();
		QString peerKey = curPeer.getAddress().toString() + ":" + QString::number(curPeer.getPort());
		if(!peersVisited.contains(peerKey)) {
			peersVisited.insert(peerKey);
			qDebug() << "Sending search request to " << peerKey << endl;
			socket->writeDatagram(getSerialized(searchMap), curPeer.getAddress(), curPeer.getPort());
		}
	}
	// If currentBudget > numPeers Send to all remaining peers with a budget of baseBudget
	searchMap["Budget"] = baseBudget;
	if(baseBudget > 0) {
		for(int i=0; i < numPeers; i++) {
			Peer curPeer = peerLst[i];
			QString peerKey = curPeer.getAddress().toString() + ":" + QString::number(curPeer.getPort());
			if(!peersVisited.contains(peerKey)) {
				qDebug() << "Sending search request to " << peerKey << endl;
				socket->writeDatagram(getSerialized(searchMap), curPeer.getAddress(), curPeer.getPort());
			}
		}
	}
}


void MessageSender::updateSearchRequestBudget() {
	QListWidget *fileSearchResultsList = this->chat->getFileSearchResultsList();

	// Resend below maxBudget and search results are below maxResults
	if(this->budget * 2 <= maxBudget && fileSearchResultsList->count() <= this->maxResults) {

		this->budget *= 2;
		qDebug() << "Sending Timeout Search Request with Budget = " << QString::number(this->budget) << endl;

		QVariantMap searchMap = createSearchRequest();
		budgetSearchRequest(searchMap);

		// Reset searchRequestTimer
		this->searchRequestTimer->start(1000);
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
	    QByteArray temp;	
	    int *metafileInception = new int(-1);
	    int bytesRead;
	    int totalBytes = 0;
	    char *dataStore = (char *)calloc(8000, 1);

	    QString tester = "00be1e7c339a29f8bcd5cf1c11e0172687cfd0f4";
	    QString tester2 = "00677edbeef85aadd54ba4d89bb15d6eb78592af";
	    QByteArray wtf = QByteArray::fromHex(tester.toLatin1());
	    QByteArray wtf2 = QByteArray::fromHex(tester2.toLatin1());
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

		
		QByteArray finalMetafile = recurseMetadata(output, metafileInception);

		QByteArray hashedMetafile = QCA::Hash("sha1").hash(finalMetafile).toByteArray();
		qDebug() << hashedMetafile.toHex() << endl;

		// Insert the metadata into the fileMetadata table
		QVariantMap metadataMap;
		metadataMap.insert("fileName", fileList[i]);
		metadataMap.insert("fileSize", totalBytes);
		metadataMap.insert("metaFile", finalMetafile);
		metadataMap.insert("inception", *metafileInception);
		fileMetadata.insert(hashedMetafile, metadataMap);	
	}
}

QByteArray MessageSender::recurseMetadata(QByteArray data, int *metafileInception) {
	(*metafileInception)++;
	if(data.size() <= 8000) {
		return data;
	}
	else {
		qDebug() << "Metafile too big!!" << endl;
		qDebug() << "Output is " << endl << data.toHex() << endl << endl;
		QCA::Hash shaHash("sha1");
		QByteArray output;
		QDataStream in(data);

	    int bytesRead;
	    

	    do {
	    	QByteArray tempData = data.mid(0, 8000);
	    	QByteArray hashVal = QCA::Hash("sha1").hash(tempData).toByteArray();

	    	// add hash and block to the fileHash table
	    	if(fileHash.contains(hashVal)) {
	    		QVariantList hashValList;
	    		hashValList.append(fileHash[hashVal].toByteArray());
	    		hashValList.append(tempData);
	    		fileHash[hashVal] = hashValList;
	    	}
	    	else {
	    		fileHash[hashVal] = tempData;
	    	}
	    	qDebug() << "Hashing " << hashVal.toHex() << endl << "TO" << fileHash[hashVal].toByteArray().toHex() << endl;

	    	// Append to hash metafile
	    	data.remove(0, 8000);
 	    	output.append(hashVal);
		}
		while(data.size() > 0);

		// Recurse in case new metafile is still too large
		return recurseMetadata(output, metafileInception);
	}
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

	textline = new MultiLineEdit();
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

	// Init Crypto
	QCA::Initializer qcainit;

	// Enter the Qt main loop; everything else is event driven
	return app.exec();
}

