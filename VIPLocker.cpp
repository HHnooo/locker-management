#include "VIPLocker.h"
using namespace std;

// 构造：调用父类初始化，密码为空
VIPLocker::VIPLocker(int r, int c)
    : Locker(r, c) {
    password = "";
}

// 设置密码：长度6且每位都是数字才保存
void VIPLocker::setPassword(string pwd) {
    if (pwd.size() != 6) return;              // 长度不为6则不设置
    for (int i = 0; i < 6; i++) {
        if (pwd[i] < '0' || pwd[i] > '9') return;  // 非数字字符则不设置
    }
    password = pwd;
}

// 验证密码：非空且匹配
bool VIPLocker::verifyPassword(string pwd) {
    if (password.empty()) return false;   // 未设置密码
    if (password != pwd) return false;    // 密码不匹配
    return true;
}

// VIP存物：空闲且姓名密码都合法才能存
bool VIPLocker::storeVIP(string userName, string pwd) {
    if (getIsUsed()) return false;                          // 已占用
    if (userName.empty()) return false;                      // 姓名为空
    if (pwd.size() != 6) return false;                       // 密码长度不对
    for (int i = 0; i < 6; i++) {
        if (pwd[i] < '0' || pwd[i] > '9') return false;     // 密码非纯数字
    }
    setUser(userName);   // 登记使用人
    password = pwd;      // 保存密码
    return true;
}

// VIP取物：占用、姓名、密码三项都匹配才释放柜子
bool VIPLocker::takeVIP(string userName, string pwd) {
    if (!getIsUsed()) return false;          // 空闲则无法取物
    if (userName != getUser()) return false; // 姓名不匹配
    if (pwd != password) return false;       // 密码不匹配
    clearUser();                              // 释放柜子
    return true;
}