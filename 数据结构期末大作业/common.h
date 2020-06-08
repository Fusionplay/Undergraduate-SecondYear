#pragma once
#pragma warning(disable:4996)

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <fstream>
#include <assert.h>
#include <cstring>
#include <time.h>
using namespace std;


#define MAX_COL_SIZE 10    //���������
#define MAX_ATTR_LEN 30	   //��Ŀ����󳤶ȣ�
#define MAX_COMMAND_SEG 20   //ָ���������
#define MAX_COMMAND_SEG_LEN 20  //������󳤶ȣ�



struct datasingle  //�������ݵĽṹ
{
	int id;
	char name[MAX_ATTR_LEN];
	int values[8];
};


struct chainnode
{
	datasingle row;
	chainnode *link;

	chainnode() { link = NULL; }
};


struct chainset
{
	datasingle *row;
	chainset *link;

	chainset() { link = NULL; }
};


class Record  //���ݼ�¼��
{
    static int attrc;   //���Ը���
    static char attr_name[MAX_COL_SIZE][MAX_ATTR_LEN];

public:
    char *attrv[MAX_COL_SIZE];
	int hasdata;
	int newattrc;

    //TODO 
    //realize get and set attr;
    //static int getAttr(char attrs[][MAX_ATTR_LEN]); //get attribute per record
    //static bool setAttr(int num_attr, const char attrs[][MAX_ATTR_LEN]); //set attribute for record before reading records

    Record();
    ~Record();

    friend ifstream & operator >> (ifstream &input, Record &record);
};



class Command  //ָ����
{
public:
    int argc;   //��������
    char *argv[MAX_COMMAND_SEG];  //�ַ�������
    Command();
    ~Command();

    friend ifstream & operator >> (ifstream &input, Command &command); //���ָ��
};



ifstream & operator >> (ifstream &input, Record &record);
ifstream & operator >> (ifstream &input, Command &command);


void INFO(const char* msg);
void helper(Command &command);
void loadData(char *datafile);