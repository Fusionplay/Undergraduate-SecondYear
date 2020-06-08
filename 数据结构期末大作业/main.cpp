#include "common.h"
#include "test.h"
#include "database.h"

FILE *singleout;
FILE *section1out;
FILE *section2out;
FILE *setout;

int TEST_TYPE = 0;
int loadcnt = 0;
Record rec;
Singlebase sing;
Singlebase sec;
Singlebase mix;
setbase set1;
setbase set2;
string first;


void helper(Command &command)
{
	//TODO
	//recognize and execute command

	first = command.argv[0];

	if (first == "INSERT")
	{
		int *vals = new int[sing.valnum];
		for (int i = 3; i < command.argc; i++)
			vals[i - 3] = atoi(command.argv[i]);

		if (TEST_TYPE == 0) //单点
			sing.insert(atoi(command.argv[1]), command.argv[2], vals);

		else if (TEST_TYPE == 2) //混合
			mix.insert(atoi(command.argv[1]), command.argv[2], vals);

		delete[] vals;
	}


	else if (first == "DELETE")
	{
		if (command.argc == 2) //单点删除
		{
			if (TEST_TYPE == 0) //单点
				sing.del(atoi(command.argv[1]));
			
			else if (TEST_TYPE == 2) //混合
				mix.del(atoi(command.argv[1]));
		}

		else //范围删除
		{
			if (TEST_TYPE == 1) //区间
				sec.del_ran(atoi(command.argv[1]), atoi(command.argv[2]));

			else if (TEST_TYPE == 2) //混合
				mix.del_ran(atoi(command.argv[1]), atoi(command.argv[2]));
		}
	}


	else if (first == "SET")
	{
		if (command.argc == 4) //单点设置
		{
			int key = 0;
			while (strcmp(rec.attrv[key + 2], command.argv[2]) != 0)
				key++;

			if (TEST_TYPE == 0)
				sing.set(atoi(command.argv[1]), key, atoi(command.argv[3]));

			else if (TEST_TYPE == 2)
				mix.set(atoi(command.argv[1]), key, atoi(command.argv[3]));
		}


		else
		{
			int key = 0;
			while (strcmp(rec.attrv[key + 2], command.argv[3]) != 0)
				key++;

			if (TEST_TYPE == 1)
				sec.set_ran(atoi(command.argv[1]), atoi(command.argv[2]), key, atoi(command.argv[4]));

			else if (TEST_TYPE == 2)
				mix.set_ran(atoi(command.argv[1]), atoi(command.argv[2]), key, atoi(command.argv[4]));
		}
	}


	else if (first == "ADD")
	{
		if (command.argc == 4) //单点
		{
			int key = 0;
			while (strcmp(rec.attrv[key + 2], command.argv[2]) != 0)
				key++;

			if (TEST_TYPE == 0)
				sing.add(atoi(command.argv[1]), key, atoi(command.argv[3]));

			else if (TEST_TYPE == 2)
				mix.add(atoi(command.argv[1]), key, atoi(command.argv[3]));
		}


		else //区间
		{
			int key = 0;
			while (strcmp(rec.attrv[key + 2], command.argv[3]) != 0)
				key++;

			if (TEST_TYPE == 1)
				sec.add_ran(atoi(command.argv[1]), atoi(command.argv[2]), key, atoi(command.argv[4]));

			else if (TEST_TYPE == 2)
				mix.add_ran(atoi(command.argv[1]), atoi(command.argv[2]), key, atoi(command.argv[4]));
		}
	}


	else if (first == "QUERY")
	{
		if (TEST_TYPE == 0) //单点
		{
			if (command.argc == 2)  //仅查ID
				sing.find_id(atoi(command.argv[1]));

			else if (strcmp(command.argv[1], "name") == 0) //查name
				sing.find_name(command.argv[3]);

			else //范围单点查找
			{
				int key = 0;
				while (strcmp(rec.attrv[key + 2], command.argv[1]) != 0)
					key++;

				sing.find_keyreange(key, command.argv[2], atoi(command.argv[3]));
			}
		}


		else if (TEST_TYPE == 1) //区间
		{  
			int key = 0;
			while (strcmp(rec.attrv[key + 2], command.argv[3]) != 0)
				key++;

			sec.find_idran(atoi(command.argv[1]), atoi(command.argv[2]), key, command.argv[4], atoi(command.argv[5]));
		}


		else if (TEST_TYPE == 2) //混合
		{
			if (command.argc == 2)  //仅查ID
				mix.find_id(atoi(command.argv[1]));

			else if (strcmp(command.argv[1], "name") == 0) //查name
				mix.find_name(command.argv[3]);


			else if (command.argc == 4)
			{
				if (strcmp(command.argv[3], "ASC") != 0 && strcmp(command.argv[3], "DESC") != 0) //范围单点查找
				{
					int key = 0;
					while (strcmp(rec.attrv[key + 2], command.argv[1]) != 0)
						key++;

					mix.find_keyreange(key, command.argv[2], atoi(command.argv[3]));
				}
				
				else //QUERY key : k ASC/DESC
				{
					int key = 0;
					while (strcmp(rec.attrv[key + 2], command.argv[1]) != 0)
						key++;

					mix.find_kthkey(key, atoi(command.argv[2]), command.argv[3]);
				}
			}


			else if (command.argc == 6)
			{
				if (strcmp(command.argv[5], "ASC") != 0 && strcmp(command.argv[5], "DESC") != 0) //范围区间查找
				{
					int key = 0;
					while (strcmp(rec.attrv[key + 2], command.argv[3]) != 0)
						key++;

					mix.find_idran(atoi(command.argv[1]), atoi(command.argv[2]), key, command.argv[4], atoi(command.argv[5]));
				}

				else //QUERY id1 id2 key : k ASC/DESC
				{
					int key = 0;
					while (strcmp(rec.attrv[key + 2], command.argv[3]) != 0)
						key++;

					mix.find_kthkey_ran(atoi(command.argv[1]), atoi(command.argv[2]), key, atoi(command.argv[4]), command.argv[5]);
				}
			}


			else if (command.argc == 5) //QUERY key : k LIST ASC/DESC
			{
				int key = 0;
				while (strcmp(rec.attrv[key + 2], command.argv[1]) != 0)
					key++;

				mix.find_kthkeylist(key, atoi(command.argv[2]), command.argv[4]);
			}


			else if (command.argc == 7) //QUERY id1 id2 key : k LIST ASC/DESC
			{
				int key = 0;
				while (strcmp(rec.attrv[key + 2], command.argv[3]) != 0)
					key++;

				mix.find_kthkey_ran_list(atoi(command.argv[1]), atoi(command.argv[2]), key, atoi(command.argv[4]), command.argv[6]);
			}
		}
	}


	else if (first == "SUM")
	{
		int key = 0;
		while (strcmp(rec.attrv[key + 2], command.argv[3]) != 0)
			key++;

		if (TEST_TYPE == 1)
			sec.sum_idran(atoi(command.argv[1]), atoi(command.argv[2]), key);

		else if (TEST_TYPE == 2)
			mix.sum_idran(atoi(command.argv[1]), atoi(command.argv[2]), key);
	}


	else if (first == "UNION")
	{
		if (strcmp(command.argv[1], set1.name) == 0)
			Union(&set1, &set2);
		else
			Union(&set2, &set1);
	}


	else if (first == "INTER")
	{
		if (strcmp(command.argv[1], set1.name) == 0)
			inter(&set1, &set2);
		else
			inter(&set2, &set1);
	}
}


void loadData(const char *file)
{
	//TODO
	//load data file
	if (TEST_TYPE != 3)
	{
		Singlebase *table;

		if (TEST_TYPE == 0) //单点
			table = &sing;
		else if (TEST_TYPE == 1)
			table = &sec;
		else if (TEST_TYPE == 2)
			table = &mix;


		ifstream datain(file); //打开文件到输入流：这里是数据文件！
		char tmp[MAX_ATTR_LEN * MAX_COL_SIZE];
		char *p;
		const char *dlim = " ,:";
		datain.getline(tmp, MAX_ATTR_LEN * MAX_COL_SIZE);  //冲掉第一行的文件名
		memset(tmp, '\0', 300);  //先将原缓存字符串清空
		datain >> rec;   //读取各个属性的名称
		table->valnum = rec.newattrc;


		while (!datain.eof())  //每次处理1行
		{
			memset(tmp, '\0', 300);  //先将原缓存字符串清空
			datain.getline(tmp, MAX_ATTR_LEN * MAX_COL_SIZE);    //读入一行

			p = strtok(tmp, dlim);
			int i = 0;
			int valcnt = 0;
			int IDhash;
			int ID;
			chainnode *tr;

			while (p)
			{
				switch (i)
				{
				case 0: //首个数据：ID
					ID = atoi(p);
					IDhash = ID / lensize; //ID的hash值:ID / 125

					if (table->dat[IDhash] == NULL) //该位置被首次访问
					{
						table->dat[IDhash] = new chainnode;
						table->dat[IDhash]->row.id = ID;  //设置ID

						tr = table->dat[IDhash];
					}


					else //该位置bushi首次访问
					{
						tr = table->dat[IDhash];
						while (tr->link != NULL)
							tr = tr->link;

						tr->link = new chainnode;
						tr = tr->link;
						tr->row.id = ID;
					}

					break;


				case 1:  //第二个数据：name
					strcpy(tr->row.name, p);
					break;


				case 2:  //第一个属性：先建立数组
					tr->row.values[valcnt] = atoi(p);
					valcnt++;
					break;


				default:
					tr->row.values[valcnt] = atoi(p);
					valcnt++;
					break;
				}


				i++;
				p = strtok(NULL, dlim);
			}
		}

		datain.close();
	}

	
	else //集合操作
	{
		setbase *table;
		if (loadcnt == 0)
		{
			table = &set1;
			loadcnt++;
		}

		else
			table = &set2;

		ifstream datain(file); //打开文件到输入流：这里是数据文件！
		char tmp[MAX_ATTR_LEN * MAX_COL_SIZE];
		char *p;
		const char *dlim = " ,:";
		datain.getline(tmp, MAX_ATTR_LEN * MAX_COL_SIZE);  //读取第一行的文件名
		strcpy(table->name, tmp);  //获取文件名
		memset(tmp, '\0', 300);  //先将原缓存字符串清空
		datain >> rec;   //读取各个属性的名称
		table->valnum = rec.newattrc;
		chainset *listtemp;


		while (!datain.eof())  //每次处理1行
		{
			chainnode *tr;
			memset(tmp, '\0', 300);  //先将原缓存字符串清空
			datain.getline(tmp, MAX_ATTR_LEN * MAX_COL_SIZE);    //读入一行

			p = strtok(tmp, dlim);  //ID
			int ID = atoi(p);

			p = strtok(NULL, dlim); //name
			int namehash = ELFhash(p);
			
			if (!table->name_exist(namehash, p)) //不存在：读入链表和hash表; 存在：两个表都不读入
			{
				if (table->dat[namehash] == NULL) //该位置被首次访问
				{
					table->dat[namehash] = new chainnode;
					table->dat[namehash]->row.id = ID;  //设置ID
					strcpy(table->dat[namehash]->row.name, p); //设置name
					tr = table->dat[namehash];
				}


				else //该位置不是首次访问
				{
					tr = table->dat[namehash];
					while (tr->link != NULL)
						tr = tr->link;

					tr->link = new chainnode;
					tr = tr->link;
					tr->row.id = ID;
					strcpy(tr->row.name, p);
				}


				int valcnt = 0;
				p = strtok(NULL, dlim);


				while (p)
				{
					tr->row.values[valcnt] = atoi(p);
					valcnt++;
					p = strtok(NULL, dlim);
				}


				if (table->listhead == NULL)
				{
					table->listhead = new chainset;
					listtemp = table->listhead;
					listtemp->row = &tr->row;
				}

				else
				{
					listtemp->link = new chainset;
					listtemp = listtemp->link;
					listtemp->row = &tr->row;
				}
			}
		}

		datain.close();
	}
}


/*=============================================main=======================================================*/


int main(int argc, char const *argv[])
{
	/* code */
	Timer timer;
	timer.tic("All");


	/*
	SingleTester single_tester("single.txt", "result_single.txt");
	SectionTester section_tester1("section1.txt", "result_section1.txt");
	SectionTester section_tester2("section2.txt", "result_section2.txt");
	SetTester set_tester("set.txt", "result_set.txt");
	*/


	SingleTester single_tester("single.txt", "result_single.txt");
	SectionTester section_tester1("section1.txt", "result_section1.txt");
	SectionTester section_tester2("section2.txt", "result_section2.txt");
	SetTester set_tester("set.txt", "result_set.txt");



	//单点测试
	TEST_TYPE = 0;
	loadData("data_single.txt");  //data加载完毕后 开始执行指令
	INFO("=====================================");
	single_tester.exec();   //解析并执行各个指令
	INFO("=====================================\n");

	
	//区间测试1
	TEST_TYPE = 1;
	loadData("data_section1.txt");
	INFO("=====================================");
	section_tester1.exec();
	INFO("=====================================\n");

	
	//区间测试2
	TEST_TYPE = 2;
	loadData("data_section2.txt");
	INFO("=====================================");
	section_tester2.exec();
	INFO("=====================================\n");


	
	//集合测试
	TEST_TYPE = 3;
	loadData("data_set1.txt");
	loadData("data_set2.txt");
	INFO("=====================================");
	set_tester.exec();
	INFO("=====================================\n");


	
	timer.toc();
	INFO("You are a genius!");

	return 0;
}