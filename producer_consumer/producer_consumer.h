#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_producer_consumer.h"
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <ctime>

class producer_consumer : public QMainWindow
{
	Q_OBJECT

public:
	producer_consumer(QWidget* parent = nullptr);
	~producer_consumer();
	void consumer(int id, int items_to_consume);
	void producer(int id, int items_to_produce);
	void printBufferinfo(int id);

private:
	Ui::producer_consumerClass ui;

	int producerNumber;//生产者数量
	int consumerNumber;//消费者数量
	int bufferLength;//缓存区长度

	std::vector<std::vector<int>>pastBuffer;//记录每个操作之后的buffer
	std::vector<int> buffer;//线程操作的buffer

	int in = 0;                         // 下一个插入点
	int out = 0;     
	int bufferSize = 0;// 下一个删除点
	std::mutex mtx;                     // 互斥锁
	std::condition_variable cond_full;  // 缓冲区非空条件变量
	std::condition_variable cond_empty; // 缓冲区非满条件变量

private slots:
	void runMainFunction();	
	void goToStep();
};
