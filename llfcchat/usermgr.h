#ifndef USERMGR_H
#define USERMGR_H
#include <QObject>
#include <memory>
#include <singleton.h>
#include "userdata.h"
#include <vector>

class UserMgr:public QObject,public Singleton<UserMgr>,
                public std::enable_shared_from_this<UserMgr>
{
    Q_OBJECT
public:
    friend class Singleton<UserMgr>;
    ~ UserMgr();
    void SetUserInfo(std::shared_ptr<UserInfo> user_info);
    void SetToken(QString token);

    int GetUid();
    QString GetName();
    QString GetNick();
    QString GetIcon();
    QString GetDesc();
    std::shared_ptr<UserInfo> GetUserInfo();

    void AppendApplyList(QJsonArray array); // 追加申请列表
    void AppendFriendList(QJsonArray array); // 追加好友列表
    std::vector<std::shared_ptr<ApplyInfo>> GetApplyList(); // 获取申请列表
    void AddApplyList(std::shared_ptr<ApplyInfo> app); // 向申请列表中添加单个 ApplyInfo 对象
    bool AlreadyApply(int uid); // 检查是否已经对指定的用户 (uid) 发起了申请
    std::vector<std::shared_ptr<FriendInfo>> GetChatListPerPage(); // 分页获取聊天列表（里面的联系人）
    bool IsLoadChatFin(); // 检查聊天列表是否已经完全加载
    void UpdateChatLoadedCount(); // 更新已加载的聊天列表数量
    std::vector<std::shared_ptr<FriendInfo>> GetConListPerPage(); // 分页获取联系人列表（里面的联系人）
    void UpdateContactLoadedCount(); // 更新已加载的联系人列表数量
    bool IsLoadConFin(); // 检查联系人列表是否已经完全加载
    bool CheckFriendById(int uid); // 检查某个用户 (uid) 是否是好友
    void AddFriend(std::shared_ptr<AuthRsp> auth_rsp); // 根据授权响应（AuthRsp）来添加好友
    void AddFriend(std::shared_ptr<AuthInfo> auth_info); // 根据授权信息（AuthInfo）来添加好友
    std::shared_ptr<FriendInfo> GetFriendById(int uid); // 根据好友的 uid 获取对应的 FriendInfo 对象
    void AppendFriendChatMsg(int friend_id,std::vector<std::shared_ptr<TextChatData>>); // 向指定好友的聊天记录中追加聊天消息
private:
    UserMgr();
    std::shared_ptr<UserInfo> _user_info;
    std::vector<std::shared_ptr<ApplyInfo>> _apply_list;
    std::vector<std::shared_ptr<FriendInfo>> _friend_list;
    QMap<int, std::shared_ptr<FriendInfo>> _friend_map;
    QString _token;
    int _chat_loaded; // 已加载的聊天数量
    int _contact_loaded; // 已加载的联系人数量

public slots:
    void SlotAddFriendRsp(std::shared_ptr<AuthRsp> rsp);
    void SlotAddFriendAuth(std::shared_ptr<AuthInfo> auth);
};

#endif // USERMGR_H

