#include "timerbtn.h"
#include <QMouseEvent>
#include <QDebug>

TimerBtn::TimerBtn(QWidget *parent):QPushButton(parent),_counter(10)
{
     // 创建定时器对象，父对象为当前按钮（利用Qt父子机制自动管理内存）
    _timer = new QTimer(this);

    // 绑定定时器超时信号：每隔1秒触发一次倒计时逻辑
    connect(_timer, &QTimer::timeout, [this](){
        _counter--;

        // 倒计时结束处理
        if(_counter <= 0){
            _timer->stop(); // 停止定时器
            _counter = 10; // 重置倒计时数值
            this->setText("获取"); // 恢复按钮显示文本
            this->setEnabled(true); // 启用按钮
            return;
        }
        this->setText(QString::number(_counter)); // 更新按钮显示当前倒计时数值
    });
}

TimerBtn::~TimerBtn()
{
    _timer->stop();
}

void TimerBtn::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        // 在这里处理鼠标左键释放事件
        qDebug() << "MyButton was released!";
        this->setEnabled(false);
        this->setText(QString::number(_counter));
        _timer->start(1000); // 1000ms = 1s
        emit clicked();
    }
    // 调用基类的mouseReleaseEvent以确保正常的事件处理（如点击效果）
    QPushButton::mouseReleaseEvent(e);
}
