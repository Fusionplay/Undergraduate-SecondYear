#pragma once
#include "common.h"


class Timer  //��ʱ����
{
public:
    char test_name[20];
    double used_time;

    clock_t start, end;

    void tic(char t[]);
    void toc();
};




class Tester  //������Ĺ�������
{
public:
    char test_name[20];
    ifstream fin;
    //static ofstream fout;

    Tester(const char *test_file, const char *result_file);  //�Ӳ����ļ��д򿪲������������Խ���������ļ�
    ~Tester();

    void exec();  //���Ե�ִ�к���
};








class SingleTester: public Tester  //���������
{
public:
    SingleTester(const char *test_file, const char *result_file);
};


class SectionTester: public Tester   //���������
{
public:
    SectionTester(const char *test_file, const char *result_file);
};


class SetTester: public Tester  //���ϲ�����
{
public:
    SetTester(const char *test_file, const char *result_file);
};