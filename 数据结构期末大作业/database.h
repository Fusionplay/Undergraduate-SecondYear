#pragma once
#include "common.h"

#define hashsize 8000
#define lensize 125 //100 0000 / 8000 = 125
#define sortsize 120000
#define setsize 120000
#define bucsize 10240  //ELFhash��Ͱ��


//class MyDatabase;
int ELFhash(char *str);



class Singlebase  //��ɢ����ʵ�֣�
{
	//ɢ�к�����hash(x) = x / lensize

public:
	chainnode* dat[hashsize];   //ɢ������
	int valnum;  //ÿ�������Ե�����

	Singlebase() { valnum = 0; memset(dat, NULL, hashsize * sizeof(chainnode*)); }
	//~Singlebase();

	void output(chainnode* out);

	//����ָ��
	void insert(int id, char* name, int *start);  //����һ������
	void del(int id);  //ɾ��ĳ��id������
	void set(int id, int key, int val);  //����ĳ�����ݵ�ֵ
	void add(int id, int key, int val);  //����ĳ�е�ĳ�����ݵ�ֵ
	void find_id(int id);  //��ID����
	void find_name(char *name);  //�����ֲ���
	void find_keyreange(int key, string op, int val);  //��key����:��Χ


	//����ָ��
	void del_ran(int idl, int idr);  //ɾ��ĳ��id���������
	void set_ran(int idl, int idr, int key, int val);  //����ĳ����Χ�����ݵ�ֵ
	void add_ran(int idl, int idr, int key, int val);  //����ĳ����Χ�����ݵ�ֵ
	void find_idran(int idl, int idr, int key, string op, int val);  //��key����:��һ����ID��Χ��
	void sum_idran(int idl, int idr, int key);         //�����

	//���ר��ָ��
	void find_kthkey(int key, int k, char *mode);
	void find_kthkey_ran(int idl, int idr, int key, int k, char *mode);
	void find_kthkeylist(int key, int k, char *mode);
	void find_kthkey_ran_list(int idl, int idr, int key, int k, char *mode);
};



class setbase
{
public:
	chainnode* dat[bucsize];   //�ַ���ɢ������
	chainset* listhead;  //�����ͷ
	int valnum;
	char name[30]; //set name

	setbase() { valnum = 0; memset(dat, NULL, bucsize * sizeof(chainnode*)); listhead = NULL; }
	inline bool name_exist(int namehash, char *name);
	//inline void output(chainnode* out);
};


void Union(setbase *table1, setbase *table2);
void inter(setbase *table1, setbase *table2);