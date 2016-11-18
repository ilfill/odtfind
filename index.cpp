#include "index.h"


sqlite3 * db=NULL;	//дескриптор БД
char *zErrMsg = 0;	//указатель на массив ошибок
std::string res_once_select;

std::string EscapeString(std::string str)
{
	std::string tmp;
	std::string::size_type index;
	//std::cout <<"asdfasdf"<<str<<"\n";
/*	if((index=str.find('\('))!=std::string::npos) 
		str.replace(index, 1, "\(");
	
	if((index=str.find('\)'))!=std::string::npos) 
		str.replace(index, 1, "\)");
//	std::cout <<"asdfasdf"<<str<<"\n";*/
	return str;

}
int IndexFile (std::string path, std::string shell_cmd)
{
	
	shell_cmd.append(" \"");
	shell_cmd.append(path);
	shell_cmd.append("\" > ~index.tmp");
	std::cout << EscapeString(shell_cmd) << "\n" ;
	int sys = system (EscapeString(shell_cmd).c_str());
	if (sys!=0) return !sys;
	std::string body_text, str;

	std::ifstream in("~index.tmp");
	while(getline(in, str))
		body_text.append(str);//считываем из файла в std::string
	
	std::string sql = "INSERT INTO docs  (path,body) values ('"+path+"','"+body_text+"');";	//добавить его в БД
	std::cout << sql << "\n";
	int rc = sqlite3_exec(db, sql.c_str(), NULL, (void*)NULL, &zErrMsg);	
				//
	//std::cout << body_text;
	
}

std::string GetMaskByPath(std::string path)
{
	size_t found; 
	std::string mystr (path); 
	found = mystr.find_last_of("."); 
	return mystr.substr(found+1); 
}


static int CallbackOnceSelect(void* data, int argc, char **argv, char **azColName)
{
	res_once_select = argv[0];
	return 0;
}




std::string OnceSelect(std::string sql)	// Принимает SQL-запрос и возвращает строку (только одну)
{
	res_once_select.clear();
	//std::cout << "select from db:" <<sql<<"\n";
	sqlite3_exec(db, sql.c_str(), CallbackOnceSelect, (void*)NULL, &zErrMsg);
	return res_once_select;

}


int FileAndSummExist(std::string hash, std::string path)
{
	std::string DBhash=OnceSelect("select md5 from hash where path=\"" +path + "\";" );
	if (DBhash == hash)
		return 1;	//1 - файл существует и его hash равен hash, полученному в результате GetIndex()
	else if (DBhash.empty())
		return -1;	//файл не найден и его надо внести в бд
	else 
		return -2;	//файл найден, но он изменился, и его необходимо переиндексировать
}


static int CallbackReindex(std::string MD5, int argc, char **argv, char **azColName){
	int i;
 //  int country_count = 0;

	for(i=0; i<argc; i++)
	{
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		/*if (!strcmp (azColName[i], "id"))
		{
				country_count = atoi(argv[i]);
			country_list[country_count-1].id = country_count;
		}
		else if (!strcmp (azColName[i], "name"))
		{
			country_list[country_count-1].name = argv[i];	
		}
		else if (!strcmp (azColName[i], "grp"))
		{
			country_list[country_count-1].grp = atoi(argv[i]);	
		}
		*/
		
	}
   
	return 0;
}


unsigned int GetIndex () //Формирует файл ~find.tmp вида md5hash_summ    путь_до_файла.
{
//	std::cout << GetHashCommand();
//	system("md5sum *");
	std::string find_pattern = "\".*\\(";  //итоговый шаблон ".*\(odt\|doc\)$"
	
	std::map <std::string, std::string> mask = GetIndexMask();
	bool first_param = true;
	for (std::map<std::string, std::string>::iterator it = mask.begin(); it!=mask.end(); ++it)
	{
		
		if (!first_param)
			find_pattern += "|";
		find_pattern += it->first;
		find_pattern += "\\";
		first_param = false;
	}
	find_pattern += ")$\" ";
	std::string find_string = "find "+GetHomeFolder()+" -regex ";
	find_string += find_pattern;
	find_string += "-type f ";
	find_string += "-exec md5sum {} \\; > ~find.tmp";
	
	system (find_string.c_str());
	Reindex ("~find.tmp");
	//std::cout <<"\n"<<find_string;
}

unsigned int Reindex(std::string hash_file)  //проверяет таблицу индексов в базе данных с файлом ~find.tmp, 
					     //если появился новый индекс, то файл вносится (перезаписывается в бд)
{
	
	int rc;
	std::string line;
	std::ifstream in(hash_file.c_str());
	rc = sqlite3_exec(db, "BEGIN;", NULL, (void*)NULL, &zErrMsg);
	while(getline(in, line))
	{
		
		std::string word;//Будет содержать текущее слово из текста
		std::stringstream ss(line);//Инициализация строкового потока
		std::vector <std::string> array;//Вектор, каждый элемент которого слово из текста
		ss>>word;
		array.push_back(word);
		std::string tt;
		bool flag=true;
		while(ss>>word)
		{
			if (!flag)
				tt+=" ";	//FIX колхоз, надо будет поправить, проблема в пробелах в именах файлов
			tt+=word;
			flag=false;
		}
		array.push_back(tt);

		int fse = FileAndSummExist(array[0], array[1]); //(HashSumm, FileName) 
	//	std::cout <<"FileAndSummExist "<<fse<<" "<< array[1]<<"\n";
		if (fse==1)	// если файл найден и хэш совпадает
		{	
			
			continue;
		}
		else if (fse == -1)	//файл не найден в бд
		{	
			std::string sql = "INSERT INTO hash  (md5, path) values ('"+array[0]+"','"+array[1]+"');";	//добавить его в БД
			rc = sqlite3_exec(db, sql.c_str(), NULL, (void*)NULL, &zErrMsg);				//
			if (!IndexFile (array[1], GetIndexMask()[GetMaskByPath (array[1])]))				//Проиндексировать его
				std::cout << "error index file "<<array[1]<<"\n";
			continue;
		}
		else if (fse == -2)	//файл найден но изменился
		{	
			continue;
		}
	///	std::cout<<tt<<"\n";
//		std::string sql = "SELECT md5 from hash where path=\""+array[1]+"\"";
		
	///	std::cout << sql<<"\n";
		std::string ggf;
		
		
		
	}
	rc = sqlite3_exec(db, "COMMIT;", NULL, (void*)NULL, &zErrMsg);
}
/*FIX*/int main()
{
	int rc;
	system ("./dbcreater.sh");
	rc = sqlite3_open("df.sql", &db);
	if (GetIndex()>0);
	system ("rm -f ~find.tmp ~index.tmp");
}


