#pragma once
#include "common.h"


class Timer  //计时器类
{
public:
    char test_name[20];
    double used_time;

    clock_t start, end;

    void tic(char t[]);
    void toc();
};




class Tester  //测试类的公共基类
{
public:
    char test_name[20];
    ifstream fin;
    //static ofstream fout;

    Tester(const char *test_file, const char *result_file);  //从测试文件中打开测试用例，测试结果送入结果文件
    ~Tester();

    void exec();  //测试的执行函数
};








class SingleTester: public Tester  //单点测试类
{
public:
    SingleTester(const char *test_file, const char *result_file);
};


class SectionTester: public Tester   //区间测试类
{
public:
    SectionTester(const char *test_file, const char *result_file);
};


class SetTester: public Tester  //集合测试类
{
public:
    SetTester(const char *test_file, const char *result_file);
};