#include "producer_consumer.h"

producer_consumer::producer_consumer(QWidget* parent)
	: QMainWindow(parent)
{
	producerNumber = 0;
	consumerNumber = 0;
	bufferLength = 0;
	srand(time(0));
	ui.setupUi(this);

}

producer_consumer::~producer_consumer()
{}

void producer_consumer::runMainFunction()
{
	//按下“确认”按钮后才能启用以下控件
	ui.goToButton->setEnabled(true);
	ui.preStepButton->setEnabled(true);
	ui.nextStepButton->setEnabled(true);
	ui.stepBox->setEnabled(true);
	ui.stepBox->setValue(0);

	//设置参数
	producerNumber = ui.producerNumberBox->value();
	consumerNumber = ui.consumerNumberBox->value();
	bufferLength = ui.bufferLengthBox->value();
	int itemsToProduce = consumerNumber * 2;
	int itemsToConsume = producerNumber * 2;
	buffer.resize(bufferLength, 0);

	//初始化参数
	in = out = bufferSize = 0;

	//初始化表格
	ui.bufferTable->clear();
	ui.bufferTable->setColumnCount(bufferLength);
	ui.bufferTable->setRowCount(1);
	ui.bufferTable->setGeometry(20, 170, 40 * bufferLength + 20, 60);
	for (int i = 0; i < bufferLength; i++)
	{
		ui.bufferTable->setColumnWidth(i, 40);
		QTableWidgetItem* newItem = new QTableWidgetItem;
		ui.bufferTable->setItem(0, i, newItem);
	}

	//初始化pastbuffer
	pastBuffer.clear();
	pastBuffer.push_back(buffer);

	//初始化可选步骤
	ui.stepBox->setMaximum(consumerNumber * producerNumber * 4);

	//创建线程vector记录将要创建的线程
	std::vector<std::thread> allThreads;

	//先锁住mutex，再释放以达到所有线程尽量同步启动的目的
	std::unique_lock lock(mtx);

	// 创建生产者和消费者线程	
	for (int i = 1; i <= consumerNumber; i++)
	{
		std::thread newConsumer([this, i, itemsToConsume]() {this->consumer(i, itemsToConsume); }); //创建新消费线程
		allThreads.push_back(std::move(newConsumer));
	}
	for (int i = 1; i <= producerNumber; i++)
	{
		std::thread newProducer([this, i, itemsToProduce]() {this->producer(i, itemsToProduce); }); //创建新生产线程
		allThreads.push_back(std::move(newProducer));
	}


	//释放互斥锁，线程启动
	lock.unlock();

	// 等待所有线程完成
	for (auto& curThread : allThreads)
	{
		curThread.join();
	}
	printBufferinfo(0);
	return;
}


// 生产者函数
void producer_consumer::producer(int id, int itemsToProduce)
{
	for (int i = 0; i < itemsToProduce; ++i)
	{

		std::unique_lock<std::mutex> lock(mtx);//获得互斥锁
		cond_empty.wait(lock, [this] { return bufferSize < bufferLength; }); // 等待缓冲区非满

		// 生产数据并放入缓冲区
		bufferSize++;
		buffer[in] = id * 100 + i;
		in = (in + 1) % bufferLength;
		pastBuffer.push_back(buffer);//记录当前buffer
		pastBuffer.back().push_back(id * 100 + i);//记录操作数
		pastBuffer.back().push_back(in);//记录插入点
		pastBuffer.back().push_back(out);//记录输出点
		pastBuffer.back().push_back(0);//记录该步操作为生产
		pastBuffer.back().push_back(id);//记录生产者的id


		// 通知等待的消费者
		cond_full.notify_one();

		lock.unlock();//释放互斥锁
		std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 7) / 5)); // 模拟生产时间
	}
}

// 消费者函数
void producer_consumer::consumer(int id, int items_to_consume)
{
	for (int i = 0; i < items_to_consume; i++)
	{

		std::unique_lock<std::mutex> lock(mtx);//获得互斥锁
		cond_full.wait(lock, [this] { return bufferSize > 0; }); // 等待缓冲区非空

		// 从缓冲区消费数据
		bufferSize--;
		int item = buffer[out];
		buffer[out] = 0;
		out = (out + 1) % bufferLength;
		pastBuffer.push_back(buffer);
		pastBuffer.back().push_back(item);//记录操作数
		pastBuffer.back().push_back(in);//记录插入点
		pastBuffer.back().push_back(out);//记录输出点
		pastBuffer.back().push_back(1);//记录该步操作为消费
		pastBuffer.back().push_back(id);//记录生产者的id


		// 通知等待的生产者
		cond_empty.notify_one();

		lock.unlock();//释放互斥锁
		std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 7) / 5)); // 模拟消费时间
	}
}

//打印信息
void producer_consumer::printBufferinfo(int id)
{
	if (id > 0)
	{
		QString idStr = QString::number(pastBuffer[id][bufferLength + 4]);
		if (pastBuffer[id][bufferLength + 3] == 0)
			ui.proOrConLabel->setText("生产者" + idStr + "号生产了");
		else
			ui.proOrConLabel->setText("消费者" + idStr + "号消费了");
		ui.itemId->setText(QString::number(pastBuffer[id][bufferLength]));
	}
	else
	{
		ui.proOrConLabel->setText("初始状态");
		ui.itemId->setText("");
	}
	for (int i = 0; i < bufferLength; i++)
	{
		QTableWidgetItem* curItem = ui.bufferTable->item(0, i);
		QString curStr = QString::number(pastBuffer[id][i]);
		if (curStr == "0")
			curStr = "---";
		curItem->setText(curStr);
		curItem->setBackground(QBrush(Qt::black));
		curItem->setForeground(QBrush(Qt::white));
		if (id == 0)
		{
			if (i == 0)
				curItem->setBackground(QBrush(QColor(128, 0, 128)));
		}
		else
		{
			if (i == pastBuffer[id][bufferLength + 1] && i == pastBuffer[id][bufferLength + 2])//in和out同位则用这个
				curItem->setBackground(QBrush(QColor(128, 0, 128)));
			else if (i == pastBuffer[id][bufferLength + 1])
				curItem->setBackground(QBrush(Qt::darkBlue));//设置in位置
			else if (i == pastBuffer[id][bufferLength + 2])
				curItem->setBackground(QBrush(Qt::darkRed));//设置out位置
		}

	}
}

void producer_consumer::goToStep()
{
	int id = ui.stepBox->value();
	printBufferinfo(id);
}