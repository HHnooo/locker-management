#include "LockerManager.h"
#include "ThreadPool.h"
#include <iostream>
#include <string>
using namespace std;
//本程序实现的"并发"是指底层通过线程池异步执行操作，
//主线程提交任务后即可继续接受下一个命令，不会因某个操作耗时而阻塞后续输入。
//但由于输入源仅为单个终端和单个 cin，同一时刻只能由一个用户在本地操作，并非网络环境下的多用户同时访问。
//test.cpp 中的并发测试是通过一次性发送一串操作命令，
//验证 LockerManager 在多线程并行处理时是否正确——多个操作同时在不同柜子上执行、各柜子的锁互不干扰。
int main() {
    LockerManager mgr;     // 初始化12个储物柜
    ThreadPool pool(4);    // 4个工作线程
    int choice, row, col;  // 用户选择、储物柜坐标
    string name, pwd;

    while (1) {
        cout << "1. 查看状态  2. 存物  3. 取物  4. 统计空闲  5. 退出" << endl;
        cout << "请选择：";
        cin >> choice;

        if (choice == 5) break;       // 退出

        if (choice == 1) {
            // 查看状态：提交到线程池异步执行
            pool.dispatch([&mgr] { mgr.showAllStatus(); });
        } else if (choice == 2) {
            // 存物：读取坐标和姓名，VIP额外读密码
            cout << "行号(1-3)："; cin >> row;
            cout << "列号(1-4)："; cin >> col;
            cout << "姓名："; cin >> name;
            if (row == 3) { cout << "6位密码："; cin >> pwd; }
            pool.dispatch([&mgr, row, col, name, pwd] {
                mgr.storeItem(row, col, name, pwd);
            });
        } else if (choice == 3) {
            // 取物：读取坐标和姓名，VIP额外读密码
            cout << "行号(1-3)："; cin >> row;
            cout << "列号(1-4)："; cin >> col;
            cout << "姓名："; cin >> name;
            if (row == 3) { cout << "密码："; cin >> pwd; }
            pool.dispatch([&mgr, row, col, name, pwd] {
                mgr.takeItem(row, col, name, pwd);
            });
        } else if (choice == 4) {
            // 统计空闲
            pool.dispatch([&mgr] { mgr.countFree(); });
        } else {
            cout << "输入无效！" << endl;
        }
    }

    cout << "再见！" << endl;
    return 0;
}