#include "NormalLocker.h"
using namespace std;

// 构造：调用父类Locker初始化
NormalLocker::NormalLocker(int r, int c)
    : Locker(r, c) {
}

// 存物：先检查空闲再检查姓名非空，通过后登记使用人
bool NormalLocker::store(string userName) {
    if (getIsUsed()) return false;       // 已占用则失败
    if (userName.empty()) return false;  // 姓名为空则失败
    setUser(userName);                   // 登记使用人
    return true;
}

// 取物：先检查占用再检查姓名匹配，通过后释放柜子
bool NormalLocker::take(string userName) {
    if (!getIsUsed()) return false;            // 空闲则无法取物
    if (userName != getUser()) return false;   // 姓名不匹配则失败
    clearUser();                               // 释放柜子
    return true;
}