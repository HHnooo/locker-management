#include "Locker.h"
#include <iostream>
using namespace std;

// 构造：初始化行号列号，默认空闲，使用人为空
Locker::Locker(int r, int c) {
    row = r;
    col = c;
    isUsed = false;
    user = "";
}

// 打印"编号：行-列，状态：占用/空闲"
void Locker::showStatus() {
    cout << "编号：" << row << "-" << col << "，状态：";
    if (isUsed) {
        cout << "占用";
    } else {
        cout << "空闲";
    }
    cout << endl;
}

// 返回占用状态
bool Locker::getIsUsed() {
    return isUsed;
}

// 返回使用人姓名
string Locker::getUser() {
    return user;
}

// 保存姓名，标记为占用
void Locker::setUser(string u) {
    user = u;
    isUsed = true;
}

// 清除姓名，标记为空闲
void Locker::clearUser() {
    user = "";
    isUsed = false;
}