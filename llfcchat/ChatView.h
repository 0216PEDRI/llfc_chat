#ifndef CHATVIEW_H
#define CHATVIEW_H

#include <QScrollArea>
#include <QVBoxLayout>
#include <QTimer>

class ChatView: public QWidget
{
    Q_OBJECT
public:
    ChatView(QWidget *parent = Q_NULLPTR);
    void appendChatItem(QWidget *item);                  // 尾插
    void prependChatItem(QWidget *item);                 // 头插
    void insertChatItem(QWidget *before, QWidget *item); // 中间插
    void removeAllItem();
protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    void paintEvent(QPaintEvent *event) override;
private slots:
    // 响应垂直滚动条移动的槽函数 on Vertical ScrollBar Moved
    void onVScrollBarMoved(int min, int max);
private:
    void initStyleSheet();
private:
    // QWidget *m_pCenterWidget;
    QVBoxLayout *m_pVl; // m_ 表示成员变量，p 表示指针，Vl 是 Vertical Layout 的缩写
    QScrollArea *m_pScrollArea; // 滚动区域
    bool isAppended;
};

#endif // CHATVIEW_H
