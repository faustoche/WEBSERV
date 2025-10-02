#include "server.hpp"

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
		tasks_html += "<button class=\"delete-btn\" onclick=\"deleteTask('" + line + "')\">🗑 Supprimer</button>";
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
	load_todo_page(version, request); // pour recharger la page avec les taches 
}

void c_response::handle_delete_todo(const c_request &request, const string &version)
{
	string target = request.get_target();
	string task_to_delete;
	size_t pos = target.find("?task=");
	if (pos != string::npos)
	{
		task_to_delete = target.substr(pos + 6); // pour passer le ?task
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
		cout << YELLOW << "Be careful: task cannot be found in the file!" << RESET << endl;
	
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