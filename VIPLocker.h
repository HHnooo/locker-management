#pragma once
#include "Locker.h"

// VIP储物柜：继承Locker，增加6位数字密码保护
class VIPLocker : public Locker {
private:
    string password;   // 6位纯数字密码

public:
    // 构造：调用父类初始化，密码初始为空
    VIPLocker(int r = 0, int c = 0);

    // 设置密码：仅6位纯数字才有效
    void setPassword(string pwd);

    // 验证密码：与存储密码一致返回true
    bool verifyPassword(string pwd);

    // VIP存物：空闲时存入，同时设置使用人和密码
    bool storeVIP(string userName, string pwd);

    // VIP取物：姓名和密码都匹配才能取物
    bool takeVIP(string userName, string pwd);
};