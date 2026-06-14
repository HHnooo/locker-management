#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
using namespace std;

// Process：启动子进程运行被测程序，通过管道发送输入、读取输出
struct Process {
    pid_t pid;              // 子进程ID
    int to_child;           // 写入管道：向子进程stdin发数据
    int from_child;         // 读取管道：从子进程stdout读数据
    string cmd;             // 启动命令

    // 启动子进程：fork后子进程用pipe重定向stdin/stdout，父进程保存管道fd
    bool start(const char *p) {
        cmd = p;
        int in[2], out[2];
        pipe(in); pipe(out);
        pid = fork();
        if (pid == 0) {
            dup2(in[0], STDIN_FILENO);
            dup2(out[1], STDOUT_FILENO);
            close(in[1]); close(out[0]);
            close(in[0]); close(out[1]);
            execlp("sh", "sh", "-c", cmd.c_str(), NULL);
            exit(1);
        }
        close(in[0]); close(out[1]);
        to_child = in[1];
        from_child = out[0];
        return pid > 0;
    }

    // 向子进程stdin发送字符串
    void send(const string &s) { write(to_child, s.c_str(), s.size()); }

    // 从子进程stdout读取所有输出，timeout_ms超时后停止
    string readAll(int timeout_ms = 2000) {
        string all; char buf[4096];
        auto dl = chrono::steady_clock::now() + chrono::milliseconds(timeout_ms);
        while (1) {
            auto now = chrono::steady_clock::now();
            if (now >= dl) break;
            int ms = chrono::duration_cast<chrono::milliseconds>(dl - now).count();
            fd_set fds; FD_ZERO(&fds); FD_SET(from_child, &fds);
            struct timeval tv = {ms / 1000, (ms % 1000) * 1000};
            if (select(from_child + 1, &fds, NULL, NULL, &tv) <= 0) break;
            int n = read(from_child, buf, sizeof(buf) - 1);
            if (n <= 0) break;
            buf[n] = 0; all += buf;
        }
        return all;
    }

    // 关闭管道并等待子进程结束
    void stop() { close(to_child); close(from_child); waitpid(pid, NULL, 0); }
};

// Result：收集测试结果，记录每项是否通过
struct Result {
    struct Item { string name; bool ok; };   // 单项测试：名称+通过标记
    vector<Item> items;                       // 所有测试项

    // 记录一项测试结果并打印
    void test(bool ok, const string &name) {
        items.push_back({name, ok});
        string status;
        if (ok) {
            status = "通过";
        } else {
            status = "失败";
        }
        cout << "     [" << name << "] " << status << endl;
    }
    // 返回通过的测试项数量
    int pass() const {
        int n = 0;
        for (int k = 0; k < (int)items.size(); k++) {
            if (items[k].ok) n++;
        }
        return n;
    }
};

// 基础功能测试：启动、存物、重复存物被拒、取物、查看状态、统计、退出
void basicTest(Result &r, const char *prog) {
    Process c; c.start(prog);
    string out = c.readAll(500);
    r.test(out.find("初始化完成") != string::npos, "启动");

    c.send("2\n1\n1\n张三\n");  out = c.readAll(500);
    r.test(out.find("存物成功") != string::npos, "存物(1,1)");

    c.send("2\n1\n1\n李四\n");  out = c.readAll(500);
    r.test(out.find("已被占用") != string::npos, "重复存物被拒");

    c.send("3\n1\n1\n张三\n");  out = c.readAll(500);
    r.test(out.find("取物成功") != string::npos, "取物(1,1)");

    c.send("1\n");  out = c.readAll(500);
    r.test(out.find("【普通储物柜】") != string::npos, "查看状态");

    c.send("4\n");  out = c.readAll(500);
    r.test(out.find("总计空闲") != string::npos, "统计空闲");

    c.send("5\n");  out = c.readAll(500);
    r.test(out.find("再见") != string::npos, "退出");
    c.stop();
}

// 并发测试：通过耗时判断是否利用了多线程
// 设置LOCKER_TEST_DELAY让每次操作sleep 100ms，多线程并行耗时短，单线程串行耗时长
void concurrencyTest(Result &r, const char *prog) {
    setenv("LOCKER_TEST_DELAY", "1", 1);

    // 测试1：8个存物到不同柜子
    // 多线程优势:不同柜子有各自的互斥锁，4个worker可并行处理互不阻塞
    // 多线程：8个操作并发 每个线程执行耗时约200ms < 600ms 通过
    // 单线程：8个操作串行耗时约800ms > 600ms 失败
    {
        Process c;
        c.start(prog);
        c.readAll(500);

        string input_string;
        for (int i = 0; i < 8; i++) {
            int hang = i / 4 + 1;
            int lie  = i % 4 + 1;
            input_string = input_string + "2\n" + to_string(hang) + "\n" + to_string(lie) + "\n用户" + to_string(i) + "\n";
        }
        input_string = input_string + "5\n";

        auto startTime = chrono::steady_clock::now();
        c.send(input_string);
        string output = c.readAll(10000);
        auto endTime = chrono::steady_clock::now();
        c.stop();

        int successCount = 0;
        size_t pos = 0;
        while ((pos = output.find("存物成功", pos)) != string::npos) {
            successCount++;
            pos++;
        }
        long elapsed = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
        bool passed = (successCount == 8 && elapsed < 600);
        string info = " (" + to_string(successCount) + "/8 成功, " + to_string(elapsed) + "ms";
        if (!passed) info = info + ", 无并发";
        info = info + ")";
        r.test(passed, "8存物不同柜" + info);
    }

    // 测试2：8个取物，先批量存入再批量取出
    // 多线程优势:16个操作均在不同柜子上，多线程并行处理互不阻塞
    // 多线程：16个操作并发耗时约400ms < 800ms 通过
    // 单线程：16个操作串行耗时约1600ms > 800ms 失败
    {
        Process c;
        c.start(prog);
        c.readAll(500);

        // 批量存入8个柜子
        string input_string;
        for (int i = 0; i < 8; i++) {
            int hang = i / 4 + 1;
            int lie  = i % 4 + 1;
            input_string = input_string + "2\n" + to_string(hang) + "\n" + to_string(lie) + "\n用户" + to_string(i) + "\n";
        }
        // 批量取出8个柜子
        for (int i = 0; i < 8; i++) {
            int hang = i / 4 + 1;
            int lie  = i % 4 + 1;
            input_string = input_string + "3\n" + to_string(hang) + "\n" + to_string(lie) + "\n用户" + to_string(i) + "\n";
        }
        input_string = input_string + "5\n";

        auto startTime = chrono::steady_clock::now();
        c.send(input_string);
        string output = c.readAll(10000);
        auto endTime = chrono::steady_clock::now();
        c.stop();

        int storeOk = 0;
        int takeOk  = 0;
        size_t pos = 0;
        while ((pos = output.find("存物成功", pos)) != string::npos) { storeOk++; pos++; }
        pos = 0;
        while ((pos = output.find("取物成功", pos)) != string::npos) { takeOk++; pos++; }
        long elapsed = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
        bool passed = (storeOk == 8 && takeOk == 8 && elapsed < 800);
        string info = " (存:" + to_string(storeOk) + "/8 取:" + to_string(takeOk) + "/8, " + to_string(elapsed) + "ms";
        if (!passed) info = info + ", 无并发";
        info = info + ")";
        r.test(passed, "8取物不同柜" + info);
    }

    // 测试3：4个存物加4个取物混合，全部不同柜
    // 多线程优势:12个操作在不同柜子上，存和取各自加锁互不干扰
    // 多线程：12个操作并发耗时约300ms < 600ms 通过
    // 单线程：12个操作串行耗时约1200ms > 600ms 失败
    {
        Process c;
        c.start(prog);
        c.readAll(500);

        // 存入柜子5-8
        string input_string;
        for (int i = 4; i < 8; i++) {
            input_string = input_string + "2\n2\n" + to_string(i - 3) + "\n用户" + to_string(i) + "\n";
        }
        // 存柜子1-4，取柜子5-8
        for (int i = 0; i < 4; i++) {
            input_string = input_string + "2\n1\n" + to_string(i + 1) + "\n用户" + to_string(i) + "\n";
        }
        for (int i = 4; i < 8; i++) {
            input_string = input_string + "3\n2\n" + to_string(i - 3) + "\n用户" + to_string(i) + "\n";
        }
        input_string = input_string + "5\n";

        auto startTime = chrono::steady_clock::now();
        c.send(input_string);
        string output = c.readAll(10000);
        auto endTime = chrono::steady_clock::now();
        c.stop();

        int storeOk = 0;
        int takeOk  = 0;
        size_t pos = 0;
        while ((pos = output.find("存物成功", pos)) != string::npos) { storeOk++; pos++; }
        pos = 0;
        while ((pos = output.find("取物成功", pos)) != string::npos) { takeOk++; pos++; }
        long elapsed = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
        bool passed = (storeOk == 8 && takeOk == 4 && elapsed < 600);
        string info = " (存:" + to_string(storeOk) + "/8 取:" + to_string(takeOk) + "/4, " + to_string(elapsed) + "ms";
        if (!passed) info = info + ", 无并发";
        info = info + ")";
        r.test(passed, "4存+4取混合" + info);
    }
}

// 打印测试汇总：逐项显示名称和结果，最后统计通过/未通过数量
void printSummary(const string &title, const Result &r) {
    cout << "\n" << title << endl;
    for (int k = 0; k < (int)r.items.size(); k++) {
        string status;
        if (r.items[k].ok) {
            status = "通过";
        } else {
            status = "失败";
        }
        cout << "  " << r.items[k].name << ": " << status << endl;
    }
    int err = (int)r.items.size() - r.pass();
    cout << "  通过 " << r.pass() << " 项  未通过 " << err << " 项" << endl;
}

// 测试入口：依次运行单线程测试和多线程测试，打印汇总
int main(int argc, char **argv) {
    string prog = "./main";
    if (argc >= 2) prog = argv[1];
    if (prog.find("/") == string::npos) prog = "./" + prog;

    cout << "--- 单线程测试 ---" << endl;
    Result basicR;
    basicTest(basicR, prog.c_str());

    cout << "\n--- 多线程测试 ---" << endl;
    Result concurrencyR;
    concurrencyTest(concurrencyR, prog.c_str());

    printSummary("\n单线程测试", basicR);
    printSummary("多线程测试", concurrencyR);
    
    return 0;
}