#include "common.h"
#include "test.h"

extern FILE *singleout;   //单点
extern FILE *section1out; //区间
extern FILE *section2out; //区间2
extern FILE *setout;      //集合
int cnt = 0;


void Timer::tic(char t[])
{
    strcpy(test_name, t);
    cout << test_name << " test begin." << endl;
    start = clock();

}


void Timer::toc()
{
    end = clock();
    used_time = double(end - start) / CLOCKS_PER_SEC;
    cout << test_name << " test done. Use time: " << used_time << endl;
}





/*时钟类结束 ========================================================================================================================*/






Tester::Tester(const char *test_file, const char *result_file)
{
    fin.open(test_file);      //打开文件到输入流：这里是指令文件！
    //fout.open(result_file);   //输出的结果文件

	switch (cnt)
	{
	case 0:
		singleout = fopen(result_file, "w");
		break;

	case 1:
		section1out = fopen(result_file, "w");
		break;

	case 2:
		section2out = fopen(result_file, "w");
		break;

	case 3:
		setout = fopen(result_file, "w");
		break;
	}

	cnt++;
    //assert((fin && fout) || (INFO("open file failed!"), 0));
	//assert((fin) || (INFO("open file failed!"), 0));
}


Tester::~Tester()
{
    fin.close();
    //fout.close();
}



//测试的执行！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
void Tester::exec()   //测试的执行
{
    Command command;
    Timer timer;
    timer.tic(test_name);

    while (fin >> command)  //通过重载函数输入各个指令
        helper(command);  //解析、执行指令
    
    timer.toc();
}








SingleTester::SingleTester(const char *test_file, const char *result_file): Tester(test_file, result_file)
{
    strcpy(test_name, "Single");   
}


SectionTester::SectionTester(const char *test_file, const char *result_file): Tester(test_file, result_file)
{
    strcpy(test_name, "Section");
}


SetTester::SetTester(const char *test_file, const char *result_file): Tester(test_file, result_file)
{
    strcpy(test_name, "Set");
}