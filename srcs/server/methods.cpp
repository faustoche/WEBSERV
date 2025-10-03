#include "server.hpp"

/*
* Checking if we can delete files without any problem.
* Unable further files
* Does the file exist? Do we have access? Does removing the file worked?
* DOes the directory exist? Do we have access? -> checking permissions with stat
*/

void	c_response::handle_delete_request(const c_request &request, const string &version, string file_path)
{
	if (file_path.find("..") != string::npos) // empeche les suppressions dans les fichiers + loins
	{
		build_error_response(403, version, request);
		return ;
	}
	if (!is_existing_file(file_path))
	{
		build_error_response(404, version, request);
		return ;
	}

	struct stat file_stat;
	if (stat(file_path.c_str(), &file_stat) == 0 && S_ISDIR(file_stat.st_mode))
	{
		build_error_response(403, version, request);
		return ;
	}
	if (access(file_path.c_str(), W_OK) != 0)
	{
		build_error_response(403, version, request);
		return ;
	}
	if (remove(file_path.c_str()) != 0)
	{
		build_error_response(500, version, request);
		return ;
	}
}

void c_response::load_todo_page(const string &version, const c_request &request)
{
	string filename = "./www/data/todo.txt";
	string todo_html = load_file_content("./www/todo.html");
	ifstream infile(filename.c_str());
	string tasks_html;
	string line;

	while (getline(infile, line))
	{
		tasks_html += "<div class=\"todo-item\">";
		tasks_html += "<span class=\"task-text\">" + line + "</span>";
		tasks_html += "<button class=\"delete-btn\" onclick=\"deleteTask('" + line + "')\">DELETE</button>";
		tasks_html += "</div>\n";
	}
	infile.close();

	size_t pos = todo_html.find("{{TASKS_HTML}}");
	if (pos != string::npos)
		todo_html.replace(pos, strlen("{{TASKS_HTML}}"), tasks_html);
	_file_content = todo_html;
	build_success_response("todo.html", version, request);
}

void c_response::handle_todo_form(const c_request &request, const string &version)
{
	map<string, string>form = parse_form_data(request.get_body());
	string task;

	map<string, string>::iterator it = form.find("task");
	if (it != form.end())
		task = it->second;
	else
		task = "";
	if (task.empty())
	{
		build_error_response(400, version, request);
		return ;
	}

	string filename = "./www/data/todo.txt";
	ofstream file(filename.c_str(), ios::app);
	if (!file.is_open())
	{
		build_error_response(500, version, request);
		return ;
	}
	file << task << endl;
	file.close();
	load_todo_page(version, request);
}

void c_response::handle_delete_todo(const c_request &request, const string &version)
{
	string target = request.get_target();
	string task_to_delete;
	size_t pos = target.find("?task=");
	if (pos != string::npos)
	{
		task_to_delete = target.substr(pos + 6);
		task_to_delete = url_decode(task_to_delete);
	}
	else
	{
		build_error_response(400, version, request);
		return ;
	}

	string filename = "./www/data/todo.txt";
	ifstream infile(filename.c_str());
	if (!infile.is_open())
	{
		build_error_response(500, version, request);
		return ;
	}
	
	vector<string> tasks;
	string line;
	int line_count = 0;
	bool found = false;

	while (getline(infile, line))
	{
		line_count++;
		if (line == task_to_delete)
			found = true;
		else
			tasks.push_back(line);
	}
	infile.close();
	if (!found)
		cout << YELLOW << "Error: task cannot be found!" << RESET << endl;
	
	ofstream outfile(filename.c_str(), ios::trunc);
	if (!outfile.is_open())
	{
		build_error_response(500, version, request);
		return ;
	}

	for (size_t i = 0; i < tasks.size(); i++)
		outfile << tasks[i] << endl;
	outfile.close();
	load_todo_page(version, request);
}