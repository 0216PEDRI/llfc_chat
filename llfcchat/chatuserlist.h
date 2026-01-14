#ifndef CHATUSERLIST_H
#define CHATUSERLIST_H
#include <QListWidget>
#include <QWheelEvent>
#include <QEvent>
#include <QScrollBar>
#include <QDebug>

class ChatUserList: public QListWidget //聊天用户表
{
    Q_OBJECT
public:
    ChatUserList(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    bool _load_pending; // 待加载
signals:
    void sig_loading_chat_user();
};

#endif // CHATUSERLIST_H
